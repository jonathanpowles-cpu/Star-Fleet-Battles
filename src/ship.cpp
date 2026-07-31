#include "ship.h"

// Helper: builds a DAC vector from a compact string.
//   W=WarpEngine  I=ImpulseEngine  0-5=Shield0-5  X=Weapon  L=Lab  A=Auxiliary
//   P=APR  B=Bridge
static std::vector<DACEntry> parse_dac(const char* s) {
    std::vector<DACEntry> v;
    for (; *s; ++s) {
        switch (*s) {
        case 'W': v.push_back(DACEntry::WarpEngine);    break;
        case 'I': v.push_back(DACEntry::ImpulseEngine); break;
        case '0': v.push_back(DACEntry::Shield0);       break;
        case '1': v.push_back(DACEntry::Shield1);       break;
        case '2': v.push_back(DACEntry::Shield2);       break;
        case '3': v.push_back(DACEntry::Shield3);       break;
        case '4': v.push_back(DACEntry::Shield4);       break;
        case '5': v.push_back(DACEntry::Shield5);       break;
        case 'X': v.push_back(DACEntry::Weapon);        break;
        case 'L': v.push_back(DACEntry::Lab);           break;
        case 'P': v.push_back(DACEntry::APR);           break;
        case 'B': v.push_back(DACEntry::Bridge);        break;
        default:  v.push_back(DACEntry::Auxiliary);     break; // 'A' or space
        }
    }
    return v;
}

// ── Federation CA ─────────────────────────────────────────────────────────────
// Balanced explorer-cruiser; phaser + photon battery; 4 warp engines
// hull 100 → 20 DAC entries
Ship make_federation_ca(std::string name, Hex pos, int facing) {
    Ship s;
    s.name     = std::move(name);
    s.faction  = Faction::Federation;
    s.position = pos;
    s.facing   = facing;

    s.sys.max_warp_power    = 24;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power        = 24;
    s.eaf.impulse_power     = 4;

    s.sys.warp_groups    = {{"Warp-1",6,6},{"Warp-2",6,6},{"Warp-3",6,6},{"Warp-4",6,6}};
    s.sys.impulse_groups = {{"Impulse",4,4}};

    s.sys.apr_rated = s.sys.apr_current = 4;
    s.sys.battery_cap   = 10;
    s.sys.battery_charge = 0;

    s.sys.crew_total    = 8;
    s.sys.shuttles      = 2;
    s.sys.pcap_rated    = 10;
    s.sys.pcap_charge   = 10;

    s.sys.shields_max[0] = s.sys.shields[0] = 30;
    s.sys.shields_max[1] = s.sys.shields[1] = 25;
    s.sys.shields_max[2] = s.sys.shields[2] = 20;
    s.sys.shields_max[3] = s.sys.shields[3] = 20;
    s.sys.shields_max[4] = s.sys.shields[4] = 20;
    s.sys.shields_max[5] = s.sys.shields[5] = 25;
    s.sys.hull = s.sys.hull_max = 100;

    // P=APR hit on 19th entry (95 penetrating hull damage)
    s.dac = parse_dac("AW0WW1WI2WX3W4I5WXP");  // 20 entries

    s.weapons = {
        make_ph1("Ph-1 #1"),
        make_ph1("Ph-1 #2"),
        make_ph1("Ph-1 #3"),
        make_ph1("Ph-1 #4"),
        make_ph1("Ph-1 #5"),
        make_ph1("Ph-1 #6"),
        make_ph1("Ph-1 #7"),
        make_ph1("Ph-1 #8"),
        make_ph3("Ph-3 #9"),
        make_ph3("Ph-3 #10"),
        make_photon("Phot (A)"),
        make_photon("Phot (B)"),
        make_drone("Drone #1"),
    };
    // Aft-facing phasers: Ph-1 #5..#8 cover rear arc (D3.2 SSD arcs)
    for (int i = 4; i <= 7; ++i) s.weapons[i].arc = 0x1C; // ARC_REAR
    s.turn_mode_cat = 4;  // D: heavy cruiser

    return s;
}

// ── Klingon D7 ────────────────────────────────────────────────────────────────
// Aggressive strike cruiser; 4 warp engines + disruptors
// hull 90 → 18 DAC entries
Ship make_klingon_d7(std::string name, Hex pos, int facing) {
    Ship s;
    s.name     = std::move(name);
    s.faction  = Faction::Klingon;
    s.position = pos;
    s.facing   = facing;

    s.sys.max_warp_power    = 24;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power        = 24;
    s.eaf.impulse_power     = 4;

    s.sys.warp_groups    = {{"Warp-1",6,6},{"Warp-2",6,6},{"Warp-3",6,6},{"Warp-4",6,6}};
    s.sys.impulse_groups = {{"Impulse",4,4}};

    s.sys.apr_rated = s.sys.apr_current = 4;
    s.sys.battery_cap   = 8;
    s.sys.battery_charge = 0;

    s.sys.crew_total = 8;
    s.sys.shuttles   = 2;

    s.sys.shields_max[0] = s.sys.shields[0] = 28;
    s.sys.shields_max[1] = s.sys.shields[1] = 22;
    s.sys.shields_max[2] = s.sys.shields[2] = 18;
    s.sys.shields_max[3] = s.sys.shields[3] = 18;
    s.sys.shields_max[4] = s.sys.shields[4] = 18;
    s.sys.shields_max[5] = s.sys.shields[5] = 22;
    s.sys.hull = s.sys.hull_max = 90;

    s.dac = parse_dac("W0AWW1WI2XW3W4IW5X");  // 18 entries

    s.weapons = {
        make_ph2("Ph-2K #1"),
        make_ph2("Ph-2K #2"),
        make_ph2("Ph-2K #3"),
        make_ph2("Ph-2 #4"),
        make_ph2("Ph-2 #5"),
        make_ph2("Ph-2 #6"),
        make_ph2("Ph-2 #7"),
        make_disruptor("Dis (PF)"),
        make_disruptor("Dis (SF)"),
        make_disruptor("Dis (PA)"),
        make_disruptor("Dis (SA)"),
        make_drone("Drone #1"),
        make_drone("Drone #2"),
    };
    // Aft disruptors: Port-Aft and Starboard-Aft face rear (indices 9,10)
    s.weapons[9].arc = 0x1C;  // ARC_REAR
    s.weapons[10].arc = 0x1C; // ARC_REAR
    s.turn_mode_cat = 4;  // D: D7 heavy cruiser

    return s;
}

// ── Romulan KR ────────────────────────────────────────────────────────────────
// Cloaking Bird-of-Prey; plasma torpedoes; cloak costs 8 PW
// hull 90 → 18 DAC entries
Ship make_romulan_kr(std::string name, Hex pos, int facing) {
    Ship s;
    s.name     = std::move(name);
    s.faction  = Faction::Romulan;
    s.position = pos;
    s.facing   = facing;

    s.sys.max_warp_power    = 22;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power        = 22;
    s.eaf.impulse_power     = 4;

    s.sys.warp_groups    = {{"Warp-1",6,6},{"Warp-2",6,6},{"Warp-3",5,5},{"Warp-4",5,5}};
    s.sys.impulse_groups = {{"Impulse",4,4}};

    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap   = 6;
    s.sys.battery_charge = 0;

    s.sys.crew_total = 8;
    s.sys.shuttles   = 2;

    s.sys.cloak_installed = true;
    s.sys.cloak_cost      = 8;

    s.sys.shields_max[0] = s.sys.shields[0] = 25;
    s.sys.shields_max[1] = s.sys.shields[1] = 22;
    s.sys.shields_max[2] = s.sys.shields[2] = 18;
    s.sys.shields_max[3] = s.sys.shields[3] = 18;
    s.sys.shields_max[4] = s.sys.shields[4] = 18;
    s.sys.shields_max[5] = s.sys.shields[5] = 22;
    s.sys.hull = s.sys.hull_max = 90;

    // B=Bridge hit on 1st entry; otherwise same layout
    s.dac = parse_dac("BW0WA1WIW2XW3W4I5XW");  // 18 entries

    s.weapons = {
        make_ph1("Ph-1 #1"),
        make_ph1("Ph-1 #2"),
        make_ph1("Ph-1 #3"),
        make_ph2("Ph-2 #4"),
        make_ph2("Ph-2 #5"),
        make_plasma_r("Plas-R (A)"),
        make_plasma_r("Plas-R (B)"),
    };
    // FP3.0: KR swivel mounts
    for (auto& w : s.weapons) {
        if (w.label == "Plas-R (A)") w.arc = 0x31; // LS: sextants 4,5,0
        if (w.label == "Plas-R (B)") w.arc = 0x07; // RS: sextants 0,1,2
    }
    s.turn_mode_cat = 3;  // C: KR bird of prey

    return s;
}

// ── Gorn CA ──────────────────────────────────────────────────────────────────
// Slow armoured cruiser; heavy plasma-G torpedoes; 3 warp engines
// hull 95 → 19 DAC entries
Ship make_gorn_ca(std::string name, Hex pos, int facing) {
    Ship s;
    s.name     = std::move(name);
    s.faction  = Faction::Gorn;
    s.position = pos;
    s.facing   = facing;

    s.sys.max_warp_power    = 18;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power        = 18;
    s.eaf.impulse_power     = 4;

    s.sys.warp_groups    = {{"Warp-1",6,6},{"Warp-2",6,6},{"Warp-3",6,6}};
    s.sys.impulse_groups = {{"Impulse",4,4}};

    s.sys.apr_rated = s.sys.apr_current = 4;
    s.sys.battery_cap   = 8;
    s.sys.battery_charge = 0;

    s.sys.crew_total = 8;
    s.sys.shuttles   = 2;

    s.sys.shields_max[0] = s.sys.shields[0] = 32;
    s.sys.shields_max[1] = s.sys.shields[1] = 28;
    s.sys.shields_max[2] = s.sys.shields[2] = 22;
    s.sys.shields_max[3] = s.sys.shields[3] = 22;
    s.sys.shields_max[4] = s.sys.shields[4] = 22;
    s.sys.shields_max[5] = s.sys.shields[5] = 28;
    s.sys.hull = s.sys.hull_max = 95;

    s.dac = parse_dac("A0WW1BI2WW3X4IW5XW");   // 19 entries (B=Bridge hit)

    s.weapons = {
        make_ph1("Ph-1 #1"),
        make_ph1("Ph-1 #2"),
        make_ph1("Ph-1 #3"),
        make_ph1("Ph-1 #4"),
        make_ph1("Ph-1 #5"),
        make_ph1("Ph-1 #6"),
        make_ph1("Ph-1 #7"),
        make_ph1("Ph-1 #8"),
        make_ph3("Ph-3 #9"),
        make_ph3("Ph-3 #10"),
        make_plasma_f("Plas-F (A)"),
        make_plasma_f("Plas-F (B)"),
    };
    // Aft phasers: Ph-1 #5..#8 cover rear arc
    for (int i = 4; i <= 7; ++i) s.weapons[i].arc = 0x1C; // ARC_REAR
    s.turn_mode_cat = 5;  // E: Gorn CA ponderous

    return s;
}

// ── Kzinti CS (Strike Cruiser, R5.2) ─────────────────────────────────────────
// Drone-heavy cruiser; 4 warp engines + disruptors; hull 90
// hull 90 → 18 DAC entries
Ship make_kzinti_cw(std::string name, Hex pos, int facing) {
    Ship s;
    s.name     = std::move(name);
    s.faction  = Faction::Kzinti;
    s.position = pos;
    s.facing   = facing;

    s.sys.max_warp_power    = 24;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power        = 24;
    s.eaf.impulse_power     = 4;

    s.sys.warp_groups    = {{"Warp-1",6,6},{"Warp-2",6,6},{"Warp-3",6,6},{"Warp-4",6,6}};
    s.sys.impulse_groups = {{"Impulse",4,4}};

    s.sys.apr_rated = s.sys.apr_current = 4;
    s.sys.battery_cap   = 10;
    s.sys.battery_charge = 0;

    s.sys.crew_total = 8;
    s.sys.shuttles   = 2;

    s.sys.shields_max[0] = s.sys.shields[0] = 28;
    s.sys.shields_max[1] = s.sys.shields[1] = 26;
    s.sys.shields_max[2] = s.sys.shields[2] = 24;
    s.sys.shields_max[3] = s.sys.shields[3] = 28;
    s.sys.shields_max[4] = s.sys.shields[4] = 24;
    s.sys.shields_max[5] = s.sys.shields[5] = 26;
    s.sys.hull = s.sys.hull_max = 90;

    s.dac = parse_dac("W0AWW1WI2XW3W4IW5X");  // 18 entries

    s.weapons = {
        make_ph1("Ph-1 #1"),
        make_ph1("Ph-1 #2"),
        make_ph3("Ph-3 #3"),
        make_ph3("Ph-3 #4"),
        make_ph3("Ph-3 #5"),
        make_ph3("Ph-3 #6"),
        make_ph3("Ph-3 #7"),
        make_ph3("Ph-3 #8"),
        make_ph3("Ph-3 #9"),
        make_ph3("Ph-3 #10"),
        make_disruptor("Dis #1"),
        make_disruptor("Dis #2"),
        make_drone("Drone #1"),
        make_drone("Drone #2"),
        make_drone("Drone #3"),
        make_drone("Drone #4"),
    };    s.turn_mode_cat = 3; // C: Kzinti war cruiser

    return s;
}
