#pragma once
#include <cstdint>
#include <string>
#include <algorithm>

static constexpr uint8_t ARC_ALL   = 0b00111111; // 360°
static constexpr uint8_t ARC_FWD   = 0b00100011; // 180° forward  (sextants 5,0,1)
static constexpr uint8_t ARC_BROAD = 0b00110111; // 240° forward  (sextants 4,5,0,1,2)
static constexpr uint8_t ARC_REAR  = 0b00011100; // 180° aft      (sextants 2,3,4)

enum class WeaponType {
    Phaser1, Phaser2, Phaser3,
    PhotonTorpedo,
    Disruptor,
    PlasmaF,       // Gorn CA: 2-turn, fwd, 20−2×range (max range 10)
    PlasmaR,       // Romulan: 3-turn, fwd, 30−2×range
    PlasmaG,       // Gorn BC: 2-turn, fwd, 10−range
    Drone,         // Kzinti: 1-turn, 360°, 8 flat at ≤20
    Hellbore,      // Hydran: 1-turn, fwd, 12 at ≤5 / 8 at ≤10
    GatlingPhaser, // Hydran: instant, high-power Ph-1 curve
    FusionBeam,    // Lyran: instant, close range, 10 at ≤3 / 5 at ≤6
    ESG,           // Lyran: Expanding Sphere Generator — area, instant, damages all ships in radius
    Fighter,       // Hydran: launched strikecraft — seeking, 6 damage on hit
};

inline const char* weapon_name(WeaponType t) {
    switch (t) {
        case WeaponType::Phaser1:       return "Ph-1";
        case WeaponType::Phaser2:       return "Ph-2";
        case WeaponType::Phaser3:       return "Ph-3";
        case WeaponType::PhotonTorpedo: return "Photon";
        case WeaponType::Disruptor:     return "Disruptor";
        case WeaponType::PlasmaF:       return "Plasma-F";
        case WeaponType::PlasmaR:       return "Plasma-R";
        case WeaponType::PlasmaG:       return "Plasma-G";
        case WeaponType::Drone:         return "Drone";
        case WeaponType::Hellbore:      return "Hellbore";
        case WeaponType::GatlingPhaser: return "Gatling";
        case WeaponType::FusionBeam:    return "Fusion";
        case WeaponType::ESG:           return "ESG";
        case WeaponType::Fighter:       return "Fighter";
    }
    return "";
}

struct Weapon {
    std::string label;
    WeaponType  type;
    uint8_t     arc;
    int max_power;     // phasers: max allocatable power; others: 0 (fixed cost)
    int arming_turns;  // 0 = instant fire
    int arming_cost;   // power per arming turn

    int  allocated = 0;
    int  charge    = 0;
    bool armed     = false;
    bool fired     = false;
    bool disabled  = false;  // knocked out by internal damage (DAC)
    int  ammo      = -1;     // FD1.1: -1 = unlimited; drone rack = 4 shots
    int  stored    = 0;      // G23.0: ESG accumulated energy (carries across turns)

    bool is_instant() const { return arming_turns == 0; }
    bool is_ready()   const { return armed || is_instant(); }
    bool can_fire()   const {
        return !disabled && is_ready() && !fired
            && (allocated > 0 || !is_instant())
            && (ammo != 0);  // FD1.1: 0 = rack empty
    }

    int phaser_damage(int range) const {
        float d = (float)allocated;
        if (range <= 3)  return (int)d;
        if (range <= 8)  return (int)(d * 0.5f);
        if (range <= 15) return (int)(d * 0.25f);
        return 0;
    }

    // Expected/planning damage (deterministic, used by AI and range-check).
    // For actual fire resolution use roll_damage().
    int damage_at(int range) const {
        switch (type) {
            case WeaponType::Phaser1:
            case WeaponType::GatlingPhaser: return phaser_damage(range);
            case WeaponType::Phaser2: {
                int pd = phaser_damage(range);
                return pd > 0 ? pd / 2 + 1 : 0;
            }
            case WeaponType::Phaser3:       return (range <= 2) ? allocated : 0;
            // E4.0: min range 2, max range 30, flat 8 expected value
            case WeaponType::PhotonTorpedo: return (range >= 2 && range <= 30) ? 8 : 0;
            // E3.0: instant 2-pt bolt, expected damage by range bracket
            case WeaponType::Disruptor:     return range <= 8 ? 6 : (range <= 15 ? 3 : 0);
            case WeaponType::PlasmaF:       return std::max(0, 20 - range * 2);
            case WeaponType::PlasmaR:       return std::max(0, 30 - range * 2);
            case WeaponType::PlasmaG:       return std::max(0, 10 - range);
            case WeaponType::Drone:         return range <= 20 ? 6 : 0;  // FD1.0 Cadet = 6 pts
            case WeaponType::Hellbore:      return range <= 5 ? 12 : (range <= 10 ? 8 : 0);
            case WeaponType::FusionBeam:    return range <= 3 ? 10 : (range <= 6 ? 5 : 0);
            // G23.0: ESG strength = (5 - radius) * energy_stored; max radius 3
            case WeaponType::ESG: {
                int total = stored + allocated;
                return range <= 3 ? std::max(0, (5 - range) * total) : 0;
            }
            case WeaponType::Fighter:       return 6;
        }
        return 0;
    }

    // Dice-based fire resolution (die = 1-6, caller supplies rand).
    // Returns actual damage for this shot; 0 = miss.
    int roll_damage(int range, int die) const {
        switch (type) {

        case WeaponType::Phaser1:
        case WeaponType::GatlingPhaser: {
            // C3.31 Ph-1 table at 10-pt allocation; scale by actual
            static const int tbl[5][6] = {
            //  d1   d2   d3   d4   d5   d6   (range row)
                {10,  9,   8,   7,   6,   5},  // r 1-3
                { 9,  7,   5,   4,   3,   1},  // r 4-8
                { 5,  4,   3,   2,   1,   0},  // r 9-15
                { 3,  2,   1,   0,   0,   0},  // r 16-24
                { 0,  0,   0,   0,   0,   0},  // r 25+
            };
            int row = (range <= 3)  ? 0
                    : (range <= 8)  ? 1
                    : (range <= 15) ? 2
                    : (range <= 24) ? 3 : 4;
            int base = tbl[row][die - 1];
            if (allocated >= 10) return base;
            return (base * allocated + 9) / 10;
        }

        case WeaponType::Phaser2: {
            // Ph-2 table at 5-pt allocation; weaker than Ph-1
            static const int tbl[4][6] = {
                { 5,  4,   4,   3,   2,   1},  // r 1-3
                { 3,  3,   2,   1,   0,   0},  // r 4-8
                { 2,  1,   0,   0,   0,   0},  // r 9-15
                { 0,  0,   0,   0,   0,   0},  // r 16+
            };
            int row = (range <= 3)  ? 0
                    : (range <= 8)  ? 1
                    : (range <= 15) ? 2 : 3;
            int base = tbl[row][die - 1];
            if (allocated >= 5) return base;
            return (base * allocated + 4) / 5;
        }

        case WeaponType::Phaser3:
            // Ph-3: point defence, max range 2, no miss table
            return (range <= 2) ? allocated : 0;

        case WeaponType::PhotonTorpedo: {
            // E4.0: hit if die <= hit_number for range bracket
            if (range < 2 || range > 30) return 0;
            int hit_num = (range <=  8) ? 4
                        : (range <= 15) ? 3
                        : (range <= 25) ? 2 : 1;
            return (die <= hit_num) ? 8 : 0;
        }

        case WeaponType::Disruptor: {
            // E3.0: d6 vs range table at standard 2-pt allocation
            static const int tbl[2][6] = {
                { 9,  8,   6,   5,   4,   1},  // r 1-8  (avg 5.5)
                { 5,  4,   3,   2,   1,   0},  // r 9-15 (avg 2.5)
            };
            if (range > 15) return 0;
            return tbl[(range <= 8) ? 0 : 1][die - 1];
        }

        default:
            return damage_at(range);  // non-direct-fire weapons: use deterministic value
        }
    }

    int eaf_cost() const {
        if (is_instant()) return allocated;
        if (armed)        return 0;
        return (charge < arming_turns) ? arming_cost : 0;
    }
};

inline Weapon make_ph1(std::string lbl)       { return {std::move(lbl), WeaponType::Phaser1,       ARC_BROAD, 10, 0, 0}; }
inline Weapon make_ph2(std::string lbl)       { return {std::move(lbl), WeaponType::Phaser2,       ARC_BROAD,  5, 0, 0}; }
inline Weapon make_ph3(std::string lbl)       { return {std::move(lbl), WeaponType::Phaser3,       ARC_ALL,    2, 0, 0}; }
inline Weapon make_photon(std::string lbl)    { return {std::move(lbl), WeaponType::PhotonTorpedo, ARC_FWD,    0, 2, 2}; }
inline Weapon make_disruptor(std::string lbl) { return {std::move(lbl), WeaponType::Disruptor,     ARC_BROAD,  2, 0, 0}; }
inline Weapon make_plasma_f(std::string lbl)  { return {std::move(lbl), WeaponType::PlasmaF,       ARC_FWD,    0, 2, 4}; }
inline Weapon make_plasma_r(std::string lbl)  { return {std::move(lbl), WeaponType::PlasmaR,       ARC_FWD,    0, 3, 6}; }
inline Weapon make_plasma_g(std::string lbl)  { return {std::move(lbl), WeaponType::PlasmaG,       ARC_FWD,    0, 2, 5}; }
inline Weapon make_drone(std::string lbl) {
    Weapon w{std::move(lbl), WeaponType::Drone, ARC_ALL, 0, 1, 2};
    w.ammo = 4;  // FD1.1: one rack = 4 drones
    return w;
}
inline Weapon make_hellbore(std::string lbl)  { return {std::move(lbl), WeaponType::Hellbore,      ARC_FWD,    0, 1, 4}; }
inline Weapon make_gatling(std::string lbl)   { return {std::move(lbl), WeaponType::GatlingPhaser, ARC_BROAD, 15, 0, 0}; }
inline Weapon make_fusion(std::string lbl)    { return {std::move(lbl), WeaponType::FusionBeam,    ARC_FWD,    0, 1, 3}; }
// ESG: max_power=8 (allocated per turn), instant (no arming turns), ARC_ALL (sphere)
inline Weapon make_esg(std::string lbl)       { return {std::move(lbl), WeaponType::ESG,           ARC_ALL,    8, 0, 0}; }
// Fighter: 1-turn arm, costs 2 power to launch, seeks any target (R9.F)
inline Weapon make_fighter(std::string lbl)   { return {std::move(lbl), WeaponType::Fighter,       ARC_ALL,    0, 1, 2}; }
