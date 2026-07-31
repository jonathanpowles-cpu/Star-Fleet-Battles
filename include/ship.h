#pragma once
#include "hex.h"
#include "weapons.h"
#include <string>
#include <vector>
#include <algorithm>

enum class Faction { Federation, Klingon, Romulan, Gorn, Kzinti, Tholian, Orion, Hydran, Lyran };
enum class ShipController { Player1, Player2, AI };

// ── Damage Allocation Chart (SFB Rule 5C) ────────────────────────────────────
enum class DACEntry : uint8_t {
    WarpEngine,    // max_warp_power  −3 (damages first live engine group)
    ImpulseEngine, // max_impulse_power −2
    Shield0, Shield1, Shield2, Shield3, Shield4, Shield5, // shields_max[i] −5
    Weapon,        // first non-disabled weapon disabled
    Lab,           // max_lab −1
    Auxiliary,     // hull damage only
    APR,           // apr_current −2
    Bridge,        // bridge_ok = false; crew_casualties +1
};

// ── Individual engine group (for per-engine SSD tracking) ────────────────────
struct EngineGroup {
    std::string label;
    int rated   = 0;
    int current = 0;
};

struct ShipSystems {
    int shields[6]        = {};
    int shields_max[6]    = {};
    int hull              = 100;
    int hull_max          = 100;
    int max_warp_power    = 24;
    int max_impulse_power = 4;
    int max_lab           = 4;

    // Individual engine groups (sum == max_warp/impulse_power)
    std::vector<EngineGroup> warp_groups;
    std::vector<EngineGroup> impulse_groups;

    // APR (Auxiliary Power Reactor)
    int apr_rated   = 0;
    int apr_current = 0;

    // Battery
    int battery_cap    = 0;
    int battery_charge = 0;

    // Crew
    int crew_total      = 6;
    int crew_casualties = 0;
    int crew_available() const { return std::max(0, crew_total - crew_casualties); }

    // Phaser capacitors (display only; rated = bank size)
    int pcap_rated  = 0;
    int pcap_charge = 0;

    // Special systems
    bool bridge_ok       = true;
    bool cloak_installed = false;
    bool cloak_active    = false;  // true when cloaked this turn
    int  cloak_cost      = 0;
    int  shuttles        = 0;
    int  fighter_bay     = 0;      // max fighters carried (R9.F)
};

struct EnergyAllocation {
    int warp_power    = 24;
    int impulse_power = 4;
    int apr_output    = 0;   // 0 .. sys.apr_current
    int battery_tap   = 0;   // 0 .. sys.battery_charge
    int cloak         = 0;   // 0 or sys.cloak_cost (toggle)
    int speed         = 0;
    int reinforce[6]        = {};
    int general_reinforce   = 0;  // D3.341: omnidirectional; absorbs dmg/2 from any shield
    int ecm           = 0;
    int eccm          = 0;
    int repair        = 0;
    int lab           = 0;
    int tractors      = 0;
    int transporters  = 0;
    // Orion engine doubling (G15.2): warp output ×2 this turn, risking DAC roll
    bool engine_double = false;
};

inline bool moves_this_impulse(int speed, int impulse) {
    if (speed <= 0 || impulse < 1 || impulse > 32) return false;
    return (speed * impulse / 32) > (speed * (impulse - 1) / 32);
}

struct Ship {
    std::string      name;
    std::string      ship_class;  // key for SSD PNG lookup (e.g. "fed_ca", "klingon_d7")
    Faction          faction;
    ShipController   controller    = ShipController::Player1;
    Hex              position;
    int              facing        = 0;

    ShipSystems         sys;
    EnergyAllocation    eaf;
    std::vector<Weapon> weapons;
    bool                eaf_committed  = false;
    bool                move_available = false;
    int                 moves_used     = 0;
    bool                destroyed      = false;
    bool                exploded       = false;
    bool                het_used       = false;  // HET performed this turn
    bool                sensor_lock    = true;   // D6.123: false -> ranges doubled this turn
    bool                pos_confirmed  = false;  // placed on board during Placement phase
    int                 hexes_since_turn = 0;  // SFB C2.0 turn mode tracker
    int                 last_speed         = 0;  // (C2.21) accel cap baseline
    int                 emerg_decel_impulses = 0; // (C8.2) restriction impulses remaining
    int                 fighters_aboard = 0;     // current fighter complement (R9.F)
    int                 turn_mode_cat   = 4;     // C3.31: 0=AA,1=A,2=B,3=C,4=D,5=E,6=F

    std::vector<DACEntry> dac;
    int                   dac_position  = 0;
    std::string           last_dac_msg;

    // ── Power budget ──────────────────────────────────────────────────────────
    int total_power() const {
        int warp = eaf.engine_double ? eaf.warp_power * 2 : eaf.warp_power;
        // H6.0: phaser capacitor adds to available pool for this turn
        return warp + eaf.impulse_power + eaf.apr_output + eaf.battery_tap
             + sys.pcap_charge;
    }
    int power_used() const {
        int u = eaf.speed + eaf.ecm + eaf.eccm + eaf.repair + eaf.lab
              + eaf.tractors + eaf.transporters + eaf.cloak;
        for (int i = 0; i < 6; ++i) u += eaf.reinforce[i];
        u += eaf.general_reinforce;
        for (auto& w : weapons) u += w.eaf_cost();
        u += mandatory_power();
        return u;
    }
    int power_remaining() const { return total_power() - power_used(); }
    // A2.0 life support (2 pts) + A5.0 fire control (2 pts): always required
    int mandatory_power() const { return 6; } // A2.0 LS(2) + A5.0 FC(2) + D3.32 shield maint(2)
    // C2.12/B3.0: APR may NOT fund movement; only warp+impulse+battery
    int movement_power() const {
        int warp = eaf.engine_double ? eaf.warp_power * 2 : eaf.warp_power;
        return warp + eaf.impulse_power + eaf.battery_tap;
    }

    // ── Apply one DAC entry ────────────────────────────────────────────────────
    void apply_dac(int entry_idx) {
        if (entry_idx < 0 || entry_idx >= (int)dac.size()) return;
        DACEntry e = dac[entry_idx];
        switch (e) {

        case DACEntry::WarpEngine: {
            int loss = 0;
            for (auto& g : sys.warp_groups) {
                if (g.current > 0) {
                    loss = std::min(3, g.current);
                    g.current -= loss;
                    last_dac_msg = g.label + " HIT! (-" + std::to_string(loss) + " power)";
                    break;
                }
            }
            if (loss == 0) {
                loss = std::min(3, sys.max_warp_power);
                last_dac_msg = "WARP ENGINE HIT (-3)";
            }
            sys.max_warp_power = std::max(0, sys.max_warp_power - loss);
            eaf.warp_power     = std::min(eaf.warp_power, sys.max_warp_power);
            break;
        }

        case DACEntry::ImpulseEngine: {
            int loss = 0;
            for (auto& g : sys.impulse_groups) {
                if (g.current > 0) {
                    loss = std::min(2, g.current);
                    g.current -= loss;
                    last_dac_msg = g.label + " HIT! (-" + std::to_string(loss) + " power)";
                    break;
                }
            }
            if (loss == 0) {
                loss = std::min(2, sys.max_impulse_power);
                last_dac_msg = "IMPULSE ENGINE HIT (-2)";
            }
            sys.max_impulse_power = std::max(0, sys.max_impulse_power - loss);
            eaf.impulse_power     = std::min(eaf.impulse_power, sys.max_impulse_power);
            break;
        }

        case DACEntry::Shield0: case DACEntry::Shield1: case DACEntry::Shield2:
        case DACEntry::Shield3: case DACEntry::Shield4: case DACEntry::Shield5: {
            int si = (int)e - (int)DACEntry::Shield0;
            sys.shields_max[si] = std::max(0, sys.shields_max[si] - 5);
            sys.shields[si]     = std::min(sys.shields[si], sys.shields_max[si]);
            last_dac_msg = std::string("SHIELD ") + char('1'+si) + " GENERATOR HIT (-5 max)";
            break;
        }

        case DACEntry::Weapon:
            for (auto& w : weapons)
                if (!w.disabled) { w.disabled = true; last_dac_msg = w.label + " DESTROYED!"; break; }
            break;

        case DACEntry::Lab:
            sys.max_lab = std::max(0, sys.max_lab - 1);
            last_dac_msg = "LAB / SENSORS HIT";
            break;

        case DACEntry::APR:
            sys.apr_current = std::max(0, sys.apr_current - 2);
            eaf.apr_output  = std::min(eaf.apr_output, sys.apr_current);
            last_dac_msg = "APR HIT! (-2 output)";
            break;

        case DACEntry::Bridge:
            sys.bridge_ok = false;
            sys.crew_casualties = std::min(sys.crew_total, sys.crew_casualties + 1);
            last_dac_msg = "BRIDGE HIT! (crew -1, max speed capped)";
            break;

        case DACEntry::Auxiliary:
            last_dac_msg.clear();
            break;
        }
    }

    // ── Damage application ─────────────────────────────────────────────────────
    void apply_damage(int dmg, int shield_idx) {
        last_dac_msg.clear();
        int idx = shield_idx % 6;
        // D3.341: general reinforcement absorbs dmg/2 (round down) regardless of shield
        if (eaf.general_reinforce > 0) {
            int gen_abs = std::min(dmg, eaf.general_reinforce / 2);
            dmg -= gen_abs;
            if (dmg <= 0) return;
        }
        // D3.342: specific shield reinforcement
        int reinf_abs = std::min(dmg, eaf.reinforce[idx]);
        eaf.reinforce[idx] -= reinf_abs;
        dmg -= reinf_abs;
        if (dmg <= 0) return;
        // Then permanent shield boxes
        int absorbed = std::min(dmg, sys.shields[idx]);
        sys.shields[idx] -= absorbed;
        dmg -= absorbed;
        if (dmg <= 0) return;

        int old_chunks = dac_position / 5;
        int hull_taken = std::min(dmg, sys.hull);
        sys.hull       = std::max(0, sys.hull - dmg);
        dac_position  += hull_taken;
        int new_chunks = dac_position / 5;
        for (int i = old_chunks; i < new_chunks; ++i) apply_dac(i);
        if (sys.hull == 0) destroyed = true;
    }

    // Hellbore (R9.H): half damage bypasses shield and hits hull directly
    void apply_damage_hellbore(int dmg, int shield_idx) {
        int hull_direct = dmg / 2;
        int shield_half = dmg - hull_direct;
        // Hull-direct portion: skip shield, go straight to hull/DAC
        if (hull_direct > 0) {
            int old_chunks = dac_position / 5;
            sys.hull = std::max(0, sys.hull - hull_direct);
            dac_position += hull_direct;
            int new_chunks = dac_position / 5;
            for (int i = old_chunks; i < new_chunks; ++i) apply_dac(i);
            if (sys.hull == 0) { destroyed = true; return; }
        }
        // Shield-half goes through normal path
        if (shield_half > 0) apply_damage(shield_half, shield_idx);
    }

    // ── Turn reset ─────────────────────────────────────────────────────────────
    // D3.2: set aft arc on weapons whose label indicates a rear-facing mount.
    // Called once from each factory; idempotent (only modifies ARC_BROAD weapons).
    void assign_weapon_arcs() {
        static constexpr uint8_t BROAD = 0b00110111;  // ARC_BROAD (default)
        static constexpr uint8_t REAR  = 0b00011100;  // ARC_REAR
        for (auto& w : weapons) {
            if (w.arc != BROAD) continue;  // already explicitly set, leave alone
            const std::string& lbl = w.label;
            auto has = [&](const char* sub) {
                return lbl.find(sub) != std::string::npos;
            };
            if (has(" RA") || has(" RS") || has(" LR") || has(" RR") ||
                has(" PA") || has(" SA") || has("(PA)") || has("(SA)") ||
                has(" A)") || has("Aft") || has(" aft") ||
                has("Rear") || has(" rear"))
                w.arc = REAR;
        }
    }

    void begin_planning() {
        // D9.0 Damage Control: 2 energy -> 1 shield box repaired at end of turn
        if (eaf.repair > 0) {
            int boxes = eaf.repair / 2;
            for (int i = 0; i < 6 && boxes > 0; ++i) {
                int restore = std::min(boxes, sys.shields_max[i] - sys.shields[i]);
                sys.shields[i] += restore;
                boxes -= restore;
            }
        }

        last_speed         = eaf.speed;   // (C2.21) record committed speed for next-turn accel cap
        het_used           = false;
        eaf_committed      = false;
        move_available     = false;
        moves_used         = 0;
        // hexes_since_turn carries over (C3.41): only reset on actual turn/HET
        // emerg_decel_impulses counts down per impulse during Impulse phase, not reset here

        eaf.warp_power    = sys.max_warp_power;
        eaf.impulse_power = sys.max_impulse_power;
        eaf.apr_output    = 0;
        eaf.battery_tap   = 0;
        eaf.cloak         = sys.cloak_active ? sys.cloak_cost : 0;
        eaf.speed         = last_speed;  // C2.21: carry committed speed from last turn
        for (int i = 0; i < 6; ++i) eaf.reinforce[i] = 0;
        eaf.general_reinforce = 0;
        eaf.ecm = eaf.eccm = eaf.repair = eaf.lab = eaf.tractors = eaf.transporters = 0;
        // H6.0: capture unfired instant-weapon energy into phaser capacitor
        if (sys.pcap_rated > 0) {
            for (auto& w : weapons) {
                if (w.is_instant() && w.allocated > 0 && !w.fired) {
                    int carry = std::min(w.allocated, sys.pcap_rated - sys.pcap_charge);
                    sys.pcap_charge = std::min(sys.pcap_rated, sys.pcap_charge + carry);
                }
            }
        }
        for (auto& w : weapons) {
            w.fired = false;
            if (w.type == WeaponType::ESG) {
                // G23.0: carry over unspent ESG energy (capped at 5)
                w.stored = std::min(5, w.stored + w.allocated);
                w.allocated = 0;
            } else if (w.is_instant()) {
                w.allocated = 0;
            } else if (!w.armed) {
                w.allocated = 0;
            }
        }
        sensor_lock = true;  // D6.123: reset each turn; re-checked at EAF commit
    }

    // ── Explosion (SFB 5D2) ────────────────────────────────────────────────────
    int explosion_power()  const { return std::max(5, (sys.max_warp_power + sys.max_impulse_power) / 3); }
    int explosion_radius() const { return std::min(3, explosion_power() / 5 + 1); }
    int explosion_damage_at(int dist) const {
        if (dist > explosion_radius()) return 0;
        return std::max(1, explosion_power() / (dist + 1));
    }
};

// ── Factory functions ──────────────────────────────────────────────────────────

// -- Crew advisory (player assistance) -------------------------------------------
struct CrewAdvice {
    std::string helm;
    std::string weapons;
    std::string engineering;
    std::string science;
    int         speed   = -1;
    std::string charge_weapon;
    bool        recommend_cloak = false;
    bool        valid = false;
};
Ship make_federation_ca      (std::string name, Hex pos, int facing);
Ship make_klingon_d7         (std::string name, Hex pos, int facing);
Ship make_romulan_kr         (std::string name, Hex pos, int facing);
Ship make_gorn_ca            (std::string name, Hex pos, int facing);
Ship make_kzinti_cw          (std::string name, Hex pos, int facing);

// Additional ships (ships_all.cpp)
Ship make_axcvl              (std::string name, Hex pos, int facing);
Ship make_small_freighter    (std::string name, Hex pos, int facing);
Ship make_large_freighter    (std::string name, Hex pos, int facing);
Ship make_fed_lq             (std::string name, Hex pos, int facing);
Ship make_fed_sq             (std::string name, Hex pos, int facing);
Ship make_klingon_lq         (std::string name, Hex pos, int facing);
Ship make_klingon_sq         (std::string name, Hex pos, int facing);
Ship make_gorn_lq            (std::string name, Hex pos, int facing);
Ship make_gorn_sq            (std::string name, Hex pos, int facing);
Ship make_federation_dn      (std::string name, Hex pos, int facing);
Ship make_federation_cc      (std::string name, Hex pos, int facing);
Ship make_federation_cl      (std::string name, Hex pos, int facing);
Ship make_federation_dd      (std::string name, Hex pos, int facing);
Ship make_federation_sc      (std::string name, Hex pos, int facing);
Ship make_federation_tug     (std::string name, Hex pos, int facing);
Ship make_federation_battle_tug(std::string name, Hex pos, int facing);
Ship make_romulan_wb         (std::string name, Hex pos, int facing);
Ship make_romulan_we         (std::string name, Hex pos, int facing);
Ship make_romulan_k5r        (std::string name, Hex pos, int facing);
Ship make_battle_station     (std::string name, Hex pos, int facing);
Ship make_starbase           (std::string name, Hex pos, int facing);
Ship make_base_station       (std::string name, Hex pos, int facing);
Ship make_klingon_c9         (std::string name, Hex pos, int facing);
Ship make_klingon_c8         (std::string name, Hex pos, int facing);
Ship make_klingon_d6         (std::string name, Hex pos, int facing);
Ship make_klingon_f5         (std::string name, Hex pos, int facing);
Ship make_klingon_e4         (std::string name, Hex pos, int facing);
Ship make_kzinti_bc          (std::string name, Hex pos, int facing);
Ship make_kzinti_cc          (std::string name, Hex pos, int facing);
Ship make_kzinti_cl          (std::string name, Hex pos, int facing);
Ship make_kzinti_cv          (std::string name, Hex pos, int facing);
Ship make_kzinti_cvs         (std::string name, Hex pos, int facing);
Ship make_kzinti_ff          (std::string name, Hex pos, int facing);
Ship make_kzinti_eff         (std::string name, Hex pos, int facing);
Ship make_kzinti_aff         (std::string name, Hex pos, int facing);
Ship make_gorn_bc            (std::string name, Hex pos, int facing);
Ship make_gorn_cl            (std::string name, Hex pos, int facing);
Ship make_gorn_dd            (std::string name, Hex pos, int facing);
Ship make_gorn_ddf           (std::string name, Hex pos, int facing);
Ship make_tholian_pc         (std::string name, Hex pos, int facing);
Ship make_tholian_pc_plus    (std::string name, Hex pos, int facing);
Ship make_orion_cr           (std::string name, Hex pos, int facing);
Ship make_federation_pod     (std::string name, Hex pos, int facing);
Ship make_hydran_hdn         (std::string name, Hex pos, int facing);
Ship make_hydran_lord        (std::string name, Hex pos, int facing);
Ship make_hydran_ranger      (std::string name, Hex pos, int facing);
Ship make_hydran_lancer      (std::string name, Hex pos, int facing);
Ship make_hydran_cuirassier  (std::string name, Hex pos, int facing);
Ship make_lyran_ca           (std::string name, Hex pos, int facing);
Ship make_lyran_bc           (std::string name, Hex pos, int facing);
Ship make_lyran_cl           (std::string name, Hex pos, int facing);
Ship make_lyran_dd           (std::string name, Hex pos, int facing);
Ship make_lyran_ff           (std::string name, Hex pos, int facing);
Ship make_federation_ff      (std::string name, Hex pos, int facing);
Ship make_romulan_condor     (std::string name, Hex pos, int facing);
Ship make_romulan_ke         (std::string name, Hex pos, int facing);
Ship make_romulan_sh         (std::string name, Hex pos, int facing);
Ship make_romulan_sky        (std::string name, Hex pos, int facing);
Ship make_romulan_snipe      (std::string name, Hex pos, int facing);
Ship make_gorn_ff            (std::string name, Hex pos, int facing);
Ship make_gorn_sc            (std::string name, Hex pos, int facing);
Ship make_tholian_dd         (std::string name, Hex pos, int facing);
Ship make_tholian_co         (std::string name, Hex pos, int facing);
Ship make_orion_lr           (std::string name, Hex pos, int facing);
Ship make_orion_br           (std::string name, Hex pos, int facing);
Ship make_orion_ca           (std::string name, Hex pos, int facing);

// Auto-generated declarations (C1/R2/R3/J/MSB)
Ship make_andro_ana		(std::string name, Hex pos, int facing);
Ship make_andro_asp		(std::string name, Hex pos, int facing);
Ship make_andro_kin		(std::string name, Hex pos, int facing);
Ship make_andro_kra		(std::string name, Hex pos, int facing);
Ship make_fed_acl		(std::string name, Hex pos, int facing);
Ship make_fed_bb		(std::string name, Hex pos, int facing);
Ship make_fed_bcf		(std::string name, Hex pos, int facing);
Ship make_fed_bcg		(std::string name, Hex pos, int facing);
Ship make_fed_bcj		(std::string name, Hex pos, int facing);
Ship make_fed_bcs		(std::string name, Hex pos, int facing);
Ship make_fed_ca_plus		(std::string name, Hex pos, int facing);
Ship make_fed_cad		(std::string name, Hex pos, int facing);
Ship make_fed_car		(std::string name, Hex pos, int facing);
Ship make_fed_cb		(std::string name, Hex pos, int facing);
Ship make_fed_cf		(std::string name, Hex pos, int facing);
Ship make_fed_clc		(std::string name, Hex pos, int facing);
Ship make_fed_clh		(std::string name, Hex pos, int facing);
Ship make_fed_cls		(std::string name, Hex pos, int facing);
Ship make_fed_cmc		(std::string name, Hex pos, int facing);
Ship make_fed_cmc_cov		(std::string name, Hex pos, int facing);
Ship make_fed_cov		(std::string name, Hex pos, int facing);
Ship make_fed_cs		(std::string name, Hex pos, int facing);
Ship make_fed_cva		(std::string name, Hex pos, int facing);
Ship make_fed_cvb		(std::string name, Hex pos, int facing);
Ship make_fed_cve		(std::string name, Hex pos, int facing);
Ship make_fed_cvl		(std::string name, Hex pos, int facing);
Ship make_fed_cvs		(std::string name, Hex pos, int facing);
Ship make_fed_cx		(std::string name, Hex pos, int facing);
Ship make_fed_dar		(std::string name, Hex pos, int facing);
Ship make_fed_ddg		(std::string name, Hex pos, int facing);
Ship make_fed_ddl		(std::string name, Hex pos, int facing);
Ship make_fed_de		(std::string name, Hex pos, int facing);
Ship make_fed_dea		(std::string name, Hex pos, int facing);
Ship make_fed_der		(std::string name, Hex pos, int facing);
Ship make_fed_dn_plus		(std::string name, Hex pos, int facing);
Ship make_fed_dng		(std::string name, Hex pos, int facing);
Ship make_fed_dnl		(std::string name, Hex pos, int facing);
Ship make_fed_dw		(std::string name, Hex pos, int facing);
Ship make_fed_dwa		(std::string name, Hex pos, int facing);
Ship make_fed_dwc		(std::string name, Hex pos, int facing);
Ship make_fed_dwd		(std::string name, Hex pos, int facing);
Ship make_fed_dwe		(std::string name, Hex pos, int facing);
Ship make_fed_dwm		(std::string name, Hex pos, int facing);
Ship make_fed_dws		(std::string name, Hex pos, int facing);
Ship make_fed_dwt		(std::string name, Hex pos, int facing);
Ship make_fed_ecl		(std::string name, Hex pos, int facing);
Ship make_fed_ffa		(std::string name, Hex pos, int facing);
Ship make_fed_ffb		(std::string name, Hex pos, int facing);
Ship make_fed_ffd		(std::string name, Hex pos, int facing);
Ship make_fed_ffe		(std::string name, Hex pos, int facing);
Ship make_fed_ffg		(std::string name, Hex pos, int facing);
Ship make_fed_ffl		(std::string name, Hex pos, int facing);
Ship make_fed_ffm		(std::string name, Hex pos, int facing);
Ship make_fed_ffp		(std::string name, Hex pos, int facing);
Ship make_fed_ffr		(std::string name, Hex pos, int facing);
Ship make_fed_ffs		(std::string name, Hex pos, int facing);
Ship make_fed_fft		(std::string name, Hex pos, int facing);
Ship make_fed_ffv		(std::string name, Hex pos, int facing);
Ship make_fed_fra		(std::string name, Hex pos, int facing);
Ship make_fed_gsc		(std::string name, Hex pos, int facing);
Ship make_fed_hcc		(std::string name, Hex pos, int facing);
Ship make_fed_lbt		(std::string name, Hex pos, int facing);
Ship make_fed_ltt		(std::string name, Hex pos, int facing);
Ship make_fed_nac		(std::string name, Hex pos, int facing);
Ship make_fed_nca		(std::string name, Hex pos, int facing);
Ship make_fed_ncc		(std::string name, Hex pos, int facing);
Ship make_fed_ncd		(std::string name, Hex pos, int facing);
Ship make_fed_ncl		(std::string name, Hex pos, int facing);
Ship make_fed_ncva		(std::string name, Hex pos, int facing);
Ship make_fed_nea		(std::string name, Hex pos, int facing);
Ship make_fed_nec		(std::string name, Hex pos, int facing);
Ship make_fed_ner		(std::string name, Hex pos, int facing);
Ship make_fed_nhcc		(std::string name, Hex pos, int facing);
Ship make_fed_nsc		(std::string name, Hex pos, int facing);
Ship make_fed_nvh		(std::string name, Hex pos, int facing);
Ship make_fed_nvl		(std::string name, Hex pos, int facing);
Ship make_fed_nvs		(std::string name, Hex pos, int facing);
Ship make_fed_pv		(std::string name, Hex pos, int facing);
Ship make_fed_scout_dd		(std::string name, Hex pos, int facing);
Ship make_fed_scs		(std::string name, Hex pos, int facing);
Ship make_fed_scsa		(std::string name, Hex pos, int facing);
Ship make_gen_axcva		(std::string name, Hex pos, int facing);
Ship make_gen_axcvl		(std::string name, Hex pos, int facing);
Ship make_hydran_apa		(std::string name, Hex pos, int facing);
Ship make_hydran_bar		(std::string name, Hex pos, int facing);
Ship make_hydran_cat		(std::string name, Hex pos, int facing);
Ship make_hydran_cav		(std::string name, Hex pos, int facing);
Ship make_hydran_com		(std::string name, Hex pos, int facing);
Ship make_hydran_cos		(std::string name, Hex pos, int facing);
Ship make_hydran_cru		(std::string name, Hex pos, int facing);
Ship make_hydran_cu		(std::string name, Hex pos, int facing);
Ship make_hydran_cve		(std::string name, Hex pos, int facing);
Ship make_hydran_d7h		(std::string name, Hex pos, int facing);
Ship make_hydran_da		(std::string name, Hex pos, int facing);
Ship make_hydran_de		(std::string name, Hex pos, int facing);
Ship make_hydran_dg		(std::string name, Hex pos, int facing);
Ship make_hydran_eh		(std::string name, Hex pos, int facing);
Ship make_hydran_gen		(std::string name, Hex pos, int facing);
Ship make_hydran_hn		(std::string name, Hex pos, int facing);
Ship make_hydran_hr		(std::string name, Hex pos, int facing);
Ship make_hydran_id		(std::string name, Hex pos, int facing);
Ship make_hydran_kn		(std::string name, Hex pos, int facing);
Ship make_hydran_lb		(std::string name, Hex pos, int facing);
Ship make_hydran_lc		(std::string name, Hex pos, int facing);
Ship make_hydran_lm		(std::string name, Hex pos, int facing);
Ship make_hydran_ln		(std::string name, Hex pos, int facing);
Ship make_hydran_lp		(std::string name, Hex pos, int facing);
Ship make_hydran_ltt		(std::string name, Hex pos, int facing);
Ship make_hydran_mng		(std::string name, Hex pos, int facing);
Ship make_hydran_ms		(std::string name, Hex pos, int facing);
Ship make_hydran_nac		(std::string name, Hex pos, int facing);
Ship make_hydran_nec		(std::string name, Hex pos, int facing);
Ship make_hydran_nms		(std::string name, Hex pos, int facing);
Ship make_hydran_npf		(std::string name, Hex pos, int facing);
Ship make_hydran_nsc		(std::string name, Hex pos, int facing);
Ship make_hydran_nvl		(std::string name, Hex pos, int facing);
Ship make_hydran_ov		(std::string name, Hex pos, int facing);
Ship make_hydran_pal		(std::string name, Hex pos, int facing);
Ship make_hydran_rn		(std::string name, Hex pos, int facing);
Ship make_hydran_sar		(std::string name, Hex pos, int facing);
Ship make_hydran_sc		(std::string name, Hex pos, int facing);
Ship make_hydran_sr		(std::string name, Hex pos, int facing);
Ship make_hydran_srg		(std::string name, Hex pos, int facing);
Ship make_hydran_srv		(std::string name, Hex pos, int facing);
Ship make_hydran_tar		(std::string name, Hex pos, int facing);
Ship make_hydran_tr		(std::string name, Hex pos, int facing);
Ship make_hydran_uh		(std::string name, Hex pos, int facing);
Ship make_hydran_war		(std::string name, Hex pos, int facing);
Ship make_kli_ad5		(std::string name, Hex pos, int facing);
Ship make_kli_c8v		(std::string name, Hex pos, int facing);
Ship make_kli_d7v		(std::string name, Hex pos, int facing);
Ship make_kli_e4e		(std::string name, Hex pos, int facing);
Ship make_kli_f5v		(std::string name, Hex pos, int facing);
Ship make_klingon_ad6		(std::string name, Hex pos, int facing);
Ship make_klingon_af5		(std::string name, Hex pos, int facing);
Ship make_klingon_c7		(std::string name, Hex pos, int facing);
Ship make_klingon_c7a		(std::string name, Hex pos, int facing);
Ship make_klingon_c8s		(std::string name, Hex pos, int facing);
Ship make_klingon_c9a		(std::string name, Hex pos, int facing);
Ship make_klingon_d5c		(std::string name, Hex pos, int facing);
Ship make_klingon_d5d		(std::string name, Hex pos, int facing);
Ship make_klingon_d5e		(std::string name, Hex pos, int facing);
Ship make_klingon_d5f		(std::string name, Hex pos, int facing);
Ship make_klingon_d5g		(std::string name, Hex pos, int facing);
Ship make_klingon_d5h		(std::string name, Hex pos, int facing);
Ship make_klingon_d5i		(std::string name, Hex pos, int facing);
Ship make_klingon_d5j		(std::string name, Hex pos, int facing);
Ship make_klingon_d5k		(std::string name, Hex pos, int facing);
Ship make_klingon_d5l		(std::string name, Hex pos, int facing);
Ship make_klingon_d5m		(std::string name, Hex pos, int facing);
Ship make_klingon_d5n		(std::string name, Hex pos, int facing);
Ship make_klingon_d5p		(std::string name, Hex pos, int facing);
Ship make_klingon_d5s		(std::string name, Hex pos, int facing);
Ship make_klingon_d5v		(std::string name, Hex pos, int facing);
Ship make_klingon_d6e		(std::string name, Hex pos, int facing);
Ship make_klingon_d6g		(std::string name, Hex pos, int facing);
Ship make_klingon_d6j		(std::string name, Hex pos, int facing);
Ship make_klingon_d6s		(std::string name, Hex pos, int facing);
Ship make_klingon_d7cx		(std::string name, Hex pos, int facing);
Ship make_klingon_d7d		(std::string name, Hex pos, int facing);
Ship make_klingon_d7e		(std::string name, Hex pos, int facing);
Ship make_klingon_d7m		(std::string name, Hex pos, int facing);
Ship make_klingon_d7n		(std::string name, Hex pos, int facing);
Ship make_klingon_d7v		(std::string name, Hex pos, int facing);
Ship make_klingon_e3d		(std::string name, Hex pos, int facing);
Ship make_klingon_e4d		(std::string name, Hex pos, int facing);
Ship make_klingon_e4j		(std::string name, Hex pos, int facing);
Ship make_klingon_e4v		(std::string name, Hex pos, int facing);
Ship make_klingon_e5		(std::string name, Hex pos, int facing);
Ship make_klingon_f5e		(std::string name, Hex pos, int facing);
Ship make_klingon_f5j		(std::string name, Hex pos, int facing);
Ship make_klingon_f5x		(std::string name, Hex pos, int facing);
Ship make_klingon_f6		(std::string name, Hex pos, int facing);
Ship make_klingon_md5		(std::string name, Hex pos, int facing);
Ship make_klingon_rkl		(std::string name, Hex pos, int facing);
Ship make_kzin_bch		(std::string name, Hex pos, int facing);
Ship make_kzin_ca		(std::string name, Hex pos, int facing);
Ship make_kzin_cd		(std::string name, Hex pos, int facing);
Ship make_kzin_cva		(std::string name, Hex pos, int facing);
Ship make_kzin_dd		(std::string name, Hex pos, int facing);
Ship make_kzin_ddv		(std::string name, Hex pos, int facing);
Ship make_kzin_dn		(std::string name, Hex pos, int facing);
Ship make_kzin_dw		(std::string name, Hex pos, int facing);
Ship make_kzin_dwa		(std::string name, Hex pos, int facing);
Ship make_kzin_dwd		(std::string name, Hex pos, int facing);
Ship make_kzin_dwe		(std::string name, Hex pos, int facing);
Ship make_kzin_dwl		(std::string name, Hex pos, int facing);
Ship make_kzin_dws		(std::string name, Hex pos, int facing);
Ship make_kzin_ffk		(std::string name, Hex pos, int facing);
Ship make_kzin_fh		(std::string name, Hex pos, int facing);
Ship make_kzin_mac		(std::string name, Hex pos, int facing);
Ship make_kzin_mcc		(std::string name, Hex pos, int facing);
Ship make_kzin_mcv		(std::string name, Hex pos, int facing);
Ship make_kzin_mdc		(std::string name, Hex pos, int facing);
Ship make_kzin_mec		(std::string name, Hex pos, int facing);
Ship make_kzin_mms		(std::string name, Hex pos, int facing);
Ship make_kzin_mpf		(std::string name, Hex pos, int facing);
Ship make_kzin_msc		(std::string name, Hex pos, int facing);
Ship make_kzin_mtt		(std::string name, Hex pos, int facing);
Ship make_kzin_pol		(std::string name, Hex pos, int facing);
Ship make_kzin_sdf		(std::string name, Hex pos, int facing);
Ship make_kzin_sr		(std::string name, Hex pos, int facing);
Ship make_kzin_srv		(std::string name, Hex pos, int facing);
Ship make_kzin_sscs		(std::string name, Hex pos, int facing);
Ship make_lyran_bch		(std::string name, Hex pos, int facing);
Ship make_lyran_cc		(std::string name, Hex pos, int facing);
Ship make_lyran_cv		(std::string name, Hex pos, int facing);
Ship make_lyran_cvl		(std::string name, Hex pos, int facing);
Ship make_lyran_cw		(std::string name, Hex pos, int facing);
Ship make_lyran_cwa		(std::string name, Hex pos, int facing);
Ship make_lyran_cwe		(std::string name, Hex pos, int facing);
Ship make_lyran_cwl		(std::string name, Hex pos, int facing);
Ship make_lyran_cwm		(std::string name, Hex pos, int facing);
Ship make_lyran_cws		(std::string name, Hex pos, int facing);
Ship make_lyran_dn		(std::string name, Hex pos, int facing);
Ship make_lyran_dw		(std::string name, Hex pos, int facing);
Ship make_lyran_dwa		(std::string name, Hex pos, int facing);
Ship make_lyran_dwe		(std::string name, Hex pos, int facing);
Ship make_lyran_dwl		(std::string name, Hex pos, int facing);
Ship make_lyran_dwm		(std::string name, Hex pos, int facing);
Ship make_lyran_dws		(std::string name, Hex pos, int facing);
Ship make_lyran_ffa		(std::string name, Hex pos, int facing);
Ship make_lyran_ffe		(std::string name, Hex pos, int facing);
Ship make_lyran_ltt		(std::string name, Hex pos, int facing);
Ship make_lyran_ltv		(std::string name, Hex pos, int facing);
Ship make_lyran_mp		(std::string name, Hex pos, int facing);
Ship make_lyran_ms		(std::string name, Hex pos, int facing);
Ship make_lyran_pfw		(std::string name, Hex pos, int facing);
Ship make_lyran_pol		(std::string name, Hex pos, int facing);
Ship make_lyran_sc		(std::string name, Hex pos, int facing);
Ship make_lyran_scs		(std::string name, Hex pos, int facing);
Ship make_lyran_sr		(std::string name, Hex pos, int facing);
Ship make_lyran_srv		(std::string name, Hex pos, int facing);
Ship make_lyran_stj		(std::string name, Hex pos, int facing);
Ship make_lyran_stt		(std::string name, Hex pos, int facing);
Ship make_lyran_tgc		(std::string name, Hex pos, int facing);
Ship make_lyran_tgp		(std::string name, Hex pos, int facing);
Ship make_orion_ar		(std::string name, Hex pos, int facing);
Ship make_orion_bch		(std::string name, Hex pos, int facing);
Ship make_orion_brp		(std::string name, Hex pos, int facing);
Ship make_orion_cvs		(std::string name, Hex pos, int facing);
Ship make_orion_dbp		(std::string name, Hex pos, int facing);
Ship make_orion_dbr		(std::string name, Hex pos, int facing);
Ship make_orion_dw		(std::string name, Hex pos, int facing);
Ship make_orion_dws		(std::string name, Hex pos, int facing);
Ship make_orion_hr		(std::string name, Hex pos, int facing);
Ship make_orion_lrs		(std::string name, Hex pos, int facing);
Ship make_orion_mr		(std::string name, Hex pos, int facing);
Ship make_orion_ok6		(std::string name, Hex pos, int facing);
Ship make_wyn_axbc		(std::string name, Hex pos, int facing);
Ship make_wyn_axc		(std::string name, Hex pos, int facing);
Ship make_wyn_axcv		(std::string name, Hex pos, int facing);
Ship make_wyn_axcva		(std::string name, Hex pos, int facing);
Ship make_wyn_axms		(std::string name, Hex pos, int facing);
Ship make_wyn_ke4		(std::string name, Hex pos, int facing);
Ship make_wyn_kg2		(std::string name, Hex pos, int facing);
Ship make_wyn_ldd		(std::string name, Hex pos, int facing);
Ship make_wyn_obr		(std::string name, Hex pos, int facing);
Ship make_wyn_ocr		(std::string name, Hex pos, int facing);
Ship make_wyn_odr		(std::string name, Hex pos, int facing);
Ship make_wyn_olr		(std::string name, Hex pos, int facing);
Ship make_wyn_pbb		(std::string name, Hex pos, int facing);
Ship make_wyn_zff		(std::string name, Hex pos, int facing);

