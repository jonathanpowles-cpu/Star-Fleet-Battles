#include "ship.h"

// parse_dac local copy (static in ship.cpp, not exported)
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
        default:  v.push_back(DACEntry::Auxiliary);     break;
        }
    }
    return v;
}

// Scaled DAC from template; hull must be multiple of 5
static std::vector<DACEntry> dac_for_hull(int hull) {
    // First 18 chars = verified D7 pattern; remainder escalates
    static const char T[] =
        "W0AWW1WI2XW3W4IW5X"   // entries 1-18  (hull 5-90)
        "WW0WW1WW2WW3WW4WW5"   // entries 19-36 (hull 95-180)
        "WW0WW1WW2WW3WW4WW5"   // entries 37-54 (hull 185-270)
        "WWWWWW";               // entries 55-60 (hull 275-300)
    int n = hull / 5;
    std::string s;
    for (int i = 0; i < n; ++i)
        s += (i < (int)(sizeof(T) - 1)) ? T[i] : 'W';
    return parse_dac(s.c_str());
}

// ── AxCVL (R1.13) ─────────────────────────────────────────────────────────────
Ship make_axcvl(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 16;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 16; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"R.Warp",8,8},{"L.Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 3; s.sys.shuttles = 4;
    s.sys.shields_max[0] = s.sys.shields[0] = 14;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 10;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 10;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 60;
    s.dac = dac_for_hull(60);
    s.sys.fighter_bay = 6;
    s.weapons.push_back(make_fighter("Fighter-1"));
    s.weapons.push_back(make_fighter("Fighter-2"));
    s.weapons.push_back(make_fighter("Fighter-3"));
    s.weapons.push_back(make_fighter("Fighter-4"));
    s.weapons.push_back(make_fighter("Fighter-5"));
    s.weapons.push_back(make_fighter("Fighter-6"));
    s.fighters_aboard = 6;    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// ── Small Freighter (R1.5) ────────────────────────────────────────────────────
Ship make_small_freighter(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 4;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 4; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"Warp",4,4}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 0;
    s.sys.battery_cap = 0;
    s.sys.crew_total = 1; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 4;
    s.sys.shields_max[1] = s.sys.shields[1] = 4;
    s.sys.shields_max[2] = s.sys.shields[2] = 4;
    s.sys.shields_max[3] = s.sys.shields[3] = 4;
    s.sys.shields_max[4] = s.sys.shields[4] = 4;
    s.sys.shields_max[5] = s.sys.shields[5] = 4;
    s.sys.hull = s.sys.hull_max = 15;
    s.dac = dac_for_hull(15);
    s.weapons = { make_ph3("Ph-3") };    s.assign_weapon_arcs();
    s.turn_mode_cat = 5;  // C3.31

    return s;
}

// ── Large Freighter (R1.6) ────────────────────────────────────────────────────
Ship make_large_freighter(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 6;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 6; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"Warp",6,6}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 2; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 6;
    s.sys.shields_max[1] = s.sys.shields[1] = 10;
    s.sys.shields_max[2] = s.sys.shields[2] = 10;
    s.sys.shields_max[3] = s.sys.shields[3] = 6;
    s.sys.shields_max[4] = s.sys.shields[4] = 6;
    s.sys.shields_max[5] = s.sys.shields[5] = 6;
    s.sys.hull = s.sys.hull_max = 30;
    s.dac = dac_for_hull(30);
    s.weapons = { make_ph2("Ph-2") };    s.assign_weapon_arcs();
    s.turn_mode_cat = 6;  // C3.31

    return s;
}

// ── Federation Large Q-Ship (R1.7) ────────────────────────────────────────────
Ship make_fed_lq(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 16;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 16; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"Warp-1",8,8},{"Warp-2",8,8}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 4;
    s.sys.battery_cap = 4; s.sys.pcap_rated = s.sys.pcap_charge = 4;
    s.sys.crew_total = 10; s.sys.shuttles = 4;
    s.sys.shields_max[0] = s.sys.shields[0] = 16;
    s.sys.shields_max[1] = s.sys.shields[1] = 20;
    s.sys.shields_max[2] = s.sys.shields[2] = 20;
    s.sys.shields_max[3] = s.sys.shields[3] = 16;
    s.sys.shields_max[4] = s.sys.shields[4] = 20;
    s.sys.shields_max[5] = s.sys.shields[5] = 20;
    s.sys.hull = s.sys.hull_max = 80;
    s.dac = dac_for_hull(80);
    s.weapons = {
        make_photon("Phot-FA"),
        make_ph1("Ph-1 #1"), make_ph1("Ph-1 #2"),
        make_ph3("Ph-3 RA"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// ── Federation Small Q-Ship (R1.7) ────────────────────────────────────────────
Ship make_fed_sq(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 12;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 12; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"Warp-1",6,6},{"Warp-2",6,6}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 4;
    s.sys.battery_cap = 4; s.sys.pcap_rated = s.sys.pcap_charge = 4;
    s.sys.crew_total = 5; s.sys.shuttles = 4;
    s.sys.shields_max[0] = s.sys.shields[0] = 12;
    s.sys.shields_max[1] = s.sys.shields[1] = 16;
    s.sys.shields_max[2] = s.sys.shields[2] = 16;
    s.sys.shields_max[3] = s.sys.shields[3] = 12;
    s.sys.shields_max[4] = s.sys.shields[4] = 16;
    s.sys.shields_max[5] = s.sys.shields[5] = 16;
    s.sys.hull = s.sys.hull_max = 60;
    s.dac = dac_for_hull(60);
    s.weapons = {
        make_ph1("Ph-1 #1"), make_ph1("Ph-1 #2"),
        make_ph3("Ph-3"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// ── Klingon Large Q-Ship (R1.7) ───────────────────────────────────────────────
Ship make_klingon_lq(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Klingon;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 16;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 16; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"Warp-1",8,8},{"Warp-2",8,8}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 4;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 10; s.sys.shuttles = 4;
    s.sys.shields_max[0] = s.sys.shields[0] = 16;
    s.sys.shields_max[1] = s.sys.shields[1] = 20;
    s.sys.shields_max[2] = s.sys.shields[2] = 20;
    s.sys.shields_max[3] = s.sys.shields[3] = 16;
    s.sys.shields_max[4] = s.sys.shields[4] = 20;
    s.sys.shields_max[5] = s.sys.shields[5] = 20;
    s.sys.hull = s.sys.hull_max = 80;
    s.dac = dac_for_hull(80);
    s.weapons = {
        make_ph2("Ph-2 #1"), make_ph2("Ph-2 #2"),
        make_disruptor("Dis #1"), make_disruptor("Dis #2"),
        make_drone("Drone #1"), make_drone("Drone #2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// ── Klingon Small Q-Ship (R1.7) ───────────────────────────────────────────────
Ship make_klingon_sq(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Klingon;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 12;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 12; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"Warp-1",6,6},{"Warp-2",6,6}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 4;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 5; s.sys.shuttles = 4;
    s.sys.shields_max[0] = s.sys.shields[0] = 12;
    s.sys.shields_max[1] = s.sys.shields[1] = 16;
    s.sys.shields_max[2] = s.sys.shields[2] = 16;
    s.sys.shields_max[3] = s.sys.shields[3] = 12;
    s.sys.shields_max[4] = s.sys.shields[4] = 16;
    s.sys.shields_max[5] = s.sys.shields[5] = 16;
    s.sys.hull = s.sys.hull_max = 60;
    s.dac = dac_for_hull(60);
    s.weapons = {
        make_ph2("Ph-2 #1"), make_ph2("Ph-2 #2"),
        make_disruptor("Dis"),
        make_drone("Drone"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// ── Gorn Large Q-Ship (R1.7) ──────────────────────────────────────────────────
Ship make_gorn_lq(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Gorn;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 16;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 16; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"Warp-1",8,8},{"Warp-2",8,8}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 4;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 10; s.sys.shuttles = 4;
    s.sys.shields_max[0] = s.sys.shields[0] = 16;
    s.sys.shields_max[1] = s.sys.shields[1] = 22;
    s.sys.shields_max[2] = s.sys.shields[2] = 22;
    s.sys.shields_max[3] = s.sys.shields[3] = 16;
    s.sys.shields_max[4] = s.sys.shields[4] = 22;
    s.sys.shields_max[5] = s.sys.shields[5] = 22;
    s.sys.hull = s.sys.hull_max = 80;
    s.dac = dac_for_hull(80);
    s.weapons = {
        make_ph1("Ph-1 FA"),
        make_plasma_f("Plas-F A"), make_plasma_f("Plas-F B"),
        make_ph1("Ph-1 LS"), make_ph1("Ph-1 LS2"), make_ph1("Ph-1 LS3"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// ── Gorn Small Q-Ship (R1.7) ──────────────────────────────────────────────────
Ship make_gorn_sq(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Gorn;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 12;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 12; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"Warp-1",6,6},{"Warp-2",6,6}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 4;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 5; s.sys.shuttles = 4;
    s.sys.shields_max[0] = s.sys.shields[0] = 12;
    s.sys.shields_max[1] = s.sys.shields[1] = 16;
    s.sys.shields_max[2] = s.sys.shields[2] = 16;
    s.sys.shields_max[3] = s.sys.shields[3] = 12;
    s.sys.shields_max[4] = s.sys.shields[4] = 16;
    s.sys.shields_max[5] = s.sys.shields[5] = 16;
    s.sys.hull = s.sys.hull_max = 60;
    s.dac = dac_for_hull(60);
    s.weapons = {
        make_ph1("Ph-1 #1"), make_ph1("Ph-1 #2"),
        make_plasma_f("Plas-F"),
        make_ph3("Ph-3"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// ── Federation Dreadnought (R2.2) ─────────────────────────────────────────────
Ship make_federation_dn(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 32;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 32; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"Warp-1",8,8},{"Warp-2",8,8},{"Warp-3",8,8},{"Warp-4",8,8}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 4;
    s.sys.battery_cap = 6; s.sys.pcap_rated = s.sys.pcap_charge = 8;
    s.sys.crew_total = 10; s.sys.shuttles = 5;
    s.sys.shields_max[0] = s.sys.shields[0] = 36;
    s.sys.shields_max[1] = s.sys.shields[1] = 28;
    s.sys.shields_max[2] = s.sys.shields[2] = 24;
    s.sys.shields_max[3] = s.sys.shields[3] = 28;
    s.sys.shields_max[4] = s.sys.shields[4] = 24;
    s.sys.shields_max[5] = s.sys.shields[5] = 28;
    s.sys.hull = s.sys.hull_max = 120;
    s.dac = dac_for_hull(120);
    s.weapons = {
        make_photon("Phot A"), make_photon("Phot B"),
        make_photon("Phot C"), make_photon("Phot D"),
        make_ph1("Ph-1 LF"),  make_ph1("Ph-1 L"),
        make_ph1("Ph-1 RF"),  make_ph1("Ph-1 5"),
        make_ph1("Ph-1 6"),   make_ph1("Ph-1 RA 9"),
        make_ph1("Ph-1 RA 10"),
        make_ph3("Ph-3 7"),   make_ph3("Ph-3 8"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 5;  // C3.31

    return s;
}

// ── Federation Command Cruiser (R2.3) ─────────────────────────────────────────
Ship make_federation_cc(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 24;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 24; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"Warp-1",6,6},{"Warp-2",6,6},{"Warp-3",6,6},{"Warp-4",6,6}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 4;
    s.sys.battery_cap = 6; s.sys.pcap_rated = s.sys.pcap_charge = 6;
    s.sys.crew_total = 10; s.sys.shuttles = 5;
    s.sys.shields_max[0] = s.sys.shields[0] = 30;
    s.sys.shields_max[1] = s.sys.shields[1] = 24;
    s.sys.shields_max[2] = s.sys.shields[2] = 24;
    s.sys.shields_max[3] = s.sys.shields[3] = 20;
    s.sys.shields_max[4] = s.sys.shields[4] = 24;
    s.sys.shields_max[5] = s.sys.shields[5] = 24;
    s.sys.hull = s.sys.hull_max = 100;
    s.dac = dac_for_hull(100);
    s.weapons = {
        make_photon("Phot A"), make_photon("Phot B"), make_photon("Phot C"),
        make_ph1("Ph-1 5"),  make_ph1("Ph-1 6"),
        make_ph1("Ph-1 LF"), make_ph1("Ph-1 L"),
        make_ph1("Ph-1 LF2"),
        make_ph3("Ph-3 7"),  make_ph3("Ph-3 8"),
        make_ph3("Ph-3 9"),  make_ph3("Ph-3 10"),
        make_drone("Drone"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// ── Federation Light Cruiser (R2.5) ───────────────────────────────────────────
Ship make_federation_cl(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 20;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 20; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",5,5},{"L.Warp2",5,5},{"R.Warp",5,5},{"R.Warp2",5,5}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 2; s.sys.pcap_rated = s.sys.pcap_charge = 4;
    s.sys.crew_total = 8; s.sys.shuttles = 5;
    s.sys.shields_max[0] = s.sys.shields[0] = 20;
    s.sys.shields_max[1] = s.sys.shields[1] = 16;
    s.sys.shields_max[2] = s.sys.shields[2] = 14;
    s.sys.shields_max[3] = s.sys.shields[3] = 14;
    s.sys.shields_max[4] = s.sys.shields[4] = 14;
    s.sys.shields_max[5] = s.sys.shields[5] = 16;
    s.sys.hull = s.sys.hull_max = 80;
    s.dac = dac_for_hull(80);
    s.weapons = {
        make_photon("Phot A"), make_photon("Phot B"),
        make_ph1("Ph-1 #1"), make_ph1("Ph-1 #2"),
        make_ph1("Ph-1 3"),  make_ph1("Ph-1 4"),  make_ph1("Ph-1 5"),
        make_ph3("Ph-3 6"),  make_ph3("Ph-3 7"),  make_ph3("Ph-3 8"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 3;  // C3.31

    return s;
}

// ── Federation Destroyer (R2.6) ───────────────────────────────────────────────
Ship make_federation_dd(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 16;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 16; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"C.Warp-1",8,8},{"C.Warp-2",8,8}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 2; s.sys.pcap_rated = s.sys.pcap_charge = 4;
    s.sys.crew_total = 6; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 20;
    s.sys.shields_max[1] = s.sys.shields[1] = 20;
    s.sys.shields_max[2] = s.sys.shields[2] = 20;
    s.sys.shields_max[3] = s.sys.shields[3] = 18;
    s.sys.shields_max[4] = s.sys.shields[4] = 20;
    s.sys.shields_max[5] = s.sys.shields[5] = 20;
    s.sys.hull = s.sys.hull_max = 60;
    s.dac = dac_for_hull(60);
    s.weapons = {
        make_photon("Phot A"), make_photon("Phot B"), make_photon("Phot C"),
        make_ph1("Ph-1 5"),  make_ph1("Ph-1 6"),
        make_ph1("Ph-1 LF"), make_ph1("Ph-1 LS"),
        make_ph3("Ph-3 5"),  make_ph3("Ph-3 7"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 3;  // C3.31

    return s;
}

// ── Federation Scout (R2.7) ───────────────────────────────────────────────────
Ship make_federation_sc(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 16;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 16; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"C.Warp-1",8,8},{"C.Warp-2",8,8}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 6; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 20;
    s.sys.shields_max[1] = s.sys.shields[1] = 20;
    s.sys.shields_max[2] = s.sys.shields[2] = 20;
    s.sys.shields_max[3] = s.sys.shields[3] = 18;
    s.sys.shields_max[4] = s.sys.shields[4] = 20;
    s.sys.shields_max[5] = s.sys.shields[5] = 20;
    s.sys.hull = s.sys.hull_max = 60;
    s.dac = dac_for_hull(60);
    s.weapons = {
        make_ph3("Ph-3 3"), make_ph3("Ph-3 4"), make_ph3("Ph-3 5"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 2;  // C3.31

    return s;
}

// ── Federation Fleet Tug (R2.8) ───────────────────────────────────────────────
Ship make_federation_tug(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 24;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 24; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",6,6},{"L.Warp2",6,6},{"R.Warp",6,6},{"R.Warp2",6,6}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 10; s.sys.shuttles = 5;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 20;
    s.sys.shields_max[2] = s.sys.shields[2] = 20;
    s.sys.shields_max[3] = s.sys.shields[3] = 16;
    s.sys.shields_max[4] = s.sys.shields[4] = 20;
    s.sys.shields_max[5] = s.sys.shields[5] = 20;
    s.sys.hull = s.sys.hull_max = 50;
    s.dac = dac_for_hull(50);
    s.weapons = {
        make_ph1("Ph-1 #1"), make_ph1("Ph-1 #2"),
        make_ph1("Ph-1 RA"), make_ph1("Ph-1 RS"),
        make_ph3("Ph-3 3"),  make_ph3("Ph-3 5"),
        make_drone("Drone"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// ── Federation Battle Tug (R2.10) ─────────────────────────────────────────────
Ship make_federation_battle_tug(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 24;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 24; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",6,6},{"L.Warp2",6,6},{"R.Warp",6,6},{"R.Warp2",6,6}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 6; s.sys.pcap_rated = s.sys.pcap_charge = 8;
    s.sys.crew_total = 10; s.sys.shuttles = 5;
    s.sys.shields_max[0] = s.sys.shields[0] = 24;
    s.sys.shields_max[1] = s.sys.shields[1] = 22;
    s.sys.shields_max[2] = s.sys.shields[2] = 18;
    s.sys.shields_max[3] = s.sys.shields[3] = 20;
    s.sys.shields_max[4] = s.sys.shields[4] = 18;
    s.sys.shields_max[5] = s.sys.shields[5] = 22;
    s.sys.hull = s.sys.hull_max = 90;
    s.dac = dac_for_hull(90);
    s.weapons = {
        make_ph1("Ph-1 #1"), make_ph1("Ph-1 #2"),
        make_photon("Phot A"), make_photon("Phot B"),
        make_photon("Phot C"), make_photon("Phot D"),
        make_ph3("Ph-3 RA"),  make_ph1("Ph-1 L"),
        make_ph3("Ph-3 LS"),  make_ph3("Ph-3 RS"),
        make_ph3("Ph-3 10"),  make_ph3("Ph-3 11"),
        make_drone("Drone #1"), make_drone("Drone #2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// ── Romulan Warbird Cruiser (R4.2) ────────────────────────────────────────────
Ship make_romulan_wb(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Romulan;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 16;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 16; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"Warp-1",8,8},{"Warp-2",8,8}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 8; s.sys.shuttles = 1;
    s.sys.cloak_installed = true; s.sys.cloak_cost = 8;
    s.sys.shields_max[0] = s.sys.shields[0] = 24;
    s.sys.shields_max[1] = s.sys.shields[1] = 24;
    s.sys.shields_max[2] = s.sys.shields[2] = 24;
    s.sys.shields_max[3] = s.sys.shields[3] = 20;
    s.sys.shields_max[4] = s.sys.shields[4] = 24;
    s.sys.shields_max[5] = s.sys.shields[5] = 24;
    s.sys.hull = s.sys.hull_max = 80;
    s.dac = dac_for_hull(80);
    s.weapons = {
        make_ph1("Ph-1 FA"), make_ph1("Ph-1 L"),
        make_plasma_r("Plas-R FA"), make_plasma_r("Plas-R L"),
        make_ph2("Ph-2 A"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 3;  // C3.31

    return s;
}

// ── Romulan War Eagle Cruiser (R4.3) ──────────────────────────────────────────
Ship make_romulan_we(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Romulan;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 22;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 22; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"R.Warp",6,6},{"R.Warp2",5,5},{"L.Warp",6,6},{"L.Warp2",5,5}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 10; s.sys.shuttles = 1;
    s.sys.cloak_installed = true; s.sys.cloak_cost = 6;  // G13.0: WE = 6, KR = 8
    s.sys.shields_max[0] = s.sys.shields[0] = 28;
    s.sys.shields_max[1] = s.sys.shields[1] = 24;
    s.sys.shields_max[2] = s.sys.shields[2] = 22;
    s.sys.shields_max[3] = s.sys.shields[3] = 22;
    s.sys.shields_max[4] = s.sys.shields[4] = 22;
    s.sys.shields_max[5] = s.sys.shields[5] = 24;
    s.sys.hull = s.sys.hull_max = 100;
    s.dac = dac_for_hull(100);
    s.weapons = {
        make_ph1("Ph-1 FA"), make_ph1("Ph-1 L"),
        make_plasma_r("Plas-R FA"), make_plasma_r("Plas-R L"),
        make_ph3("Ph-3 R RA"), make_ph3("Ph-3 L FA"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 3;  // C3.31

    return s;
}

// ── Romulan K5R Frigate (R4.5) ────────────────────────────────────────────────
Ship make_romulan_k5r(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Romulan;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 8;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 8; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"F.Hull",4,4},{"Rear.Hull",4,4}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 4;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 5; s.sys.shuttles = 1;
    s.sys.cloak_installed = true; s.sys.cloak_cost = 6;
    s.sys.shields_max[0] = s.sys.shields[0] = 16;
    s.sys.shields_max[1] = s.sys.shields[1] = 14;
    s.sys.shields_max[2] = s.sys.shields[2] = 12;
    s.sys.shields_max[3] = s.sys.shields[3] = 12;
    s.sys.shields_max[4] = s.sys.shields[4] = 12;
    s.sys.shields_max[5] = s.sys.shields[5] = 14;
    s.sys.hull = s.sys.hull_max = 50;
    s.dac = dac_for_hull(50);
    s.weapons = {
        make_ph1("Ph-1 FA L"), make_ph1("Ph-1 FA"),
        make_plasma_f("Plas-F LP"), make_plasma_f("Plas-F RP"),
        make_ph2("Ph-2 RX"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 3;  // C3.31

    return s;
}

// ── Battle Station (R1.2) ─────────────────────────────────────────────────────
Ship make_battle_station(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 0;
    s.sys.max_impulse_power = 0;
    s.eaf.warp_power = 0; s.eaf.impulse_power = 0;
    s.sys.apr_rated = s.sys.apr_current = 0;
    s.sys.battery_cap = 0;
    s.sys.crew_total = 8; s.sys.shuttles = 4;
    s.sys.shields_max[0] = s.sys.shields[0] = 24;
    s.sys.shields_max[1] = s.sys.shields[1] = 20;
    s.sys.shields_max[2] = s.sys.shields[2] = 18;
    s.sys.shields_max[3] = s.sys.shields[3] = 20;
    s.sys.shields_max[4] = s.sys.shields[4] = 18;
    s.sys.shields_max[5] = s.sys.shields[5] = 20;
    s.sys.hull = s.sys.hull_max = 80;
    s.dac = dac_for_hull(80);
    s.weapons = {
        make_ph3("Ph-3 FX #1"), make_ph3("Ph-3 FX #2"),
        make_ph1("Ph-4 FX"),    make_ph1("Ph-4"),
        make_ph3("Ph-3 #5"),    make_ph3("Ph-3 #6"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 6;  // C3.31

    return s;
}

// ── Starbase (R1.1) ───────────────────────────────────────────────────────────
Ship make_starbase(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 0;
    s.sys.max_impulse_power = 0;
    s.eaf.warp_power = 0; s.eaf.impulse_power = 0;
    s.sys.apr_rated = s.sys.apr_current = 0;
    s.sys.battery_cap = 0;
    s.sys.crew_total = 30; s.sys.shuttles = 6;
    s.sys.shields_max[0] = s.sys.shields[0] = 60;
    s.sys.shields_max[1] = s.sys.shields[1] = 50;
    s.sys.shields_max[2] = s.sys.shields[2] = 50;
    s.sys.shields_max[3] = s.sys.shields[3] = 60;
    s.sys.shields_max[4] = s.sys.shields[4] = 50;
    s.sys.shields_max[5] = s.sys.shields[5] = 50;
    s.sys.hull = s.sys.hull_max = 300;
    s.dac = dac_for_hull(300);
    s.weapons = {
        make_ph1("Ph-4 W4 #1"), make_ph1("Ph-4 W4 #2"),
        make_ph1("Ph-4 W4 #3"), make_ph1("Ph-4 W4 #4"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 6;  // C3.31

    return s;
}

// ── Base Station (R1.3) ───────────────────────────────────────────────────────
Ship make_base_station(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 0;
    s.sys.max_impulse_power = 0;
    s.eaf.warp_power = 0; s.eaf.impulse_power = 0;
    s.sys.apr_rated = s.sys.apr_current = 0;
    s.sys.battery_cap = 0;
    s.sys.crew_total = 5; s.sys.shuttles = 3;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 14;
    s.sys.shields_max[2] = s.sys.shields[2] = 12;
    s.sys.shields_max[3] = s.sys.shields[3] = 14;
    s.sys.shields_max[4] = s.sys.shields[4] = 12;
    s.sys.shields_max[5] = s.sys.shields[5] = 14;
    s.sys.hull = s.sys.hull_max = 60;
    s.dac = dac_for_hull(60);
    s.weapons = {
        make_ph1("Ph-4 #1"), make_ph1("Ph-4 #2"),
        make_ph3("Ph-3 #1"), make_ph3("Ph-3 #2"),
        make_ph3("Ph-5"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 6;  // C3.31

    return s;
}

// ── Klingon C9 Dreadnought (R3.2) ─────────────────────────────────────────────
Ship make_klingon_c9(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Klingon;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 18;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 18; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",6,6},{"R.Warp",6,6},{"A.Warp",3,3},{"B.Warp",3,3}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 4;
    s.sys.battery_cap = 6;
    s.sys.crew_total = 19; s.sys.shuttles = 4;
    s.sys.cloak_installed = true; s.sys.cloak_cost = 8;
    s.sys.shields_max[0] = s.sys.shields[0] = 32;
    s.sys.shields_max[1] = s.sys.shields[1] = 28;
    s.sys.shields_max[2] = s.sys.shields[2] = 26;
    s.sys.shields_max[3] = s.sys.shields[3] = 24;
    s.sys.shields_max[4] = s.sys.shields[4] = 26;
    s.sys.shields_max[5] = s.sys.shields[5] = 28;
    s.sys.hull = s.sys.hull_max = 120;
    s.dac = dac_for_hull(120);
    s.weapons = {
        make_ph1("Ph-1 FX"),   make_ph1("Ph-1 LF"),
        make_ph2("Ph-2K RF"),  make_ph2("Ph-2K RR"),
        make_ph2("Ph-2 LF"),   make_ph2("Ph-2 LR"),
        make_ph2("Ph-2 RF"),   make_ph2("Ph-2 RR"),
        make_disruptor("Dis C"), make_disruptor("Dis D"),
        make_disruptor("Dis E"), make_disruptor("Dis F"),
        make_disruptor("Dis FX"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// ── Klingon C8 Dreadnought (R3.3) ─────────────────────────────────────────────
Ship make_klingon_c8(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Klingon;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 18;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 18; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",6,6},{"R.Warp",6,6},{"A.Warp",3,3},{"B.Warp",3,3}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 4;
    s.sys.battery_cap = 6;
    s.sys.crew_total = 19; s.sys.shuttles = 4;
    s.sys.cloak_installed = true; s.sys.cloak_cost = 8;
    s.sys.shields_max[0] = s.sys.shields[0] = 28;
    s.sys.shields_max[1] = s.sys.shields[1] = 24;
    s.sys.shields_max[2] = s.sys.shields[2] = 24;
    s.sys.shields_max[3] = s.sys.shields[3] = 20;
    s.sys.shields_max[4] = s.sys.shields[4] = 24;
    s.sys.shields_max[5] = s.sys.shields[5] = 24;
    s.sys.hull = s.sys.hull_max = 110;
    s.dac = dac_for_hull(110);
    s.weapons = {
        make_ph2("Ph-2K RF"),  make_ph2("Ph-2K RR"),
        make_ph2("Ph-2 LF"),   make_ph2("Ph-2 LR"),
        make_ph2("Ph-2 RF"),   make_ph2("Ph-2 RR"),
        make_ph3("Ph-3 K"),
        make_disruptor("Dis E"), make_disruptor("Dis F"),
        make_disruptor("Dis FA"), make_disruptor("P.Warp"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// ── Klingon D6 Battlecruiser (R3.5) ───────────────────────────────────────────
Ship make_klingon_d6(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Klingon;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 10;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 10; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",4,4},{"R.Warp",4,4},{"A.Warp",2,2}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 10; s.sys.shuttles = 2;
    s.sys.cloak_installed = true; s.sys.cloak_cost = 6;
    s.sys.shields_max[0] = s.sys.shields[0] = 22;
    s.sys.shields_max[1] = s.sys.shields[1] = 20;
    s.sys.shields_max[2] = s.sys.shields[2] = 20;
    s.sys.shields_max[3] = s.sys.shields[3] = 16;
    s.sys.shields_max[4] = s.sys.shields[4] = 20;
    s.sys.shields_max[5] = s.sys.shields[5] = 20;
    s.sys.hull = s.sys.hull_max = 80;
    s.dac = dac_for_hull(80);
    s.weapons = {
        make_ph2("Ph-2K FX"),
        make_ph2("Ph-2 R"),   make_ph2("Ph-2 RR"),
        make_disruptor("Dis C"), make_disruptor("Dis D"),
        make_disruptor("Dis FA"), make_disruptor("R.Warp Dis"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 3;  // C3.31

    return s;
}

// ── Klingon F5 Frigate (R3.6) ─────────────────────────────────────────────────
Ship make_klingon_f5(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Klingon;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 6;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 6; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",3,3},{"R.Warp",3,3}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 10; s.sys.shuttles = 2;
    s.sys.cloak_installed = true; s.sys.cloak_cost = 4;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 16;
    s.sys.shields_max[2] = s.sys.shields[2] = 16;
    s.sys.shields_max[3] = s.sys.shields[3] = 14;
    s.sys.shields_max[4] = s.sys.shields[4] = 16;
    s.sys.shields_max[5] = s.sys.shields[5] = 16;
    s.sys.hull = s.sys.hull_max = 55;
    s.dac = dac_for_hull(55);
    s.weapons = {
        make_ph2("Ph-2K L"), make_ph2("Ph-2K R"), make_ph2("Ph-2K FA"),
        make_ph2("Ph-2 R"),
        make_disruptor("Dis A"), make_disruptor("Dis B"),
        make_drone("Drone"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 2;  // C3.31

    return s;
}

// ── Klingon E4 Escort (R3.7) ──────────────────────────────────────────────────
Ship make_klingon_e4(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Klingon;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 4;
    s.sys.max_impulse_power = 1;
    s.eaf.warp_power = 4; s.eaf.impulse_power = 1;
    s.sys.warp_groups    = {{"L.Warp",2,2},{"R.Warp",2,2}};
    s.sys.impulse_groups = {{"Impulse",1,1}};
    s.sys.apr_rated = s.sys.apr_current = 1;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 6; s.sys.shuttles = 1;
    s.sys.cloak_installed = true; s.sys.cloak_cost = 4;
    s.sys.shields_max[0] = s.sys.shields[0] = 14;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 12;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 12;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 40;
    s.dac = dac_for_hull(40);
    s.weapons = {
        make_ph2("Ph-2 L"), make_ph2("Ph-2 FA"),
        make_disruptor("Dis A"), make_disruptor("Dis B"),
        make_drone("Drone"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 2;  // C3.31

    return s;
}

// ── Kzinti Battlecruiser (R5.3) ───────────────────────────────────────────────
Ship make_kzinti_bc(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Kzinti;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 24;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 24; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",8,8},{"C.Warp",8,8},{"R.Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 8; s.sys.shuttles = 4;
    s.sys.shields_max[0] = s.sys.shields[0] = 32;
    s.sys.shields_max[1] = s.sys.shields[1] = 28;
    s.sys.shields_max[2] = s.sys.shields[2] = 24;
    s.sys.shields_max[3] = s.sys.shields[3] = 28;
    s.sys.shields_max[4] = s.sys.shields[4] = 24;
    s.sys.shields_max[5] = s.sys.shields[5] = 28;
    s.sys.hull = s.sys.hull_max = 110;
    s.dac = dac_for_hull(110);
    s.weapons = {
        make_ph1("Ph-1 #1 FA/L"), make_ph1("Ph-1 #2 FA"),
        make_disruptor("Dis A FA/L"), make_disruptor("Dis B FA/L"),
        make_disruptor("Dis C FA/R"), make_disruptor("Dis D FA/R"),
        make_ph1("Ph-1-360 #7"),   make_ph1("Ph-1-360 #8"),
        make_ph3("Ph-3 #3 LS"),    make_ph3("Ph-3 #4 LS"),
        make_ph3("Ph-3 #5 RS"),    make_ph3("Ph-3 #6 RS"),
        make_ph3("Ph-3 #9 LR"),    make_ph3("Ph-3 #10 LR"),
        make_ph3("Ph-3 #11 RR"),   make_ph3("Ph-3 #12 RR"),
        make_drone("Drone #1"), make_drone("Drone #2"),
        make_drone("Drone #3"), make_drone("Drone #4"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// ── Kzinti Command Cruiser (R5.4) ─────────────────────────────────────────────
Ship make_kzinti_cc(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Kzinti;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 24;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 24; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",8,8},{"C.Warp",8,8},{"R.Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 8; s.sys.shuttles = 4;
    s.sys.shields_max[0] = s.sys.shields[0] = 32;
    s.sys.shields_max[1] = s.sys.shields[1] = 28;
    s.sys.shields_max[2] = s.sys.shields[2] = 24;
    s.sys.shields_max[3] = s.sys.shields[3] = 28;
    s.sys.shields_max[4] = s.sys.shields[4] = 24;
    s.sys.shields_max[5] = s.sys.shields[5] = 28;
    s.sys.hull = s.sys.hull_max = 100;
    s.dac = dac_for_hull(100);
    s.weapons = {
        make_ph1("Ph-1 #1 FA/L"), make_ph1("Ph-1 #2 FA"),
        make_disruptor("Dis A FA/L"), make_disruptor("Dis B FA/L"),
        make_disruptor("Dis C FA/R"), make_disruptor("Dis D FA/R"),
        make_ph3("Ph-3 #7 RS"),    make_ph3("Ph-3 #8 RS"),
        make_ph3("Ph-3 #3 LS"),    make_ph3("Ph-3 #4 LS"),
        make_ph3("Ph-3 #13 LS"),   make_ph3("Ph-3 #14 LS"),
        make_ph3("Ph-3 #9 LR"),    make_ph3("Ph-3 #10 LR"),
        make_ph3("Ph-3 #11 RR"),   make_ph3("Ph-3 #12 RR"),
        make_drone("Drone #1"), make_drone("Drone #2"),
        make_drone("Drone #3"), make_drone("Drone #4"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// ── Kzinti Light Cruiser (R5.5) ───────────────────────────────────────────────
Ship make_kzinti_cl(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Kzinti;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 18;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 18; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",6,6},{"C.Warp",6,6},{"R.Warp",6,6}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 6; s.sys.shuttles = 3;
    s.sys.shields_max[0] = s.sys.shields[0] = 22;
    s.sys.shields_max[1] = s.sys.shields[1] = 20;
    s.sys.shields_max[2] = s.sys.shields[2] = 20;
    s.sys.shields_max[3] = s.sys.shields[3] = 20;
    s.sys.shields_max[4] = s.sys.shields[4] = 20;
    s.sys.shields_max[5] = s.sys.shields[5] = 20;
    s.sys.hull = s.sys.hull_max = 80;
    s.dac = dac_for_hull(80);
    s.weapons = {
        make_ph1("Ph-1 #1 LF/FA"), make_ph1("Ph-1 #2 RF"),
        make_disruptor("Dis A LF"), make_disruptor("Dis B RF"),
        make_ph3("Ph-3 #3 LS"), make_ph3("Ph-3 #4 LS"),
        make_ph3("Ph-3 #5 RS"), make_ph3("Ph-3 #6 RS"),
        make_drone("Drone #1"), make_drone("Drone #2"),
        make_drone("Drone #3"), make_drone("Drone #4"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 3;  // C3.31

    return s;
}

// ── Kzinti Carrier (R5.6) ─────────────────────────────────────────────────────
Ship make_kzinti_cv(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Kzinti;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 24;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 24; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",8,8},{"C.Warp",8,8},{"R.Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 6;
    s.sys.crew_total = 9; s.sys.shuttles = 12;
    s.sys.shields_max[0] = s.sys.shields[0] = 24;
    s.sys.shields_max[1] = s.sys.shields[1] = 20;
    s.sys.shields_max[2] = s.sys.shields[2] = 18;
    s.sys.shields_max[3] = s.sys.shields[3] = 20;
    s.sys.shields_max[4] = s.sys.shields[4] = 18;
    s.sys.shields_max[5] = s.sys.shields[5] = 20;
    s.sys.hull = s.sys.hull_max = 100;
    s.dac = dac_for_hull(100);
    s.weapons = {
        make_ph1("Ph-1 #1 LF"), make_ph1("Ph-1 #2 RF"),
        make_disruptor("Dis A LF"),
        make_ph1("Ph-1-360 #7"), make_ph1("Ph-1-360 #8"), make_ph1("Ph-1-360 #9"),
        make_ph3("Ph-3-360 #12"), make_ph3("Ph-3-360 #13"), make_ph3("Ph-3-360 #14"),
        make_ph3("Ph-3 #3 LS"),  make_ph3("Ph-3 #4 LS"),
        make_ph3("Ph-3 #5 RS"),  make_ph3("Ph-3 #6 RS"),
        make_ph3("Ph-3 #10 LR"), make_ph3("Ph-3 #11 LR"),
        make_ph3("Ph-3 #15 RR"), make_ph3("Ph-3 #16 RR"),
        make_drone("Drone #1"), make_drone("Drone #2"),
        make_drone("Drone #3"), make_drone("Drone #4"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 5;  // C3.31

    return s;
}

// ── Kzinti Strike Carrier (R5.7) ──────────────────────────────────────────────
Ship make_kzinti_cvs(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Kzinti;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 24;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 24; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",8,8},{"C.Warp",8,8},{"R.Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 6;
    s.sys.crew_total = 9; s.sys.shuttles = 12;
    s.sys.shields_max[0] = s.sys.shields[0] = 24;
    s.sys.shields_max[1] = s.sys.shields[1] = 20;
    s.sys.shields_max[2] = s.sys.shields[2] = 18;
    s.sys.shields_max[3] = s.sys.shields[3] = 20;
    s.sys.shields_max[4] = s.sys.shields[4] = 18;
    s.sys.shields_max[5] = s.sys.shields[5] = 20;
    s.sys.hull = s.sys.hull_max = 90;
    s.dac = dac_for_hull(90);
    s.weapons = {
        make_ph1("Ph-1 #1 FA"), make_ph1("Ph-1 #2 FA"),
        make_disruptor("Dis A FA"), make_disruptor("Dis B FA"),
        make_ph1("Ph-1-360 #7"), make_ph1("Ph-1-360 #8"), make_ph1("Ph-1-360 #9"),
        make_ph3("Ph-3-360 #12"), make_ph3("Ph-3-360 #13"), make_ph3("Ph-3-360 #14"),
        make_ph3("Ph-3 #3 LS"),  make_ph3("Ph-3 #4 LS"),
        make_ph3("Ph-3 #5 RS"),  make_ph3("Ph-3 #6 RS"),
        make_ph3("Ph-3 #10 L/LR"), make_ph3("Ph-3 #11 L/LR"),
        make_ph3("Ph-3 #15 RR"), make_ph3("Ph-3 #16 RR"),
        make_drone("Drone #1"), make_drone("Drone #2"),
        make_drone("Drone #3"), make_drone("Drone #4"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 5;  // C3.31

    return s;
}

// ── Kzinti Frigate (R5.8) ─────────────────────────────────────────────────────
Ship make_kzinti_ff(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Kzinti;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 12;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 12; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",4,4},{"C.Warp",4,4},{"R.Warp",4,4}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 1;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 4; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 14;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 10;
    s.sys.shields_max[3] = s.sys.shields[3] = 12;
    s.sys.shields_max[4] = s.sys.shields[4] = 10;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 50;
    s.dac = dac_for_hull(50);
    s.weapons = {
        make_ph1("Ph-1 FA #1"), make_ph1("Ph-1 #4"),
        make_ph3("Ph-3 #2 LS"), make_ph3("Ph-3 #3 LS"), make_ph3("Ph-3 #1 RS"),
        make_drone("Drone #1"), make_drone("Drone #2"),
        make_drone("Drone #3"), make_drone("Drone #4"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 2;  // C3.31

    return s;
}

// ── Kzinti Escort Frigate (R5.20) ─────────────────────────────────────────────
Ship make_kzinti_eff(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Kzinti;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 12;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 12; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",4,4},{"C.Warp",4,4},{"R.Warp",4,4}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 1;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 4; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 14;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 10;
    s.sys.shields_max[3] = s.sys.shields[3] = 12;
    s.sys.shields_max[4] = s.sys.shields[4] = 10;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 50;
    s.dac = dac_for_hull(50);
    s.weapons = {
        make_ph1("Ph-1 FA #1"), make_ph1("Ph-1 #4"),
        make_ph3("Ph-3 #3 RS"), make_ph3("Ph-3 #2 LS"),
        make_drone("Drone #1"), make_drone("Drone #2"),
        make_drone("ADD #2"),   make_drone("ADD #3"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 2;  // C3.31

    return s;
}

// ── Kzinti Aegis Frigate (R5.20A) ─────────────────────────────────────────────
Ship make_kzinti_aff(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Kzinti;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 12;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 12; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",4,4},{"C.Warp",4,4},{"R.Warp",4,4}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 1;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 4; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 14;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 10;
    s.sys.shields_max[3] = s.sys.shields[3] = 12;
    s.sys.shields_max[4] = s.sys.shields[4] = 10;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 50;
    s.dac = dac_for_hull(50);
    s.weapons = {
        make_ph1("Ph-1 FA #1"), make_ph1("Ph-1 #4"),
        make_ph3("Ph-3 #3 RS"), make_ph3("Ph-3 #2 LS"),
        make_drone("Drone #1"), make_drone("Drone #2"),
        make_drone("ADD #2"),   make_drone("ADD #3"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 2;  // C3.31

    return s;
}

// ── Gorn Battlecruiser (R6.19) ────────────────────────────────────────────────
// hull/shields ESTIMATED; weapons from agent extraction
Ship make_gorn_bc(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Gorn;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 24;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 24; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",8,8},{"C.Warp",8,8},{"R.Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 4;
    s.sys.battery_cap = 6;
    s.sys.crew_total = 8; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 36;
    s.sys.shields_max[1] = s.sys.shields[1] = 30;
    s.sys.shields_max[2] = s.sys.shields[2] = 24;
    s.sys.shields_max[3] = s.sys.shields[3] = 26;
    s.sys.shields_max[4] = s.sys.shields[4] = 24;
    s.sys.shields_max[5] = s.sys.shields[5] = 30;
    s.sys.hull = s.sys.hull_max = 110;
    s.dac = dac_for_hull(110);
    s.weapons = {
        make_ph1("Ph-1 FA #1"), make_ph1("Ph-1 FA #2"),
        make_ph1("Ph-1 L"),     make_ph1("Ph-1 RA"),
        make_ph1("Ph-1 LS"),    make_ph1("Ph-1 RS"),
        make_plasma_f("Plas-F RP"), make_plasma_f("Plas-S RP"),
        make_plasma_f("Plas-F LP"), make_plasma_f("Plas-S LP"),
        make_ph3("Ph-3 RP"),    make_ph3("Ph-3 LP"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 5;  // C3.31

    return s;
}

// ── Gorn Light Cruiser (R6.3) ─────────────────────────────────────────────────
Ship make_gorn_cl(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Gorn;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 18;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 18; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",6,6},{"C.Warp",6,6},{"R.Warp",6,6}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 6; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 24;
    s.sys.shields_max[1] = s.sys.shields[1] = 20;
    s.sys.shields_max[2] = s.sys.shields[2] = 20;
    s.sys.shields_max[3] = s.sys.shields[3] = 18;
    s.sys.shields_max[4] = s.sys.shields[4] = 20;
    s.sys.shields_max[5] = s.sys.shields[5] = 20;
    s.sys.hull = s.sys.hull_max = 80;
    s.dac = dac_for_hull(80);
    s.weapons = {
        make_ph1("Ph-1 FA #1"), make_ph1("Ph-1 FA #2"),
        make_ph1("Ph-1 L"),
        make_ph3("Ph-3 LS"),   make_ph3("Ph-3 RS"),
        make_plasma_f("Plas-F RP"), make_plasma_f("Plas-S RP"),
        make_plasma_f("Plas-F LP"), make_plasma_f("Plas-S LP"),
        make_ph1("Ph-1 RS"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 3;  // C3.31

    return s;
}

// ── Gorn Destroyer (R6.4) ─────────────────────────────────────────────────────
Ship make_gorn_dd(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Gorn;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 8;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 8; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 4; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 16;
    s.sys.shields_max[2] = s.sys.shields[2] = 14;
    s.sys.shields_max[3] = s.sys.shields[3] = 14;
    s.sys.shields_max[4] = s.sys.shields[4] = 14;
    s.sys.shields_max[5] = s.sys.shields[5] = 16;
    s.sys.hull = s.sys.hull_max = 60;
    s.dac = dac_for_hull(60);
    s.weapons = {
        make_ph1("Ph-1 FA"),
        make_ph1("Ph-1 LS"),
        make_plasma_g("Plas-G TRAN"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 3;  // C3.31

    return s;
}

// ── Gorn Fleet Destroyer (R6.4A) ──────────────────────────────────────────────
Ship make_gorn_ddf(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Gorn;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 8;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 8; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 4; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 16;
    s.sys.shields_max[2] = s.sys.shields[2] = 14;
    s.sys.shields_max[3] = s.sys.shields[3] = 14;
    s.sys.shields_max[4] = s.sys.shields[4] = 14;
    s.sys.shields_max[5] = s.sys.shields[5] = 16;
    s.sys.hull = s.sys.hull_max = 60;
    s.dac = dac_for_hull(60);
    s.weapons = {
        make_ph1("Ph-1 FX"),
        make_ph1("Ph-1 LS"),
        make_plasma_f("Plas-F FP"),
        make_ph2("Plas-S C"),   // PlasmaS → ph2 placeholder
        make_ph3("Ph-3 RS"),
        make_ph3("Ph-3 LS"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 3;  // C3.31

    return s;
}

// ── Tholian Patrol Corvette (R7.2) ────────────────────────────────────────────
// Web caster → ph3 placeholder (TODO: implement Web weapon type)
Ship make_tholian_pc(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Tholian;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 8;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 8; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",4,4},{"R.Warp",4,4}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 4; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 14;
    s.sys.shields_max[1] = s.sys.shields[1] = 18;
    s.sys.shields_max[2] = s.sys.shields[2] = 18;
    s.sys.shields_max[3] = s.sys.shields[3] = 12;
    s.sys.shields_max[4] = s.sys.shields[4] = 10;
    s.sys.shields_max[5] = s.sys.shields[5] = 10;
    s.sys.hull = s.sys.hull_max = 40;
    s.dac = dac_for_hull(40);
    s.weapons = {
        make_ph1("Ph-1 FX"),
        make_ph3("Web"),    // TODO: replace with Web weapon type
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 3;  // C3.31

    return s;
}

// ── Tholian Improved Patrol Corvette (R7.3) ───────────────────────────────────
Ship make_tholian_pc_plus(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Tholian;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 8;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 8; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",4,4},{"R.Warp",4,4}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 4; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 20;
    s.sys.shields_max[2] = s.sys.shields[2] = 20;
    s.sys.shields_max[3] = s.sys.shields[3] = 14;
    s.sys.shields_max[4] = s.sys.shields[4] = 12;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 45;
    s.dac = dac_for_hull(45);
    s.weapons = {
        make_ph1("Ph-1 FX"),
        make_ph3("Ph-3 LS"),
        make_ph3("Web"),    // TODO: replace with Web weapon type
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 3;  // C3.31

    return s;
}

// ── Orion Raider Cruiser (R8.2) ───────────────────────────────────────────────
Ship make_orion_cr(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Orion;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 12;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 12; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",6,6},{"R.Warp",6,6}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 6;
    s.sys.crew_total = 6; s.sys.shuttles = 4;
    s.sys.cloak_installed = true; s.sys.cloak_cost = 10;
    s.sys.shields_max[0] = s.sys.shields[0] = 22;
    s.sys.shields_max[1] = s.sys.shields[1] = 20;
    s.sys.shields_max[2] = s.sys.shields[2] = 20;
    s.sys.shields_max[3] = s.sys.shields[3] = 18;
    s.sys.shields_max[4] = s.sys.shields[4] = 20;
    s.sys.shields_max[5] = s.sys.shields[5] = 20;
    s.sys.hull = s.sys.hull_max = 80;
    s.dac = dac_for_hull(80);
    s.weapons = {
        make_ph1("Ph-1 FA #1"), make_ph1("Ph-1 FA #2"),
        make_ph3("Ph-3 RS"), make_ph3("Ph-3 LS"), make_ph3("Ph-3 RA"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 3;  // C3.31

    return s;
}

// ── Hydran Knight-class DN (HKS Monarch, R6.1 approx) ────────────────────────
Ship make_hydran_hdn(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Hydran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 26;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 26; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"R.Warp",13,13},{"L.Warp",13,13}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 6;
    s.sys.battery_charge = 0;
    s.sys.crew_total = 6; s.sys.shuttles = 2;
    s.sys.pcap_rated = s.sys.pcap_charge = 20;
    s.sys.shields_max[0] = s.sys.shields[0] = 20;
    s.sys.shields_max[1] = s.sys.shields[1] = 16;
    s.sys.shields_max[2] = s.sys.shields[2] = 12;
    s.sys.shields_max[3] = s.sys.shields[3] = 14;
    s.sys.shields_max[4] = s.sys.shields[4] = 12;
    s.sys.shields_max[5] = s.sys.shields[5] = 16;
    s.sys.hull = s.sys.hull_max = 110;
    s.dac = dac_for_hull(110);
    s.sys.fighter_bay = 3;
    // Hellbore Cannons (x2, fwd) + Gatling Phaser (x2, broadside) + Ph-3s (defense)
    s.weapons.push_back(make_hellbore("Hellbore-P"));
    s.weapons.push_back(make_hellbore("Hellbore-S"));
    s.weapons.push_back(make_gatling("Gatling-P"));
    s.weapons.push_back(make_gatling("Gatling-S"));
    s.weapons.push_back(make_ph3("Ph-3(F)"));
    s.weapons.push_back(make_ph3("Ph-3(A)"));
    s.weapons.push_back(make_fighter("Fighter-1"));
    s.weapons.push_back(make_fighter("Fighter-2"));
    s.weapons.push_back(make_fighter("Fighter-3"));
    s.fighters_aboard = 3;    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// ── Lyran Tiger-class CA (LSS Inquisitor, R5.1 approx) ───────────────────────
Ship make_lyran_ca(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Lyran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 24;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 24; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"R.Warp",12,12},{"L.Warp",12,12}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 4;
    s.sys.battery_charge = 0;
    s.sys.crew_total = 6; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 22;
    s.sys.shields_max[1] = s.sys.shields[1] = 18;
    s.sys.shields_max[2] = s.sys.shields[2] = 14;
    s.sys.shields_max[3] = s.sys.shields[3] = 16;
    s.sys.shields_max[4] = s.sys.shields[4] = 14;
    s.sys.shields_max[5] = s.sys.shields[5] = 18;
    s.sys.hull = s.sys.hull_max = 100;
    s.dac = dac_for_hull(100);
    // Disruptors x2 (primary), Fusion Beams x2 (short-range brawling), Ph-1 x2
    s.weapons.push_back(make_disruptor("Disruptor-P"));
    s.weapons.push_back(make_disruptor("Disruptor-S"));
    s.weapons.push_back(make_fusion("Fusion-P"));
    s.weapons.push_back(make_fusion("Fusion-S"));
    s.weapons.push_back(make_ph1("Ph-1(P)"));
    s.weapons.push_back(make_ph1("Ph-1(S)"));    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// ── Federation Frigate (R2.4) ────────────────────────────────────────────────
Ship make_federation_ff(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 12;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 12; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"C.Warp-1",6,6},{"C.Warp-2",6,6}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 2; s.sys.pcap_rated = s.sys.pcap_charge = 2;
    s.sys.crew_total = 4; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 16;
    s.sys.shields_max[1] = s.sys.shields[1] = 16;
    s.sys.shields_max[2] = s.sys.shields[2] = 16;
    s.sys.shields_max[3] = s.sys.shields[3] = 14;
    s.sys.shields_max[4] = s.sys.shields[4] = 16;
    s.sys.shields_max[5] = s.sys.shields[5] = 16;
    s.sys.hull = s.sys.hull_max = 45;
    s.dac = dac_for_hull(45);
    s.weapons = {
        make_photon("Phot A"),
        make_ph1("Ph-1 FA"), make_ph1("Ph-1 LS"),
        make_ph3("Ph-3 RS"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 2;  // C3.31

    return s;
}

// ── Romulan Condor Dreadnought (R4.2) ─────────────────────────────────────────
Ship make_romulan_condor(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Romulan;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 28;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 28; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"R.Warp-1",7,7},{"R.Warp-2",7,7},{"L.Warp-1",7,7},{"L.Warp-2",7,7}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 4;
    s.sys.battery_cap = 6;
    s.sys.crew_total = 15; s.sys.shuttles = 4;
    s.sys.cloak_installed = true; s.sys.cloak_cost = 10;
    s.sys.shields_max[0] = s.sys.shields[0] = 36;
    s.sys.shields_max[1] = s.sys.shields[1] = 28;
    s.sys.shields_max[2] = s.sys.shields[2] = 26;
    s.sys.shields_max[3] = s.sys.shields[3] = 26;
    s.sys.shields_max[4] = s.sys.shields[4] = 26;
    s.sys.shields_max[5] = s.sys.shields[5] = 28;
    s.sys.hull = s.sys.hull_max = 120;
    s.dac = dac_for_hull(120);
    s.weapons = {
        make_plasma_r("Plas-R A"), make_plasma_r("Plas-R B"),
        make_ph1("Ph-1 #1"), make_ph1("Ph-1 #2"),
        make_ph1("Ph-1 #3"), make_ph1("Ph-1 #4"),
        make_ph3("Ph-3 L"),  make_ph3("Ph-3 R"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 3;  // C3.31

    return s;
}

// ── Romulan King Eagle Command Cruiser (R4.3) ─────────────────────────────────
Ship make_romulan_ke(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Romulan;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 24;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 24; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"R.Warp-1",6,6},{"R.Warp-2",6,6},{"L.Warp-1",6,6},{"L.Warp-2",6,6}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 4;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 12; s.sys.shuttles = 2;
    s.sys.cloak_installed = true; s.sys.cloak_cost = 10;
    s.sys.shields_max[0] = s.sys.shields[0] = 30;
    s.sys.shields_max[1] = s.sys.shields[1] = 26;
    s.sys.shields_max[2] = s.sys.shields[2] = 24;
    s.sys.shields_max[3] = s.sys.shields[3] = 24;
    s.sys.shields_max[4] = s.sys.shields[4] = 24;
    s.sys.shields_max[5] = s.sys.shields[5] = 26;
    s.sys.hull = s.sys.hull_max = 100;
    s.dac = dac_for_hull(100);
    s.weapons = {
        make_plasma_r("Plas-R A"), make_plasma_r("Plas-R B"),
        make_ph1("Ph-1 #1"), make_ph1("Ph-1 #2"), make_ph1("Ph-1 #3"),
        make_ph3("Ph-3 L"),  make_ph3("Ph-3 R"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 3;  // C3.31

    return s;
}

// ── Romulan Sparrowhawk Light Cruiser (R4.4) ──────────────────────────────────
Ship make_romulan_sh(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Romulan;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 20;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 20; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"R.Warp",5,5},{"R.Warp2",5,5},{"L.Warp",5,5},{"L.Warp2",5,5}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 8; s.sys.shuttles = 1;
    s.sys.cloak_installed = true; s.sys.cloak_cost = 8;
    s.sys.shields_max[0] = s.sys.shields[0] = 24;
    s.sys.shields_max[1] = s.sys.shields[1] = 22;
    s.sys.shields_max[2] = s.sys.shields[2] = 20;
    s.sys.shields_max[3] = s.sys.shields[3] = 20;
    s.sys.shields_max[4] = s.sys.shields[4] = 20;
    s.sys.shields_max[5] = s.sys.shields[5] = 22;
    s.sys.hull = s.sys.hull_max = 80;
    s.dac = dac_for_hull(80);
    s.weapons = {
        make_plasma_r("Plas-R A"),
        make_ph1("Ph-1 FA"), make_ph1("Ph-1 L"),
        make_ph3("Ph-3 RS"), make_ph3("Ph-3 RA"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 3;  // C3.31

    return s;
}

// ── Romulan Skyhawk Destroyer (R4.5a) ─────────────────────────────────────────
Ship make_romulan_sky(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Romulan;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 14;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 14; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"Warp-A",7,7},{"Warp-B",7,7}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 6; s.sys.shuttles = 1;
    s.sys.cloak_installed = true; s.sys.cloak_cost = 6;
    s.sys.shields_max[0] = s.sys.shields[0] = 20;
    s.sys.shields_max[1] = s.sys.shields[1] = 18;
    s.sys.shields_max[2] = s.sys.shields[2] = 16;
    s.sys.shields_max[3] = s.sys.shields[3] = 14;
    s.sys.shields_max[4] = s.sys.shields[4] = 16;
    s.sys.shields_max[5] = s.sys.shields[5] = 18;
    s.sys.hull = s.sys.hull_max = 55;
    s.dac = dac_for_hull(55);
    s.weapons = {
        make_plasma_f("Plas-F FA"),
        make_ph1("Ph-1 FA"), make_ph1("Ph-1 LS"),
        make_ph3("Ph-3 RS"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 3;  // C3.31

    return s;
}

// ── Romulan Snipe Frigate (R4.6) ──────────────────────────────────────────────
Ship make_romulan_snipe(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Romulan;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 10;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 10; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"Warp-A",5,5},{"Warp-B",5,5}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 4; s.sys.shuttles = 1;
    s.sys.cloak_installed = true; s.sys.cloak_cost = 5;
    s.sys.shields_max[0] = s.sys.shields[0] = 16;
    s.sys.shields_max[1] = s.sys.shields[1] = 14;
    s.sys.shields_max[2] = s.sys.shields[2] = 12;
    s.sys.shields_max[3] = s.sys.shields[3] = 12;
    s.sys.shields_max[4] = s.sys.shields[4] = 12;
    s.sys.shields_max[5] = s.sys.shields[5] = 14;
    s.sys.hull = s.sys.hull_max = 40;
    s.dac = dac_for_hull(40);
    s.weapons = {
        make_plasma_g("Plas-G FA"),
        make_ph1("Ph-1 FA"),
        make_ph3("Ph-3 LS"), make_ph3("Ph-3 RS"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 2;  // C3.31

    return s;
}

// ── Gorn Frigate (R6.5) ───────────────────────────────────────────────────────
Ship make_gorn_ff(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Gorn;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 8;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 8; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 1;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 3; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 14;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 10;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 10;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 40;
    s.dac = dac_for_hull(40);
    s.weapons = {
        make_plasma_g("Plas-G FA"),
        make_ph1("Ph-1 FA"),
        make_ph3("Ph-3 LS"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 2;  // C3.31

    return s;
}

// ── Gorn Scout Cruiser (R6.6) ─────────────────────────────────────────────────
Ship make_gorn_sc(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Gorn;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 14;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 14; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",7,7},{"R.Warp",7,7}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 5; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 20;
    s.sys.shields_max[1] = s.sys.shields[1] = 18;
    s.sys.shields_max[2] = s.sys.shields[2] = 16;
    s.sys.shields_max[3] = s.sys.shields[3] = 16;
    s.sys.shields_max[4] = s.sys.shields[4] = 16;
    s.sys.shields_max[5] = s.sys.shields[5] = 18;
    s.sys.hull = s.sys.hull_max = 55;
    s.dac = dac_for_hull(55);
    s.weapons = {
        make_plasma_f("Plas-F FA"),
        make_ph1("Ph-1 FA"),
        make_ph3("Ph-3 LS"), make_ph3("Ph-3 RS"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// ── Tholian Destroyer (R7.4) ──────────────────────────────────────────────────
Ship make_tholian_dd(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Tholian;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 10;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 10; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",5,5},{"R.Warp",5,5}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 4; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 16;
    s.sys.shields_max[1] = s.sys.shields[1] = 16;
    s.sys.shields_max[2] = s.sys.shields[2] = 14;
    s.sys.shields_max[3] = s.sys.shields[3] = 12;
    s.sys.shields_max[4] = s.sys.shields[4] = 14;
    s.sys.shields_max[5] = s.sys.shields[5] = 16;
    s.sys.hull = s.sys.hull_max = 50;
    s.dac = dac_for_hull(50);
    s.weapons = {
        make_ph1("Ph-1 FX"),
        make_ph3("Web A"), make_ph3("Web B"),   // Web = ph3 placeholder
        make_ph3("Ph-3 LS"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 2;  // C3.31

    return s;
}

// ── Tholian Cruiser (R7.5) ────────────────────────────────────────────────────
Ship make_tholian_co(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Tholian;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 14;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 14; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",7,7},{"R.Warp",7,7}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 5; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 20;
    s.sys.shields_max[1] = s.sys.shields[1] = 20;
    s.sys.shields_max[2] = s.sys.shields[2] = 18;
    s.sys.shields_max[3] = s.sys.shields[3] = 16;
    s.sys.shields_max[4] = s.sys.shields[4] = 18;
    s.sys.shields_max[5] = s.sys.shields[5] = 20;
    s.sys.hull = s.sys.hull_max = 65;
    s.dac = dac_for_hull(65);
    s.weapons = {
        make_ph1("Ph-1 FX"),
        make_ph3("Web A"), make_ph3("Web B"),
        make_ph3("Ph-3 LS"), make_ph3("Ph-3 RS"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 3;  // C3.31

    return s;
}

// ── Orion Light Raider (R8.3) ─────────────────────────────────────────────────
Ship make_orion_lr(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Orion;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 10;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 10; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",5,5},{"R.Warp",5,5}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 4; s.sys.shuttles = 2;
    s.sys.cloak_installed = true; s.sys.cloak_cost = 8;
    s.sys.shields_max[0] = s.sys.shields[0] = 16;
    s.sys.shields_max[1] = s.sys.shields[1] = 14;
    s.sys.shields_max[2] = s.sys.shields[2] = 12;
    s.sys.shields_max[3] = s.sys.shields[3] = 12;
    s.sys.shields_max[4] = s.sys.shields[4] = 12;
    s.sys.shields_max[5] = s.sys.shields[5] = 14;
    s.sys.hull = s.sys.hull_max = 50;
    s.dac = dac_for_hull(50);
    s.weapons = {
        make_ph3("Ph-3 FA"), make_ph3("Ph-3 LS"), make_ph3("Ph-3 RS"),
        make_drone("Drone #1"), make_drone("Drone #2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 3;  // C3.31

    return s;
}

// ── Orion Battle Raider (R8.4) ────────────────────────────────────────────────
Ship make_orion_br(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Orion;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 14;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 14; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",7,7},{"R.Warp",7,7}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 5; s.sys.shuttles = 2;
    s.sys.cloak_installed = true; s.sys.cloak_cost = 8;
    s.sys.shields_max[0] = s.sys.shields[0] = 20;
    s.sys.shields_max[1] = s.sys.shields[1] = 18;
    s.sys.shields_max[2] = s.sys.shields[2] = 16;
    s.sys.shields_max[3] = s.sys.shields[3] = 16;
    s.sys.shields_max[4] = s.sys.shields[4] = 16;
    s.sys.shields_max[5] = s.sys.shields[5] = 18;
    s.sys.hull = s.sys.hull_max = 65;
    s.dac = dac_for_hull(65);
    s.weapons = {
        make_ph1("Ph-1 FA #1"), make_ph1("Ph-1 FA #2"),
        make_ph3("Ph-3 LS"), make_ph3("Ph-3 RS"), make_ph3("Ph-3 RA"),
        make_drone("Drone #1"), make_drone("Drone #2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// ── Orion Heavy Cruiser (R8.5) ────────────────────────────────────────────────
Ship make_orion_ca(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Orion;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 18;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 18; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",9,9},{"R.Warp",9,9}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 6;
    s.sys.crew_total = 7; s.sys.shuttles = 4;
    s.sys.cloak_installed = true; s.sys.cloak_cost = 10;
    s.sys.shields_max[0] = s.sys.shields[0] = 26;
    s.sys.shields_max[1] = s.sys.shields[1] = 22;
    s.sys.shields_max[2] = s.sys.shields[2] = 20;
    s.sys.shields_max[3] = s.sys.shields[3] = 20;
    s.sys.shields_max[4] = s.sys.shields[4] = 20;
    s.sys.shields_max[5] = s.sys.shields[5] = 22;
    s.sys.hull = s.sys.hull_max = 90;
    s.dac = dac_for_hull(90);
    s.weapons = {
        make_ph1("Ph-1 FA #1"), make_ph1("Ph-1 FA #2"),
        make_ph1("Ph-1 LS"),
        make_ph3("Ph-3 RS"), make_ph3("Ph-3 RA"), make_ph3("Ph-3 LA"),
        make_drone("Drone #1"), make_drone("Drone #2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// ── Lyran Wildcat Battlecruiser ────────────────────────────────────────────────
Ship make_lyran_bc(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Lyran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 26;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 26; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"R.Warp",13,13},{"L.Warp",13,13}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 4;
    s.sys.battery_cap = 6;
    s.sys.crew_total = 8; s.sys.shuttles = 3;
    s.sys.shields_max[0] = s.sys.shields[0] = 28;
    s.sys.shields_max[1] = s.sys.shields[1] = 24;
    s.sys.shields_max[2] = s.sys.shields[2] = 22;
    s.sys.shields_max[3] = s.sys.shields[3] = 22;
    s.sys.shields_max[4] = s.sys.shields[4] = 22;
    s.sys.shields_max[5] = s.sys.shields[5] = 24;
    s.sys.hull = s.sys.hull_max = 110;
    s.dac = dac_for_hull(110);
    s.weapons.push_back(make_disruptor("Disruptor-P"));
    s.weapons.push_back(make_disruptor("Disruptor-S"));
    s.weapons.push_back(make_disruptor("Disruptor-C"));
    s.weapons.push_back(make_fusion("Fusion-P"));
    s.weapons.push_back(make_fusion("Fusion-S"));
    s.weapons.push_back(make_ph1("Ph-1(P)"));
    s.weapons.push_back(make_ph1("Ph-1(S)"));
    s.weapons.push_back(make_ph3("Ph-3(A)"));    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// ── Lyran Panther Light Cruiser ────────────────────────────────────────────────
Ship make_lyran_cl(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Lyran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 20;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 20; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"R.Warp",10,10},{"L.Warp",10,10}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 6; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 22;
    s.sys.shields_max[1] = s.sys.shields[1] = 18;
    s.sys.shields_max[2] = s.sys.shields[2] = 16;
    s.sys.shields_max[3] = s.sys.shields[3] = 16;
    s.sys.shields_max[4] = s.sys.shields[4] = 16;
    s.sys.shields_max[5] = s.sys.shields[5] = 18;
    s.sys.hull = s.sys.hull_max = 80;
    s.dac = dac_for_hull(80);
    s.weapons.push_back(make_disruptor("Disruptor-P"));
    s.weapons.push_back(make_disruptor("Disruptor-S"));
    s.weapons.push_back(make_fusion("Fusion-P"));
    s.weapons.push_back(make_ph1("Ph-1(P)"));
    s.weapons.push_back(make_ph1("Ph-1(S)"));
    s.weapons.push_back(make_ph3("Ph-3(A)"));    s.assign_weapon_arcs();
    s.turn_mode_cat = 3;  // C3.31

    return s;
}

// ── Lyran Leopard Destroyer ────────────────────────────────────────────────────
Ship make_lyran_dd(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Lyran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 16;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 16; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"R.Warp",8,8},{"L.Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 4; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 16;
    s.sys.shields_max[2] = s.sys.shields[2] = 14;
    s.sys.shields_max[3] = s.sys.shields[3] = 14;
    s.sys.shields_max[4] = s.sys.shields[4] = 14;
    s.sys.shields_max[5] = s.sys.shields[5] = 16;
    s.sys.hull = s.sys.hull_max = 60;
    s.dac = dac_for_hull(60);
    s.weapons.push_back(make_disruptor("Disruptor-P"));
    s.weapons.push_back(make_fusion("Fusion-P"));
    s.weapons.push_back(make_ph1("Ph-1(P)"));
    s.weapons.push_back(make_ph1("Ph-1(S)"));
    s.weapons.push_back(make_ph3("Ph-3(A)"));    s.assign_weapon_arcs();
    s.turn_mode_cat = 3;  // C3.31

    return s;
}

// ── Lyran Cheetah Frigate ─────────────────────────────────────────────────────
Ship make_lyran_ff(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Lyran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 12;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 12; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"R.Warp",6,6},{"L.Warp",6,6}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 3; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 14;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 10;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 10;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 45;
    s.dac = dac_for_hull(45);
    s.weapons.push_back(make_disruptor("Disruptor-P"));
    s.weapons.push_back(make_fusion("Fusion-P"));
    s.weapons.push_back(make_ph1("Ph-1(P)"));    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// ── Hydran Lord Command Cruiser ────────────────────────────────────────────────
Ship make_hydran_lord(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Hydran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 22;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 22; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"R.Warp",11,11},{"L.Warp",11,11}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 6;
    s.sys.crew_total = 7; s.sys.shuttles = 3;
    s.sys.pcap_rated = s.sys.pcap_charge = 16;
    s.sys.shields_max[0] = s.sys.shields[0] = 24;
    s.sys.shields_max[1] = s.sys.shields[1] = 20;
    s.sys.shields_max[2] = s.sys.shields[2] = 18;
    s.sys.shields_max[3] = s.sys.shields[3] = 18;
    s.sys.shields_max[4] = s.sys.shields[4] = 18;
    s.sys.shields_max[5] = s.sys.shields[5] = 20;
    s.sys.hull = s.sys.hull_max = 100;
    s.dac = dac_for_hull(100);
    s.weapons.push_back(make_hellbore("Hellbore-P"));
    s.weapons.push_back(make_hellbore("Hellbore-S"));
    s.weapons.push_back(make_gatling("Gatling-P"));
    s.weapons.push_back(make_gatling("Gatling-S"));
    s.weapons.push_back(make_ph3("Ph-3(F)"));
    s.weapons.push_back(make_ph3("Ph-3(A)"));
    s.weapons.push_back(make_ph3("Ph-3(L)"));
    s.sys.fighter_bay = 3;
    s.weapons.push_back(make_fighter("Fighter-1"));
    s.weapons.push_back(make_fighter("Fighter-2"));
    s.weapons.push_back(make_fighter("Fighter-3"));
    s.fighters_aboard = 3;    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// ── Hydran Ranger Light Cruiser ────────────────────────────────────────────────
Ship make_hydran_ranger(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Hydran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 18;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 18; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"R.Warp",9,9},{"L.Warp",9,9}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 5; s.sys.shuttles = 2;
    s.sys.pcap_rated = s.sys.pcap_charge = 12;
    s.sys.shields_max[0] = s.sys.shields[0] = 20;
    s.sys.shields_max[1] = s.sys.shields[1] = 16;
    s.sys.shields_max[2] = s.sys.shields[2] = 14;
    s.sys.shields_max[3] = s.sys.shields[3] = 14;
    s.sys.shields_max[4] = s.sys.shields[4] = 14;
    s.sys.shields_max[5] = s.sys.shields[5] = 16;
    s.sys.hull = s.sys.hull_max = 80;
    s.dac = dac_for_hull(80);
    s.weapons.push_back(make_hellbore("Hellbore-P"));
    s.weapons.push_back(make_hellbore("Hellbore-S"));
    s.weapons.push_back(make_gatling("Gatling-P"));
    s.weapons.push_back(make_ph3("Ph-3(F)"));
    s.weapons.push_back(make_ph3("Ph-3(A)"));    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// ── Hydran Lancer Destroyer ────────────────────────────────────────────────────
Ship make_hydran_lancer(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Hydran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 14;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 14; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"R.Warp",7,7},{"L.Warp",7,7}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 4; s.sys.shuttles = 2;
    s.sys.pcap_rated = s.sys.pcap_charge = 8;
    s.sys.shields_max[0] = s.sys.shields[0] = 16;
    s.sys.shields_max[1] = s.sys.shields[1] = 14;
    s.sys.shields_max[2] = s.sys.shields[2] = 12;
    s.sys.shields_max[3] = s.sys.shields[3] = 12;
    s.sys.shields_max[4] = s.sys.shields[4] = 12;
    s.sys.shields_max[5] = s.sys.shields[5] = 14;
    s.sys.hull = s.sys.hull_max = 60;
    s.dac = dac_for_hull(60);
    s.weapons.push_back(make_hellbore("Hellbore-P"));
    s.weapons.push_back(make_gatling("Gatling-P"));
    s.weapons.push_back(make_ph3("Ph-3(F)"));
    s.weapons.push_back(make_ph3("Ph-3(A)"));    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// ── Hydran Cuirassier Frigate ──────────────────────────────────────────────────
Ship make_hydran_cuirassier(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Hydran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 10;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 10; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"R.Warp",5,5},{"L.Warp",5,5}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 3; s.sys.shuttles = 1;
    s.sys.pcap_rated = s.sys.pcap_charge = 6;
    s.sys.shields_max[0] = s.sys.shields[0] = 14;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 10;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 10;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 45;
    s.dac = dac_for_hull(45);
    s.weapons.push_back(make_hellbore("Hellbore-P"));
    s.weapons.push_back(make_ph3("Ph-3(F)"));
    s.weapons.push_back(make_ph3("Ph-3(A)"));    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// ── Federation Cargo Pod (R2.9) ────────────────────────────────────────────────
Ship make_federation_pod(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 0;
    s.sys.max_impulse_power = 0;
    s.eaf.warp_power = 0; s.eaf.impulse_power = 0;
    s.sys.apr_rated = s.sys.apr_current = 0;
    s.sys.battery_cap = 0;
    s.sys.crew_total = 0; s.sys.shuttles = 0;
    s.sys.shields_max[0] = s.sys.shields[0] = 8;
    s.sys.shields_max[1] = s.sys.shields[1] = 8;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 8;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 8;
    s.sys.hull = s.sys.hull_max = 20;
    s.dac = dac_for_hull(20);    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}


// ═══════════════════════════════════════════════════════════════════════════════
// AUTO-GENERATED FROM MODULE C1/R2/R3/J/FED-MSB EXTRACTION
// ═══════════════════════════════════════════════════════════════════════════════
// â”€â”€ Battle Carrier (CVB) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_cvb(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 24;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 24; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",12,12},{"R.Warp",12,12}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 4;
    s.sys.battery_cap = 6;
    s.sys.crew_total = 10; s.sys.shuttles = 6;
    s.sys.shields_max[0] = s.sys.shields[0] = 24;
    s.sys.shields_max[1] = s.sys.shields[1] = 20;
    s.sys.shields_max[2] = s.sys.shields[2] = 16;
    s.sys.shields_max[3] = s.sys.shields[3] = 16;
    s.sys.shields_max[4] = s.sys.shields[4] = 16;
    s.sys.shields_max[5] = s.sys.shields[5] = 20;
    s.sys.hull = s.sys.hull_max = 46;
    s.dac = dac_for_hull(45);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"), make_ph1("Ph-1 C"), make_ph1("Ph-1 D"),
        make_photon("Photon A"), make_photon("Photon B"),
        make_drone("Drone Rack 1"), make_drone("Drone Rack 2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ War Destroyer Minesweeper (DWM) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_dwm(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 16;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 16; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",8,8},{"R.Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 3;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 10; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 14;
    s.sys.shields_max[2] = s.sys.shields[2] = 10;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 10;
    s.sys.shields_max[5] = s.sys.shields[5] = 14;
    s.sys.hull = s.sys.hull_max = 20;
    s.dac = dac_for_hull(20);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_ph3("Ph-G 1"), make_ph3("Ph-G 2"),
        make_drone("Drone Rack 1"), make_drone("Drone Rack 2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Bismarck-Class Battlecruiser (BCF) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_bcf(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 30;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 30; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",15,15},{"R.Warp",15,15}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 6;
    s.sys.battery_cap = 6;
    s.sys.crew_total = 10; s.sys.shuttles = 6;
    s.sys.shields_max[0] = s.sys.shields[0] = 36;
    s.sys.shields_max[1] = s.sys.shields[1] = 28;
    s.sys.shields_max[2] = s.sys.shields[2] = 20;
    s.sys.shields_max[3] = s.sys.shields[3] = 20;
    s.sys.shields_max[4] = s.sys.shields[4] = 28;
    s.sys.shields_max[5] = s.sys.shields[5] = 24;
    s.sys.hull = s.sys.hull_max = 36;
    s.dac = dac_for_hull(35);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"), make_ph1("Ph-1 C"),
        make_ph1("Ph-1 D"), make_ph1("Ph-1 E"), make_ph1("Ph-1 F"),
        make_ph3("Ph-3 1"), make_ph3("Ph-3 2"), make_ph3("Ph-3 3"), make_ph3("Ph-3 4"),
        make_photon("Photon A"), make_photon("Photon B"),
        make_photon("Photon C"), make_photon("Photon D"),
        make_plasma_f("Plasma-F 1"), make_plasma_f("Plasma-F 2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Priority Transport (FFT) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_fft(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 12;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 12; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",6,6},{"R.Warp",6,6}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 0;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 10; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 14;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 8;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 12;
    s.dac = dac_for_hull(10);
    s.weapons = {
        make_ph3("Ph-3 1"), make_ph3("Ph-3 2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Minehunter Frigate (FFM) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_ffm(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 12;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 12; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",6,6},{"R.Warp",6,6}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 0;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 10; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 14;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 8;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 16;
    s.dac = dac_for_hull(15);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_ph3("Ph-3 1"), make_ph3("Ph-3 2"),
        make_drone("Drone Rack 1"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Escort Carrier (CVE) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_cve(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 20;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 20; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",10,10},{"R.Warp",10,10}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 4;
    s.sys.battery_cap = 6;
    s.sys.crew_total = 10; s.sys.shuttles = 4;
    s.sys.shields_max[0] = s.sys.shields[0] = 20;
    s.sys.shields_max[1] = s.sys.shields[1] = 16;
    s.sys.shields_max[2] = s.sys.shields[2] = 12;
    s.sys.shields_max[3] = s.sys.shields[3] = 12;
    s.sys.shields_max[4] = s.sys.shields[4] = 12;
    s.sys.shields_max[5] = s.sys.shields[5] = 16;
    s.sys.hull = s.sys.hull_max = 37;
    s.dac = dac_for_hull(35);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_ph3("Ph-3 1"), make_ph3("Ph-3 2"),
        make_drone("Drone Rack 1"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Destroyer Leader (DDL) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_ddl(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 16;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 16; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",8,8},{"R.Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 3;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 10; s.sys.shuttles = 3;
    s.sys.shields_max[0] = s.sys.shields[0] = 22;
    s.sys.shields_max[1] = s.sys.shields[1] = 16;
    s.sys.shields_max[2] = s.sys.shields[2] = 10;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 16;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 22;
    s.dac = dac_for_hull(20);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"), make_ph1("Ph-1 C"), make_ph1("Ph-1 D"),
        make_ph3("Ph-3 LS"), make_ph3("Ph-3 RS"),
        make_photon("Photon A"), make_photon("Photon B"), make_photon("Photon C"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Strike Cruiser (CS) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_cs(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 26;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 26; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",13,13},{"R.Warp",13,13}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 4;
    s.sys.battery_cap = 6;
    s.sys.crew_total = 10; s.sys.shuttles = 4;
    s.sys.shields_max[0] = s.sys.shields[0] = 30;
    s.sys.shields_max[1] = s.sys.shields[1] = 22;
    s.sys.shields_max[2] = s.sys.shields[2] = 16;
    s.sys.shields_max[3] = s.sys.shields[3] = 16;
    s.sys.shields_max[4] = s.sys.shields[4] = 22;
    s.sys.shields_max[5] = s.sys.shields[5] = 18;
    s.sys.hull = s.sys.hull_max = 30;
    s.dac = dac_for_hull(30);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"), make_ph1("Ph-1 C"), make_ph1("Ph-1 D"),
        make_ph2("Ph-2 1"), make_ph2("Ph-2 2"),
        make_ph3("Ph-3 1"), make_ph3("Ph-3 2"),
        make_photon("Photon A"), make_photon("Photon B"),
        make_photon("Photon C"), make_photon("Photon D"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Commando Carrier (COV) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_cov(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 22;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 22; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",11,11},{"R.Warp",11,11}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 4;
    s.sys.battery_cap = 6;
    s.sys.crew_total = 10; s.sys.shuttles = 7;
    s.sys.shields_max[0] = s.sys.shields[0] = 26;
    s.sys.shields_max[1] = s.sys.shields[1] = 18;
    s.sys.shields_max[2] = s.sys.shields[2] = 12;
    s.sys.shields_max[3] = s.sys.shields[3] = 12;
    s.sys.shields_max[4] = s.sys.shields[4] = 18;
    s.sys.shields_max[5] = s.sys.shields[5] = 14;
    s.sys.hull = s.sys.hull_max = 30;
    s.dac = dac_for_hull(30);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"), make_ph1("Ph-1 C"), make_ph1("Ph-1 D"),
        make_ph3("Ph-G 1"), make_ph3("Ph-G 2"),
        make_photon("Photon A"), make_photon("Photon B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ VIP Transport (FFP) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_ffp(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 10;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 10; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",5,5},{"R.Warp",5,5}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 0;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 10; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 14;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 8;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 10;
    s.dac = dac_for_hull(10);
    s.weapons = {
        make_ph3("Ph-3 1"), make_ph3("Ph-3 2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Light Carrier (CVL) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_cvl(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 22;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 22; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",11,11},{"R.Warp",11,11}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 4;
    s.sys.battery_cap = 6;
    s.sys.crew_total = 10; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 26;
    s.sys.shields_max[1] = s.sys.shields[1] = 18;
    s.sys.shields_max[2] = s.sys.shields[2] = 12;
    s.sys.shields_max[3] = s.sys.shields[3] = 12;
    s.sys.shields_max[4] = s.sys.shields[4] = 18;
    s.sys.shields_max[5] = s.sys.shields[5] = 14;
    s.sys.hull = s.sys.hull_max = 28;
    s.dac = dac_for_hull(30);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_ph3("Ph-3 1"), make_ph3("Ph-3 2"),
        make_photon("Photon A"), make_photon("Photon B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Guided Weapons Destroyer (DDG) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_ddg(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 14;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 14; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",7,7},{"R.Warp",7,7}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 3;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 10; s.sys.shuttles = 3;
    s.sys.shields_max[0] = s.sys.shields[0] = 20;
    s.sys.shields_max[1] = s.sys.shields[1] = 14;
    s.sys.shields_max[2] = s.sys.shields[2] = 10;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 14;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 20;
    s.dac = dac_for_hull(20);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_ph3("Ph-3 1"), make_ph3("Ph-3 2"),
        make_photon("Photon A"), make_photon("Photon B"),
        make_drone("Drone 1"), make_drone("Drone 2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Advanced Technology Command Cruiser (CX) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_cx(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 30;
    s.sys.max_impulse_power = 6;
    s.eaf.warp_power = 30; s.eaf.impulse_power = 6;
    s.sys.warp_groups    = {{"L.Warp",15,15},{"R.Warp",15,15}};
    s.sys.impulse_groups = {{"Impulse",6,6}};
    s.sys.apr_rated = s.sys.apr_current = 6;
    s.sys.battery_cap = 8;
    s.sys.crew_total = 10; s.sys.shuttles = 6;
    s.sys.shields_max[0] = s.sys.shields[0] = 36;
    s.sys.shields_max[1] = s.sys.shields[1] = 28;
    s.sys.shields_max[2] = s.sys.shields[2] = 20;
    s.sys.shields_max[3] = s.sys.shields[3] = 20;
    s.sys.shields_max[4] = s.sys.shields[4] = 28;
    s.sys.shields_max[5] = s.sys.shields[5] = 24;
    s.sys.hull = s.sys.hull_max = 36;
    s.dac = dac_for_hull(35);
    s.weapons = {
        make_gatling("Ph-G 1"), make_gatling("Ph-G 2"), make_gatling("Ph-G 3"),
        make_gatling("Ph-G 4"), make_gatling("Ph-G 5"), make_gatling("Ph-G 6"),
        make_photon("Photon A"), make_photon("Photon B"),
        make_photon("Photon C"), make_photon("Photon D"),
        make_ph3("Ph-3 1"), make_ph3("Ph-3 2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ New Scout Cruiser (NSC) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_nsc(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 20;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 20; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",10,10},{"R.Warp",10,10}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 4;
    s.sys.battery_cap = 6;
    s.sys.crew_total = 10; s.sys.shuttles = 3;
    s.sys.shields_max[0] = s.sys.shields[0] = 28;
    s.sys.shields_max[1] = s.sys.shields[1] = 20;
    s.sys.shields_max[2] = s.sys.shields[2] = 14;
    s.sys.shields_max[3] = s.sys.shields[3] = 14;
    s.sys.shields_max[4] = s.sys.shields[4] = 20;
    s.sys.shields_max[5] = s.sys.shields[5] = 16;
    s.sys.hull = s.sys.hull_max = 28;
    s.dac = dac_for_hull(30);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_ph3("Ph-3 1"), make_ph3("Ph-3 2"),
        make_photon("Photon A"), make_photon("Photon B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Heavy Drone Cruiser (CAD) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_cad(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 22;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 22; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",11,11},{"R.Warp",11,11}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 4;
    s.sys.battery_cap = 6;
    s.sys.crew_total = 10; s.sys.shuttles = 4;
    s.sys.shields_max[0] = s.sys.shields[0] = 28;
    s.sys.shields_max[1] = s.sys.shields[1] = 20;
    s.sys.shields_max[2] = s.sys.shields[2] = 14;
    s.sys.shields_max[3] = s.sys.shields[3] = 14;
    s.sys.shields_max[4] = s.sys.shields[4] = 20;
    s.sys.shields_max[5] = s.sys.shields[5] = 16;
    s.sys.hull = s.sys.hull_max = 30;
    s.dac = dac_for_hull(30);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"), make_ph1("Ph-1 C"), make_ph1("Ph-1 D"),
        make_ph3("Ph-3 1"), make_ph3("Ph-3 2"),
        make_drone("Drone 1"), make_drone("Drone 2"), make_drone("Drone 3"), make_drone("Drone 4"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Heavy Carrier (CVA) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_cva(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 28;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 28; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",14,14},{"R.Warp",14,14}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 4;
    s.sys.battery_cap = 6;
    s.sys.crew_total = 10; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 34;
    s.sys.shields_max[1] = s.sys.shields[1] = 26;
    s.sys.shields_max[2] = s.sys.shields[2] = 18;
    s.sys.shields_max[3] = s.sys.shields[3] = 18;
    s.sys.shields_max[4] = s.sys.shields[4] = 26;
    s.sys.shields_max[5] = s.sys.shields[5] = 22;
    s.sys.hull = s.sys.hull_max = 36;
    s.dac = dac_for_hull(35);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"), make_ph1("Ph-1 C"), make_ph1("Ph-1 D"),
        make_ph2("Ph-2 1"), make_ph2("Ph-2 2"), make_ph2("Ph-2 3"), make_ph2("Ph-2 4"),
        make_photon("Photon A"), make_photon("Photon B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ PV Police Carrier (Police Cutter conversion) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_pv(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 14;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 14; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",7,7},{"R.Warp",7,7}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 0;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 10; s.sys.shuttles = 12;
    s.sys.shields_max[0] = s.sys.shields[0] = 20;
    s.sys.shields_max[1] = s.sys.shields[1] = 16;
    s.sys.shields_max[2] = s.sys.shields[2] = 12;
    s.sys.shields_max[3] = s.sys.shields[3] = 12;
    s.sys.shields_max[4] = s.sys.shields[4] = 16;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 28;
    s.dac = dac_for_hull(30);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_ph3("Ph-3 1"), make_ph3("Ph-3 2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ New Command Cruiser (NCC) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_ncc(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 28;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 28; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",14,14},{"R.Warp",14,14}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 4;
    s.sys.battery_cap = 8;
    s.sys.crew_total = 10; s.sys.shuttles = 6;
    s.sys.shields_max[0] = s.sys.shields[0] = 34;
    s.sys.shields_max[1] = s.sys.shields[1] = 26;
    s.sys.shields_max[2] = s.sys.shields[2] = 18;
    s.sys.shields_max[3] = s.sys.shields[3] = 18;
    s.sys.shields_max[4] = s.sys.shields[4] = 26;
    s.sys.shields_max[5] = s.sys.shields[5] = 22;
    s.sys.hull = s.sys.hull_max = 36;
    s.dac = dac_for_hull(35);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"), make_ph1("Ph-1 C"),
        make_ph1("Ph-1 D"), make_ph1("Ph-1 E"), make_ph1("Ph-1 F"),
        make_ph2("Ph-2 1"), make_ph2("Ph-2 2"),
        make_ph3("Ph-3 1"), make_ph3("Ph-3 2"),
        make_photon("Photon A"), make_photon("Photon B"),
        make_photon("Photon C"), make_photon("Photon D"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Kirov-Class Battlecruiser (BCG) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_bcg(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 30;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 30; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",15,15},{"R.Warp",15,15}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 5;
    s.sys.battery_cap = 8;
    s.sys.crew_total = 10; s.sys.shuttles = 6;
    s.sys.shields_max[0] = s.sys.shields[0] = 38;
    s.sys.shields_max[1] = s.sys.shields[1] = 30;
    s.sys.shields_max[2] = s.sys.shields[2] = 22;
    s.sys.shields_max[3] = s.sys.shields[3] = 22;
    s.sys.shields_max[4] = s.sys.shields[4] = 30;
    s.sys.shields_max[5] = s.sys.shields[5] = 26;
    s.sys.hull = s.sys.hull_max = 38;
    s.dac = dac_for_hull(40);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"), make_ph1("Ph-1 C"),
        make_ph1("Ph-1 D"), make_ph1("Ph-1 E"), make_ph1("Ph-1 F"),
        make_ph2("Ph-2 1"), make_ph2("Ph-2 2"),
        make_ph3("Ph-3 1"), make_ph3("Ph-3 2"),
        make_photon("Photon A"), make_photon("Photon B"), make_photon("Photon C"),
        make_photon("Photon D"), make_photon("Photon E"), make_photon("Photon F"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Strike Carrier (CVS) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_cvs(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 24;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 24; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",12,12},{"R.Warp",12,12}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 4;
    s.sys.battery_cap = 6;
    s.sys.crew_total = 10; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 30;
    s.sys.shields_max[1] = s.sys.shields[1] = 22;
    s.sys.shields_max[2] = s.sys.shields[2] = 16;
    s.sys.shields_max[3] = s.sys.shields[3] = 16;
    s.sys.shields_max[4] = s.sys.shields[4] = 22;
    s.sys.shields_max[5] = s.sys.shields[5] = 18;
    s.sys.hull = s.sys.hull_max = 30;
    s.dac = dac_for_hull(30);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"), make_ph1("Ph-1 C"), make_ph1("Ph-1 D"),
        make_ph3("Ph-3 1"), make_ph3("Ph-3 2"),
        make_photon("Photon A"), make_photon("Photon B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Light Battle Tug (LBT) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_lbt(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 28;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 28; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",14,14},{"R.Warp",14,14}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 4;
    s.sys.battery_cap = 6;
    s.sys.crew_total = 10; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 30;
    s.sys.shields_max[1] = s.sys.shields[1] = 22;
    s.sys.shields_max[2] = s.sys.shields[2] = 16;
    s.sys.shields_max[3] = s.sys.shields[3] = 16;
    s.sys.shields_max[4] = s.sys.shields[4] = 22;
    s.sys.shields_max[5] = s.sys.shields[5] = 18;
    s.sys.hull = s.sys.hull_max = 48;
    s.dac = dac_for_hull(50);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"), make_ph1("Ph-1 C"), make_ph1("Ph-1 D"),
        make_photon("Photon A"), make_photon("Photon B"),
        make_drone("Drone Rack 1"), make_drone("Drone Rack 2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ War Destroyer Aegis Escort (DWA) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_dwa(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 16;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 16; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",8,8},{"R.Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 3;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 10; s.sys.shuttles = 3;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 14;
    s.sys.shields_max[2] = s.sys.shields[2] = 10;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 10;
    s.sys.shields_max[5] = s.sys.shields[5] = 14;
    s.sys.hull = s.sys.hull_max = 20;
    s.dac = dac_for_hull(20);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_gatling("Ph-G 1"), make_gatling("Ph-G 2"),
        make_drone("Drone Rack G1"), make_drone("Drone Rack G2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Heavy Cruiser Plus (CA+) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_ca_plus(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 24;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 24; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",12,12},{"R.Warp",12,12}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 6;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 10; s.sys.shuttles = 4;
    s.sys.shields_max[0] = s.sys.shields[0] = 32;
    s.sys.shields_max[1] = s.sys.shields[1] = 24;
    s.sys.shields_max[2] = s.sys.shields[2] = 18;
    s.sys.shields_max[3] = s.sys.shields[3] = 18;
    s.sys.shields_max[4] = s.sys.shields[4] = 24;
    s.sys.shields_max[5] = s.sys.shields[5] = 20;
    s.sys.hull = s.sys.hull_max = 30;
    s.dac = dac_for_hull(30);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"), make_ph1("Ph-1 C"), make_ph1("Ph-1 D"),
        make_ph3("Ph-3 1"), make_ph3("Ph-3 2"), make_ph3("Ph-3 3"), make_ph3("Ph-3 4"),
        make_photon("Photon A"), make_photon("Photon B"),
        make_photon("Photon C"), make_photon("Photon D"),
        make_drone("Drone Rack 1"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Heavy Command Cruiser (HCC) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_hcc(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 24;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 24; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",12,12},{"R.Warp",12,12}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 6;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 10; s.sys.shuttles = 6;
    s.sys.shields_max[0] = s.sys.shields[0] = 32;
    s.sys.shields_max[1] = s.sys.shields[1] = 24;
    s.sys.shields_max[2] = s.sys.shields[2] = 18;
    s.sys.shields_max[3] = s.sys.shields[3] = 18;
    s.sys.shields_max[4] = s.sys.shields[4] = 24;
    s.sys.shields_max[5] = s.sys.shields[5] = 20;
    s.sys.hull = s.sys.hull_max = 30;
    s.dac = dac_for_hull(30);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"), make_ph1("Ph-1 C"),
        make_ph1("Ph-1 D"), make_ph1("Ph-1 E"), make_ph1("Ph-1 F"),
        make_ph3("Ph-3 1"), make_ph3("Ph-3 2"), make_ph3("Ph-3 3"), make_ph3("Ph-3 4"),
        make_photon("Photon A"), make_photon("Photon B"),
        make_photon("Photon C"), make_photon("Photon D"),
        make_drone("Drone Rack 1"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Plasma Frigate (FFL) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_ffl(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 12;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 12; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",6,6},{"R.Warp",6,6}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 0;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 10; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 14;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 8;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 16;
    s.dac = dac_for_hull(15);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_ph3("Ph-3 1"), make_ph3("Ph-3 2"),
        make_plasma_f("Plasma-F 1"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ New Light Carrier (NVL) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_nvl(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 26;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 26; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",13,13},{"R.Warp",13,13}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 4;
    s.sys.battery_cap = 6;
    s.sys.crew_total = 10; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 24;
    s.sys.shields_max[1] = s.sys.shields[1] = 20;
    s.sys.shields_max[2] = s.sys.shields[2] = 16;
    s.sys.shields_max[3] = s.sys.shields[3] = 16;
    s.sys.shields_max[4] = s.sys.shields[4] = 16;
    s.sys.shields_max[5] = s.sys.shields[5] = 20;
    s.sys.hull = s.sys.hull_max = 38;
    s.dac = dac_for_hull(40);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_ph3("Ph-3 1"), make_ph3("Ph-3 2"),
        make_ph3("Ph-3 3"), make_ph3("Ph-3 4"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Drone Frigate (FFD) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_ffd(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 12;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 12; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",6,6},{"R.Warp",6,6}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 0;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 10; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 14;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 8;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 16;
    s.dac = dac_for_hull(15);
    s.weapons = {
        make_ph3("Ph-3 1"), make_ph3("Ph-3 2"),
        make_drone("Drone Rack 1"), make_drone("Drone Rack 2"),
        make_drone("Drone Rack 3"), make_drone("Drone Rack 4"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Dreadnought Plus (DN+) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_dn_plus(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 34;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 34; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",17,17},{"R.Warp",17,17}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 8;
    s.sys.battery_cap = 6;
    s.sys.crew_total = 10; s.sys.shuttles = 6;
    s.sys.shields_max[0] = s.sys.shields[0] = 42;
    s.sys.shields_max[1] = s.sys.shields[1] = 32;
    s.sys.shields_max[2] = s.sys.shields[2] = 24;
    s.sys.shields_max[3] = s.sys.shields[3] = 24;
    s.sys.shields_max[4] = s.sys.shields[4] = 32;
    s.sys.shields_max[5] = s.sys.shields[5] = 28;
    s.sys.hull = s.sys.hull_max = 40;
    s.dac = dac_for_hull(40);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"), make_ph1("Ph-1 C"),
        make_ph1("Ph-1 D"), make_ph1("Ph-1 E"), make_ph1("Ph-1 F"),
        make_ph3("Ph-3 1"), make_ph3("Ph-3 2"), make_ph3("Ph-3 3"), make_ph3("Ph-3 4"),
        make_photon("Photon A"), make_photon("Photon B"), make_photon("Photon C"),
        make_photon("Photon D"), make_photon("Photon E"), make_photon("Photon F"),
        make_drone("Drone Rack 1"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Heavy Fighter Transport (NVH) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_nvh(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 24;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 24; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",12,12},{"R.Warp",12,12}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 4;
    s.sys.battery_cap = 6;
    s.sys.crew_total = 10; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 24;
    s.sys.shields_max[1] = s.sys.shields[1] = 20;
    s.sys.shields_max[2] = s.sys.shields[2] = 16;
    s.sys.shields_max[3] = s.sys.shields[3] = 16;
    s.sys.shields_max[4] = s.sys.shields[4] = 16;
    s.sys.shields_max[5] = s.sys.shields[5] = 20;
    s.sys.hull = s.sys.hull_max = 35;
    s.dac = dac_for_hull(35);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"), make_ph1("Ph-1 C"), make_ph1("Ph-1 D"),
        make_gatling("Ph-G 1"), make_gatling("Ph-G 2"),
        make_photon("Photon A"), make_photon("Photon B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Heavy Cruiser Rear Phaser (CAR) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_car(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 24;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 24; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",12,12},{"R.Warp",12,12}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 6;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 10; s.sys.shuttles = 4;
    s.sys.shields_max[0] = s.sys.shields[0] = 30;
    s.sys.shields_max[1] = s.sys.shields[1] = 22;
    s.sys.shields_max[2] = s.sys.shields[2] = 16;
    s.sys.shields_max[3] = s.sys.shields[3] = 16;
    s.sys.shields_max[4] = s.sys.shields[4] = 22;
    s.sys.shields_max[5] = s.sys.shields[5] = 18;
    s.sys.hull = s.sys.hull_max = 30;
    s.dac = dac_for_hull(30);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"), make_ph1("Ph-1 C"), make_ph1("Ph-1 D"),
        make_ph1("Ph-1 RH1"), make_ph1("Ph-1 RH2"),
        make_ph3("Ph-3 1"), make_ph3("Ph-3 2"),
        make_photon("Photon A"), make_photon("Photon B"),
        make_photon("Photon C"), make_photon("Photon D"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Space Control Ship (SCS) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_scs(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 28;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 28; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",14,14},{"R.Warp",14,14}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 4;
    s.sys.battery_cap = 8;
    s.sys.crew_total = 10; s.sys.shuttles = 4;
    s.sys.shields_max[0] = s.sys.shields[0] = 34;
    s.sys.shields_max[1] = s.sys.shields[1] = 26;
    s.sys.shields_max[2] = s.sys.shields[2] = 18;
    s.sys.shields_max[3] = s.sys.shields[3] = 18;
    s.sys.shields_max[4] = s.sys.shields[4] = 26;
    s.sys.shields_max[5] = s.sys.shields[5] = 22;
    s.sys.hull = s.sys.hull_max = 36;
    s.dac = dac_for_hull(35);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"), make_ph1("Ph-1 C"),
        make_ph1("Ph-1 D"), make_ph1("Ph-1 E"), make_ph1("Ph-1 F"),
        make_ph2("Ph-2 1"), make_ph2("Ph-2 2"),
        make_ph3("Ph-3 1"), make_ph3("Ph-3 2"),
        make_photon("Photon A"), make_photon("Photon B"),
        make_photon("Photon C"), make_photon("Photon D"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ War Destroyer Command (DWC) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_dwc(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 16;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 16; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",8,8},{"R.Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 3;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 10; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 14;
    s.sys.shields_max[2] = s.sys.shields[2] = 10;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 10;
    s.sys.shields_max[5] = s.sys.shields[5] = 14;
    s.sys.hull = s.sys.hull_max = 22;
    s.dac = dac_for_hull(20);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"), make_ph1("Ph-1 C"), make_ph1("Ph-1 D"),
        make_gatling("Ph-G 1"), make_gatling("Ph-G 2"),
        make_photon("Photon A"), make_photon("Photon B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Battle Control Ship (BCS) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_bcs(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 32;
    s.sys.max_impulse_power = 6;
    s.eaf.warp_power = 32; s.eaf.impulse_power = 6;
    s.sys.warp_groups    = {{"L.Warp",16,16},{"R.Warp",16,16}};
    s.sys.impulse_groups = {{"Impulse",6,6}};
    s.sys.apr_rated = s.sys.apr_current = 6;
    s.sys.battery_cap = 10;
    s.sys.crew_total = 10; s.sys.shuttles = 6;
    s.sys.shields_max[0] = s.sys.shields[0] = 40;
    s.sys.shields_max[1] = s.sys.shields[1] = 32;
    s.sys.shields_max[2] = s.sys.shields[2] = 24;
    s.sys.shields_max[3] = s.sys.shields[3] = 24;
    s.sys.shields_max[4] = s.sys.shields[4] = 32;
    s.sys.shields_max[5] = s.sys.shields[5] = 28;
    s.sys.hull = s.sys.hull_max = 40;
    s.dac = dac_for_hull(40);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"), make_ph1("Ph-1 C"),
        make_ph1("Ph-1 D"), make_ph1("Ph-1 E"), make_ph1("Ph-1 F"),
        make_ph2("Ph-2 1"), make_ph2("Ph-2 2"),
        make_ph3("Ph-3 1"), make_ph3("Ph-3 2"),
        make_photon("Photon A"), make_photon("Photon B"), make_photon("Photon C"),
        make_photon("Photon D"), make_photon("Photon E"), make_photon("Photon F"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Heavy Command Cruiser (CB) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_cb(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 28;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 28; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",14,14},{"R.Warp",14,14}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 4;
    s.sys.battery_cap = 8;
    s.sys.crew_total = 10; s.sys.shuttles = 6;
    s.sys.shields_max[0] = s.sys.shields[0] = 36;
    s.sys.shields_max[1] = s.sys.shields[1] = 28;
    s.sys.shields_max[2] = s.sys.shields[2] = 20;
    s.sys.shields_max[3] = s.sys.shields[3] = 20;
    s.sys.shields_max[4] = s.sys.shields[4] = 28;
    s.sys.shields_max[5] = s.sys.shields[5] = 24;
    s.sys.hull = s.sys.hull_max = 36;
    s.dac = dac_for_hull(35);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"), make_ph1("Ph-1 C"),
        make_ph1("Ph-1 D"), make_ph1("Ph-1 E"), make_ph1("Ph-1 F"),
        make_ph2("Ph-2 1"), make_ph2("Ph-2 2"),
        make_ph3("Ph-3 1"), make_ph3("Ph-3 2"),
        make_photon("Photon A"), make_photon("Photon B"),
        make_photon("Photon C"), make_photon("Photon D"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ War Destroyer Transport (DWT) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_dwt(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 16;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 16; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",8,8},{"R.Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 3;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 10; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 14;
    s.sys.shields_max[2] = s.sys.shields[2] = 10;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 10;
    s.sys.shields_max[5] = s.sys.shields[5] = 14;
    s.sys.hull = s.sys.hull_max = 20;
    s.dac = dac_for_hull(20);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_ph3("Ph-3 1"), make_ph3("Ph-3 2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Commando Carrier (COV) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_cmc_cov(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 22;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 22; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",11,11},{"R.Warp",11,11}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 4;
    s.sys.battery_cap = 6;
    s.sys.crew_total = 10; s.sys.shuttles = 12;
    s.sys.shields_max[0] = s.sys.shields[0] = 28;
    s.sys.shields_max[1] = s.sys.shields[1] = 20;
    s.sys.shields_max[2] = s.sys.shields[2] = 14;
    s.sys.shields_max[3] = s.sys.shields[3] = 14;
    s.sys.shields_max[4] = s.sys.shields[4] = 20;
    s.sys.shields_max[5] = s.sys.shields[5] = 16;
    s.sys.hull = s.sys.hull_max = 30;
    s.dac = dac_for_hull(30);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_ph3("Ph-3 1"), make_ph3("Ph-3 2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ New Strike Carrier (NVS) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_nvs(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 26;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 26; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",13,13},{"R.Warp",13,13}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 4;
    s.sys.battery_cap = 6;
    s.sys.crew_total = 10; s.sys.shuttles = 3;
    s.sys.shields_max[0] = s.sys.shields[0] = 24;
    s.sys.shields_max[1] = s.sys.shields[1] = 20;
    s.sys.shields_max[2] = s.sys.shields[2] = 16;
    s.sys.shields_max[3] = s.sys.shields[3] = 16;
    s.sys.shields_max[4] = s.sys.shields[4] = 16;
    s.sys.shields_max[5] = s.sys.shields[5] = 20;
    s.sys.hull = s.sys.hull_max = 42;
    s.dac = dac_for_hull(40);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"), make_ph1("Ph-1 C"), make_ph1("Ph-1 D"),
        make_gatling("Ph-G 1"), make_gatling("Ph-G 2"),
        make_photon("Photon A"), make_photon("Photon B"),
        make_drone("Drone Rack 1"), make_drone("Drone Rack 2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Galactic Survey Cruiser (GSC) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_gsc(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 22;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 22; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",11,11},{"R.Warp",11,11}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 4;
    s.sys.battery_cap = 6;
    s.sys.crew_total = 10; s.sys.shuttles = 6;
    s.sys.shields_max[0] = s.sys.shields[0] = 26;
    s.sys.shields_max[1] = s.sys.shields[1] = 18;
    s.sys.shields_max[2] = s.sys.shields[2] = 12;
    s.sys.shields_max[3] = s.sys.shields[3] = 12;
    s.sys.shields_max[4] = s.sys.shields[4] = 18;
    s.sys.shields_max[5] = s.sys.shields[5] = 14;
    s.sys.hull = s.sys.hull_max = 28;
    s.dac = dac_for_hull(30);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_ph3("Ph-3 1"), make_ph3("Ph-3 2"),
        make_photon("Photon A"), make_photon("Photon B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Light Survey Cruiser (CLS) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_cls(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 22;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 22; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",11,11},{"R.Warp",11,11}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 4;
    s.sys.battery_cap = 6;
    s.sys.crew_total = 10; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 20;
    s.sys.shields_max[1] = s.sys.shields[1] = 16;
    s.sys.shields_max[2] = s.sys.shields[2] = 12;
    s.sys.shields_max[3] = s.sys.shields[3] = 12;
    s.sys.shields_max[4] = s.sys.shields[4] = 12;
    s.sys.shields_max[5] = s.sys.shields[5] = 16;
    s.sys.hull = s.sys.hull_max = 35;
    s.dac = dac_for_hull(35);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_ph3("Ph-3 1"), make_ph3("Ph-3 2"),
        make_drone("Drone Rack 1"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ FFG Frigate (FFG) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_ffg(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 14;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 14; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",7,7},{"R.Warp",7,7}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 10; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 16;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 8;
    s.sys.shields_max[4] = s.sys.shields[4] = 12;
    s.sys.shields_max[5] = s.sys.shields[5] = 10;
    s.sys.hull = s.sys.hull_max = 16;
    s.dac = dac_for_hull(15);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_ph3("Ph-3 1"), make_ph3("Ph-3 2"),
        make_photon("Photon A"), make_photon("Photon B"),
        make_drone("Drone 1"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ New Escort-R Cruiser (NER) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_ner(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 26;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 26; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",13,13},{"R.Warp",13,13}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 4;
    s.sys.battery_cap = 6;
    s.sys.crew_total = 10; s.sys.shuttles = 5;
    s.sys.shields_max[0] = s.sys.shields[0] = 24;
    s.sys.shields_max[1] = s.sys.shields[1] = 20;
    s.sys.shields_max[2] = s.sys.shields[2] = 16;
    s.sys.shields_max[3] = s.sys.shields[3] = 16;
    s.sys.shields_max[4] = s.sys.shields[4] = 16;
    s.sys.shields_max[5] = s.sys.shields[5] = 20;
    s.sys.hull = s.sys.hull_max = 38;
    s.dac = dac_for_hull(40);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"), make_ph1("Ph-1 C"), make_ph1("Ph-1 D"),
        make_gatling("Ph-G 1"), make_gatling("Ph-G 2"),
        make_photon("Photon A"), make_photon("Photon B"),
        make_drone("Drone Rack G1"), make_drone("Drone Rack G2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Escort Cruiser (ECL) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_ecl(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 18;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 18; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",9,9},{"R.Warp",9,9}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 4;
    s.sys.battery_cap = 6;
    s.sys.crew_total = 10; s.sys.shuttles = 3;
    s.sys.shields_max[0] = s.sys.shields[0] = 22;
    s.sys.shields_max[1] = s.sys.shields[1] = 16;
    s.sys.shields_max[2] = s.sys.shields[2] = 10;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 16;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 24;
    s.dac = dac_for_hull(25);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"), make_ph1("Ph-1 C"), make_ph1("Ph-1 D"),
        make_ph3("Ph-3 1"), make_ph3("Ph-3 2"),
        make_photon("Photon A"), make_photon("Photon B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ War Destroyer Escort (DWE) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_dwe(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 16;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 16; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",8,8},{"R.Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 3;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 10; s.sys.shuttles = 3;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 14;
    s.sys.shields_max[2] = s.sys.shields[2] = 10;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 10;
    s.sys.shields_max[5] = s.sys.shields[5] = 14;
    s.sys.hull = s.sys.hull_max = 20;
    s.dac = dac_for_hull(20);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_gatling("Ph-G 1"), make_gatling("Ph-G 2"),
        make_drone("Drone Rack G1"), make_drone("Drone Rack G2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Escort Carrier Frigate (FFV) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_ffv(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 14;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 14; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",7,7},{"R.Warp",7,7}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 0;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 10; s.sys.shuttles = 3;
    s.sys.shields_max[0] = s.sys.shields[0] = 14;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 8;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 20;
    s.dac = dac_for_hull(20);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_ph3("Ph-3 1"), make_ph3("Ph-3 2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Destroyer Escort (DE) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_de(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 12;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 12; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",6,6},{"R.Warp",6,6}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 10; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 16;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 8;
    s.sys.shields_max[4] = s.sys.shields[4] = 12;
    s.sys.shields_max[5] = s.sys.shields[5] = 10;
    s.sys.hull = s.sys.hull_max = 18;
    s.dac = dac_for_hull(20);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_ph3("Ph-3 1"), make_ph3("Ph-3 2"),
        make_photon("Photon A"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Destroyer Scout (SC) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_scout_dd(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 14;
    s.sys.max_impulse_power = 6;
    s.eaf.warp_power = 14; s.eaf.impulse_power = 6;
    s.sys.warp_groups    = {{"L.Warp",7,7},{"R.Warp",7,7}};
    s.sys.impulse_groups = {{"Impulse",6,6}};
    s.sys.apr_rated = s.sys.apr_current = 4;
    s.sys.battery_cap = 3;
    s.sys.crew_total = 10; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 8;
    s.sys.shields_max[4] = s.sys.shields[4] = 12;
    s.sys.shields_max[5] = s.sys.shields[5] = 10;
    s.sys.hull = s.sys.hull_max = 20;
    s.dac = dac_for_hull(20);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_ph3("Ph-3 1"), make_ph3("Ph-3 2"),
        make_photon("Photon A"), make_photon("Photon B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Fast Cruiser (CF) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_cf(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 28;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 28; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",14,14},{"R.Warp",14,14}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 4;
    s.sys.battery_cap = 6;
    s.sys.crew_total = 10; s.sys.shuttles = 4;
    s.sys.shields_max[0] = s.sys.shields[0] = 28;
    s.sys.shields_max[1] = s.sys.shields[1] = 20;
    s.sys.shields_max[2] = s.sys.shields[2] = 14;
    s.sys.shields_max[3] = s.sys.shields[3] = 14;
    s.sys.shields_max[4] = s.sys.shields[4] = 20;
    s.sys.shields_max[5] = s.sys.shields[5] = 16;
    s.sys.hull = s.sys.hull_max = 30;
    s.dac = dac_for_hull(30);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"), make_ph1("Ph-1 C"), make_ph1("Ph-1 D"),
        make_ph2("Ph-2 1"), make_ph2("Ph-2 2"),
        make_ph3("Ph-3 1"), make_ph3("Ph-3 2"),
        make_photon("Photon A"), make_photon("Photon B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Light Dreadnought (DNL) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_dnl(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 30;
    s.sys.max_impulse_power = 6;
    s.eaf.warp_power = 30; s.eaf.impulse_power = 6;
    s.sys.warp_groups    = {{"L.Warp",15,15},{"R.Warp",15,15}};
    s.sys.impulse_groups = {{"Impulse",6,6}};
    s.sys.apr_rated = s.sys.apr_current = 5;
    s.sys.battery_cap = 8;
    s.sys.crew_total = 10; s.sys.shuttles = 6;
    s.sys.shields_max[0] = s.sys.shields[0] = 36;
    s.sys.shields_max[1] = s.sys.shields[1] = 28;
    s.sys.shields_max[2] = s.sys.shields[2] = 20;
    s.sys.shields_max[3] = s.sys.shields[3] = 20;
    s.sys.shields_max[4] = s.sys.shields[4] = 28;
    s.sys.shields_max[5] = s.sys.shields[5] = 24;
    s.sys.hull = s.sys.hull_max = 36;
    s.dac = dac_for_hull(35);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"), make_ph1("Ph-1 C"),
        make_ph1("Ph-1 D"), make_ph1("Ph-1 E"), make_ph1("Ph-1 F"),
        make_ph2("Ph-2 1"), make_ph2("Ph-2 2"),
        make_ph3("Ph-3 1"), make_ph3("Ph-3 2"),
        make_photon("Photon A"), make_photon("Photon B"),
        make_photon("Photon C"), make_photon("Photon D"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Space Control Ship (SCSA) - Conjectural PF variant â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_scsa(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 12;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 12; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",6,6},{"R.Warp",6,6}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 4;
    s.sys.battery_cap = 6;
    s.sys.crew_total = 10; s.sys.shuttles = 6;
    s.sys.shields_max[0] = s.sys.shields[0] = 34;
    s.sys.shields_max[1] = s.sys.shields[1] = 26;
    s.sys.shields_max[2] = s.sys.shields[2] = 18;
    s.sys.shields_max[3] = s.sys.shields[3] = 18;
    s.sys.shields_max[4] = s.sys.shields[4] = 26;
    s.sys.shields_max[5] = s.sys.shields[5] = 22;
    s.sys.hull = s.sys.hull_max = 55;
    s.dac = dac_for_hull(55);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"), make_ph1("Ph-1 C"),
        make_ph1("Ph-1 D"), make_ph1("Ph-1 E"), make_ph1("Ph-1 F"),
        make_gatling("Ph-G 1"), make_gatling("Ph-G 2"),
        make_photon("Photon A"), make_photon("Photon B"),
        make_photon("Photon C"), make_photon("Photon D"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Hospital Ship (CLH) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_clh(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 20;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 20; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",10,10},{"R.Warp",10,10}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 4;
    s.sys.battery_cap = 6;
    s.sys.crew_total = 10; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 20;
    s.sys.shields_max[1] = s.sys.shields[1] = 16;
    s.sys.shields_max[2] = s.sys.shields[2] = 12;
    s.sys.shields_max[3] = s.sys.shields[3] = 12;
    s.sys.shields_max[4] = s.sys.shields[4] = 12;
    s.sys.shields_max[5] = s.sys.shields[5] = 16;
    s.sys.hull = s.sys.hull_max = 40;
    s.dac = dac_for_hull(40);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_ph3("Ph-3 1"), make_ph3("Ph-3 2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Scout Frigate (FFS) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_ffs(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 14;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 14; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",7,7},{"R.Warp",7,7}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 10; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 14;
    s.sys.shields_max[1] = s.sys.shields[1] = 10;
    s.sys.shields_max[2] = s.sys.shields[2] = 6;
    s.sys.shields_max[3] = s.sys.shields[3] = 6;
    s.sys.shields_max[4] = s.sys.shields[4] = 10;
    s.sys.shields_max[5] = s.sys.shields[5] = 8;
    s.sys.hull = s.sys.hull_max = 16;
    s.dac = dac_for_hull(15);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_ph3("Ph-3 1"), make_ph3("Ph-3 2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Aegis Frigate (FFA) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_ffa(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 14;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 14; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",7,7},{"R.Warp",7,7}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 10; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 16;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 8;
    s.sys.shields_max[4] = s.sys.shields[4] = 12;
    s.sys.shields_max[5] = s.sys.shields[5] = 10;
    s.sys.hull = s.sys.hull_max = 16;
    s.dac = dac_for_hull(15);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_ph3("Ph-3 1"), make_ph3("Ph-3 2"),
        make_photon("Photon A"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Destroyer Escort-R (DER) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_der(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 16;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 16; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",8,8},{"R.Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 0;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 10; s.sys.shuttles = 5;
    s.sys.shields_max[0] = s.sys.shields[0] = 16;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 8;
    s.sys.shields_max[4] = s.sys.shields[4] = 12;
    s.sys.shields_max[5] = s.sys.shields[5] = 10;
    s.sys.hull = s.sys.hull_max = 22;
    s.dac = dac_for_hull(20);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_gatling("Ph-G 1"), make_gatling("Ph-G 2"),
        make_drone("Drone Rack G1"), make_drone("Drone Rack G2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ New Attack Carrier (NCVA) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_ncva(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 28;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 28; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",14,14},{"R.Warp",14,14}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 6;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 10; s.sys.shuttles = 6;
    s.sys.shields_max[0] = s.sys.shields[0] = 36;
    s.sys.shields_max[1] = s.sys.shields[1] = 26;
    s.sys.shields_max[2] = s.sys.shields[2] = 20;
    s.sys.shields_max[3] = s.sys.shields[3] = 20;
    s.sys.shields_max[4] = s.sys.shields[4] = 26;
    s.sys.shields_max[5] = s.sys.shields[5] = 22;
    s.sys.hull = s.sys.hull_max = 35;
    s.dac = dac_for_hull(35);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"), make_ph1("Ph-1 C"), make_ph1("Ph-1 D"),
        make_gatling("Ph-G 1"), make_gatling("Ph-G 2"), make_gatling("Ph-G 3"), make_gatling("Ph-G 4"),
        make_ph1("Ph-1 360-1"), make_ph1("Ph-1 360-2"),
        make_photon("Photon A"), make_photon("Photon B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Aegis Frigate-R (FRA) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_fra(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 12;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 12; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",6,6},{"R.Warp",6,6}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 0;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 10; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 14;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 8;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 16;
    s.dac = dac_for_hull(15);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_ph3("Ph-3 1"), make_ph3("Ph-3 2"),
        make_drone("Drone Rack G1"), make_drone("Drone Rack G2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ New Escort Cruiser (NEC) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_nec(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 20;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 20; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",10,10},{"R.Warp",10,10}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 4;
    s.sys.battery_cap = 6;
    s.sys.crew_total = 10; s.sys.shuttles = 3;
    s.sys.shields_max[0] = s.sys.shields[0] = 28;
    s.sys.shields_max[1] = s.sys.shields[1] = 20;
    s.sys.shields_max[2] = s.sys.shields[2] = 14;
    s.sys.shields_max[3] = s.sys.shields[3] = 14;
    s.sys.shields_max[4] = s.sys.shields[4] = 20;
    s.sys.shields_max[5] = s.sys.shields[5] = 16;
    s.sys.hull = s.sys.hull_max = 28;
    s.dac = dac_for_hull(30);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"), make_ph1("Ph-1 C"), make_ph1("Ph-1 D"),
        make_ph3("Ph-3 1"), make_ph3("Ph-3 2"),
        make_photon("Photon A"), make_photon("Photon B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ War Drone Destroyer (DWD) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_dwd(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 16;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 16; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",8,8},{"R.Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 3;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 10; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 14;
    s.sys.shields_max[2] = s.sys.shields[2] = 10;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 10;
    s.sys.shields_max[5] = s.sys.shields[5] = 14;
    s.sys.hull = s.sys.hull_max = 20;
    s.dac = dac_for_hull(20);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_drone("Drone Rack 1"), make_drone("Drone Rack 2"),
        make_drone("Drone Rack 3"), make_drone("Drone Rack 4"),
        make_drone("Drone Rack 5"), make_drone("Drone Rack 6"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Destroyer Aegis-R (DAR) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_dar(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 16;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 16; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",8,8},{"R.Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 0;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 10; s.sys.shuttles = 5;
    s.sys.shields_max[0] = s.sys.shields[0] = 16;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 8;
    s.sys.shields_max[4] = s.sys.shields[4] = 12;
    s.sys.shields_max[5] = s.sys.shields[5] = 10;
    s.sys.hull = s.sys.hull_max = 22;
    s.dac = dac_for_hull(20);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_gatling("Ph-G 1"), make_gatling("Ph-G 2"),
        make_drone("Drone Rack G1"), make_drone("Drone Rack G2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ DEA Aegis Destroyer (DE refit Y175) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_dea(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 12;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 12; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",6,6},{"R.Warp",6,6}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 0;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 10; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 16;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 8;
    s.sys.shields_max[4] = s.sys.shields[4] = 12;
    s.sys.shields_max[5] = s.sys.shields[5] = 10;
    s.sys.hull = s.sys.hull_max = 18;
    s.dac = dac_for_hull(20);
    s.weapons = {
        make_gatling("Ph-G FA1"), make_gatling("Ph-G FA2"),
        make_ph3("Ph-3 1"), make_ph3("Ph-3 2"),
        make_photon("Photon A"), make_photon("Photon B"),
        make_drone("Drone Rack 1"), make_drone("Drone Rack 2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ New Aegis Cruiser (NAC) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_nac(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 20;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 20; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",10,10},{"R.Warp",10,10}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 4;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 10; s.sys.shuttles = 4;
    s.sys.shields_max[0] = s.sys.shields[0] = 28;
    s.sys.shields_max[1] = s.sys.shields[1] = 20;
    s.sys.shields_max[2] = s.sys.shields[2] = 16;
    s.sys.shields_max[3] = s.sys.shields[3] = 16;
    s.sys.shields_max[4] = s.sys.shields[4] = 20;
    s.sys.shields_max[5] = s.sys.shields[5] = 18;
    s.sys.hull = s.sys.hull_max = 25;
    s.dac = dac_for_hull(25);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_gatling("Ph-G 1"), make_gatling("Ph-G 2"),
        make_gatling("Ph-G 3"), make_gatling("Ph-G 4"),
        make_ph3("Ph-3 1"), make_ph3("Ph-3 2"),
        make_photon("Photon A"), make_photon("Photon B"),
        make_drone("Drone Rack 1"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ New Light Cruiser (NCL) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_ncl(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 22;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 22; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",11,11},{"R.Warp",11,11}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 4;
    s.sys.battery_cap = 6;
    s.sys.crew_total = 10; s.sys.shuttles = 3;
    s.sys.shields_max[0] = s.sys.shields[0] = 28;
    s.sys.shields_max[1] = s.sys.shields[1] = 20;
    s.sys.shields_max[2] = s.sys.shields[2] = 14;
    s.sys.shields_max[3] = s.sys.shields[3] = 14;
    s.sys.shields_max[4] = s.sys.shields[4] = 20;
    s.sys.shields_max[5] = s.sys.shields[5] = 16;
    s.sys.hull = s.sys.hull_max = 28;
    s.dac = dac_for_hull(30);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"), make_ph1("Ph-1 C"), make_ph1("Ph-1 D"),
        make_ph2("Ph-2 1"), make_ph2("Ph-2 2"),
        make_ph3("Ph-3 1"),
        make_photon("Photon A"), make_photon("Photon B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Escort Frigate (FFE) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_ffe(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 14;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 14; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",7,7},{"R.Warp",7,7}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 10; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 16;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 8;
    s.sys.shields_max[4] = s.sys.shields[4] = 12;
    s.sys.shields_max[5] = s.sys.shields[5] = 10;
    s.sys.hull = s.sys.hull_max = 16;
    s.dac = dac_for_hull(15);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_ph3("Ph-3 1"), make_ph3("Ph-3 2"),
        make_photon("Photon A"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ New Heavy Cruiser (NCA) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_nca(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 28;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 28; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",14,14},{"R.Warp",14,14}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 4;
    s.sys.battery_cap = 6;
    s.sys.crew_total = 10; s.sys.shuttles = 4;
    s.sys.shields_max[0] = s.sys.shields[0] = 34;
    s.sys.shields_max[1] = s.sys.shields[1] = 26;
    s.sys.shields_max[2] = s.sys.shields[2] = 18;
    s.sys.shields_max[3] = s.sys.shields[3] = 18;
    s.sys.shields_max[4] = s.sys.shields[4] = 26;
    s.sys.shields_max[5] = s.sys.shields[5] = 22;
    s.sys.hull = s.sys.hull_max = 36;
    s.dac = dac_for_hull(35);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"), make_ph1("Ph-1 C"), make_ph1("Ph-1 D"),
        make_ph2("Ph-2 1"), make_ph2("Ph-2 2"),
        make_ph3("Ph-3 1"), make_ph3("Ph-3 2"),
        make_photon("Photon A"), make_photon("Photon B"),
        make_photon("Photon C"), make_photon("Photon D"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Battle Frigate (FFB) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_ffb(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 16;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 16; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",8,8},{"R.Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 3;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 10; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 14;
    s.sys.shields_max[2] = s.sys.shields[2] = 10;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 14;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 18;
    s.dac = dac_for_hull(20);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"), make_ph1("Ph-1 C"), make_ph1("Ph-1 D"),
        make_ph3("Ph-3 1"), make_ph3("Ph-3 2"),
        make_photon("Photon A"), make_photon("Photon B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ New Drone Cruiser (NCD) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_ncd(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 22;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 22; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",11,11},{"R.Warp",11,11}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 4;
    s.sys.battery_cap = 6;
    s.sys.crew_total = 10; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 28;
    s.sys.shields_max[1] = s.sys.shields[1] = 20;
    s.sys.shields_max[2] = s.sys.shields[2] = 14;
    s.sys.shields_max[3] = s.sys.shields[3] = 14;
    s.sys.shields_max[4] = s.sys.shields[4] = 20;
    s.sys.shields_max[5] = s.sys.shields[5] = 16;
    s.sys.hull = s.sys.hull_max = 36;
    s.dac = dac_for_hull(35);
    s.weapons = {
        make_ph3("Ph-3 1"), make_ph3("Ph-3 2"), make_ph3("Ph-3 3"), make_ph3("Ph-3 4"),
        make_drone("Drone Rack 1"), make_drone("Drone Rack 2"), make_drone("Drone Rack 3"),
        make_drone("Drone Rack 4"), make_drone("Drone Rack 5"), make_drone("Drone Rack 6"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ War Destroyer (DW) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_dw(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 16;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 16; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",8,8},{"R.Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 3;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 10; s.sys.shuttles = 3;
    s.sys.shields_max[0] = s.sys.shields[0] = 22;
    s.sys.shields_max[1] = s.sys.shields[1] = 16;
    s.sys.shields_max[2] = s.sys.shields[2] = 10;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 16;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 22;
    s.dac = dac_for_hull(20);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"), make_ph1("Ph-1 C"), make_ph1("Ph-1 D"),
        make_ph3("Ph-3 1"), make_ph3("Ph-3 2"),
        make_photon("Photon A"), make_photon("Photon B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Light Tactical Transport (LTT) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_ltt(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 22;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 22; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",11,11},{"R.Warp",11,11}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 4;
    s.sys.battery_cap = 6;
    s.sys.crew_total = 10; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 20;
    s.sys.shields_max[1] = s.sys.shields[1] = 16;
    s.sys.shields_max[2] = s.sys.shields[2] = 12;
    s.sys.shields_max[3] = s.sys.shields[3] = 12;
    s.sys.shields_max[4] = s.sys.shields[4] = 12;
    s.sys.shields_max[5] = s.sys.shields[5] = 16;
    s.sys.hull = s.sys.hull_max = 30;
    s.dac = dac_for_hull(30);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_ph3("Ph-3 1"), make_ph3("Ph-3 2"), make_ph3("Ph-3 3"), make_ph3("Ph-3 4"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ ACL Aegis Cruiser (ECL refit) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_acl(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 18;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 18; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",9,9},{"R.Warp",9,9}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 4;
    s.sys.battery_cap = 6;
    s.sys.crew_total = 10; s.sys.shuttles = 3;
    s.sys.shields_max[0] = s.sys.shields[0] = 22;
    s.sys.shields_max[1] = s.sys.shields[1] = 16;
    s.sys.shields_max[2] = s.sys.shields[2] = 10;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 16;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 24;
    s.dac = dac_for_hull(25);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"), make_ph1("Ph-1 C"), make_ph1("Ph-1 D"),
        make_gatling("Ph-G 1"), make_gatling("Ph-G 2"),
        make_photon("Photon A"), make_photon("Photon B"),
        make_drone("Drone 1"), make_drone("Drone 2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Escort Frigate-R (FFR) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_ffr(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 12;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 12; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",6,6},{"R.Warp",6,6}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 0;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 10; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 14;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 8;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 16;
    s.dac = dac_for_hull(15);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_ph3("Ph-3 1"), make_ph3("Ph-3 2"),
        make_drone("Drone Rack G1"), make_drone("Drone Rack G2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ War Destroyer Scout (DWS) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_dws(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 16;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 16; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",8,8},{"R.Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 3;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 10; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 14;
    s.sys.shields_max[2] = s.sys.shields[2] = 10;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 10;
    s.sys.shields_max[5] = s.sys.shields[5] = 14;
    s.sys.hull = s.sys.hull_max = 20;
    s.dac = dac_for_hull(20);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_gatling("Ph-G 1"), make_gatling("Ph-G 2"),
        make_photon("Photon A"), make_photon("Photon B"),
        make_drone("Drone Rack 1"), make_drone("Drone Rack 2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ New Heavy Command Cruiser (NHCC) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_nhcc(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 28;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 28; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",14,14},{"R.Warp",14,14}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 6;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 10; s.sys.shuttles = 6;
    s.sys.shields_max[0] = s.sys.shields[0] = 36;
    s.sys.shields_max[1] = s.sys.shields[1] = 28;
    s.sys.shields_max[2] = s.sys.shields[2] = 22;
    s.sys.shields_max[3] = s.sys.shields[3] = 22;
    s.sys.shields_max[4] = s.sys.shields[4] = 28;
    s.sys.shields_max[5] = s.sys.shields[5] = 24;
    s.sys.hull = s.sys.hull_max = 35;
    s.dac = dac_for_hull(35);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"), make_ph1("Ph-1 C"),
        make_ph1("Ph-1 D"), make_ph1("Ph-1 E"), make_ph1("Ph-1 F"),
        make_ph3("Ph-3 1"), make_ph3("Ph-3 2"), make_ph3("Ph-3 3"), make_ph3("Ph-3 4"),
        make_photon("Photon A"), make_photon("Photon B"),
        make_photon("Photon C"), make_photon("Photon D"),
        make_drone("Drone Rack 1"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Commando Cruiser (CMC) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_cmc(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 22;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 22; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",11,11},{"R.Warp",11,11}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 4;
    s.sys.battery_cap = 6;
    s.sys.crew_total = 10; s.sys.shuttles = 8;
    s.sys.shields_max[0] = s.sys.shields[0] = 28;
    s.sys.shields_max[1] = s.sys.shields[1] = 20;
    s.sys.shields_max[2] = s.sys.shields[2] = 14;
    s.sys.shields_max[3] = s.sys.shields[3] = 14;
    s.sys.shields_max[4] = s.sys.shields[4] = 20;
    s.sys.shields_max[5] = s.sys.shields[5] = 16;
    s.sys.hull = s.sys.hull_max = 30;
    s.dac = dac_for_hull(30);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"), make_ph1("Ph-1 C"), make_ph1("Ph-1 D"),
        make_ph3("Ph-3 1"), make_ph3("Ph-3 2"),
        make_photon("Photon A"), make_photon("Photon B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Battleship (BB) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_bb(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 40;
    s.sys.max_impulse_power = 6;
    s.eaf.warp_power = 40; s.eaf.impulse_power = 6;
    s.sys.warp_groups    = {{"L.Warp",20,20},{"R.Warp",20,20}};
    s.sys.impulse_groups = {{"Impulse",6,6}};
    s.sys.apr_rated = s.sys.apr_current = 6;
    s.sys.battery_cap = 10;
    s.sys.crew_total = 10; s.sys.shuttles = 8;
    s.sys.shields_max[0] = s.sys.shields[0] = 50;
    s.sys.shields_max[1] = s.sys.shields[1] = 40;
    s.sys.shields_max[2] = s.sys.shields[2] = 30;
    s.sys.shields_max[3] = s.sys.shields[3] = 30;
    s.sys.shields_max[4] = s.sys.shields[4] = 40;
    s.sys.shields_max[5] = s.sys.shields[5] = 36;
    s.sys.hull = s.sys.hull_max = 50;
    s.dac = dac_for_hull(50);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"), make_ph1("Ph-1 C"), make_ph1("Ph-1 D"),
        make_ph1("Ph-1 E"), make_ph1("Ph-1 F"), make_ph1("Ph-1 G"), make_ph1("Ph-1 H"),
        make_ph2("Ph-2 1"), make_ph2("Ph-2 2"),
        make_ph3("Ph-3 1"), make_ph3("Ph-3 2"),
        make_photon("Photon A"), make_photon("Photon B"), make_photon("Photon C"),
        make_photon("Photon D"), make_photon("Photon E"), make_photon("Photon F"),
        make_photon("Photon G"), make_photon("Photon H"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ New Jersey-class Battlecruiser (BCJ) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_bcj(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 34;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 34; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",17,17},{"R.Warp",17,17}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 5;
    s.sys.battery_cap = 8;
    s.sys.crew_total = 10; s.sys.shuttles = 3;
    s.sys.shields_max[0] = s.sys.shields[0] = 38;
    s.sys.shields_max[1] = s.sys.shields[1] = 30;
    s.sys.shields_max[2] = s.sys.shields[2] = 22;
    s.sys.shields_max[3] = s.sys.shields[3] = 22;
    s.sys.shields_max[4] = s.sys.shields[4] = 30;
    s.sys.shields_max[5] = s.sys.shields[5] = 26;
    s.sys.hull = s.sys.hull_max = 50;
    s.dac = dac_for_hull(50);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"), make_ph1("Ph-1 C"), make_ph1("Ph-1 D"),
        make_gatling("Ph-G 1"), make_gatling("Ph-G 2"),
        make_photon("Photon A"), make_photon("Photon B"),
        make_photon("Photon C"), make_photon("Photon D"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Light Command Cruiser (CLC) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_clc(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 24;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 24; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",12,12},{"R.Warp",12,12}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 4;
    s.sys.battery_cap = 6;
    s.sys.crew_total = 10; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 28;
    s.sys.shields_max[1] = s.sys.shields[1] = 20;
    s.sys.shields_max[2] = s.sys.shields[2] = 14;
    s.sys.shields_max[3] = s.sys.shields[3] = 14;
    s.sys.shields_max[4] = s.sys.shields[4] = 20;
    s.sys.shields_max[5] = s.sys.shields[5] = 16;
    s.sys.hull = s.sys.hull_max = 40;
    s.dac = dac_for_hull(40);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"), make_ph1("Ph-1 C"), make_ph1("Ph-1 D"),
        make_gatling("Ph-G 1"), make_gatling("Ph-G 2"),
        make_photon("Photon A"), make_photon("Photon B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ New Aegis Escort (NEA) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_nea(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 20;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 20; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",10,10},{"R.Warp",10,10}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 4;
    s.sys.battery_cap = 6;
    s.sys.crew_total = 10; s.sys.shuttles = 3;
    s.sys.shields_max[0] = s.sys.shields[0] = 28;
    s.sys.shields_max[1] = s.sys.shields[1] = 20;
    s.sys.shields_max[2] = s.sys.shields[2] = 14;
    s.sys.shields_max[3] = s.sys.shields[3] = 14;
    s.sys.shields_max[4] = s.sys.shields[4] = 20;
    s.sys.shields_max[5] = s.sys.shields[5] = 16;
    s.sys.hull = s.sys.hull_max = 28;
    s.dac = dac_for_hull(30);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"), make_ph1("Ph-1 C"), make_ph1("Ph-1 D"),
        make_ph3("Ph-3 1"), make_ph3("Ph-3 2"),
        make_photon("Photon A"), make_photon("Photon B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Guided Weapons Dreadnought (DNG) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_fed_dng(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 32;
    s.sys.max_impulse_power = 6;
    s.eaf.warp_power = 32; s.eaf.impulse_power = 6;
    s.sys.warp_groups    = {{"L.Warp",16,16},{"R.Warp",16,16}};
    s.sys.impulse_groups = {{"Impulse",6,6}};
    s.sys.apr_rated = s.sys.apr_current = 6;
    s.sys.battery_cap = 8;
    s.sys.crew_total = 10; s.sys.shuttles = 6;
    s.sys.shields_max[0] = s.sys.shields[0] = 40;
    s.sys.shields_max[1] = s.sys.shields[1] = 32;
    s.sys.shields_max[2] = s.sys.shields[2] = 24;
    s.sys.shields_max[3] = s.sys.shields[3] = 24;
    s.sys.shields_max[4] = s.sys.shields[4] = 32;
    s.sys.shields_max[5] = s.sys.shields[5] = 28;
    s.sys.hull = s.sys.hull_max = 40;
    s.dac = dac_for_hull(40);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"), make_ph1("Ph-1 C"),
        make_ph1("Ph-1 D"), make_ph1("Ph-1 E"), make_ph1("Ph-1 F"),
        make_ph2("Ph-2 1"), make_ph2("Ph-2 2"),
        make_ph3("Ph-3 1"), make_ph3("Ph-3 2"),
        make_photon("Photon A"), make_photon("Photon B"),
        make_photon("Photon C"), make_photon("Photon D"),
        make_drone("Drone 1"), make_drone("Drone 2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ D5G Commando Cruiser (klingon_d5g) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_klingon_d5g(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Klingon;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 16;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 16; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",8,8},{"R.Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 3;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 8; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 24;
    s.dac = dac_for_hull(25);
    s.sys.cloak_installed = true; s.sys.cloak_cost = 6;
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_ph3("Ph-3 LS"), make_ph3("Ph-3 RS"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ D7D Drone Battlecruiser (klingon_d7d) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_klingon_d7d(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Klingon;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 18;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 18; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",9,9},{"R.Warp",9,9}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 3;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 10; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 20;
    s.sys.shields_max[1] = s.sys.shields[1] = 14;
    s.sys.shields_max[2] = s.sys.shields[2] = 10;
    s.sys.shields_max[3] = s.sys.shields[3] = 12;
    s.sys.shields_max[4] = s.sys.shields[4] = 10;
    s.sys.shields_max[5] = s.sys.shields[5] = 14;
    s.sys.hull = s.sys.hull_max = 28;
    s.dac = dac_for_hull(30);
    s.weapons = {
        make_ph1("Ph-1 FX"), make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_ph2("Ph-2 A"), make_ph2("Ph-2 B"),
        make_disruptor("Disruptor A"), make_disruptor("Disruptor B"),
        make_drone("Drone A"), make_drone("Drone B"),
        make_drone("Drone C"), make_drone("Drone D"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ F5X Frigate FX (klingon_f5x) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_klingon_f5x(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Klingon;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 8;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 8; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",4,4},{"R.Warp",4,4}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 1;
    s.sys.crew_total = 4; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 14;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 8;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 16;
    s.dac = dac_for_hull(15);
    s.sys.cloak_installed = true; s.sys.cloak_cost = 6;
    s.weapons = {
        make_disruptor("Disruptor A"), make_disruptor("Disruptor B"),
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_drone("Drone Rack A"), make_drone("Drone Rack B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ D5I ISF Cruiser (klingon_d5i) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_klingon_d5i(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Klingon;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 16;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 16; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",8,8},{"R.Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 3;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 6; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 24;
    s.dac = dac_for_hull(25);
    s.weapons = {
        make_ph2("Ph-2 FX"), make_ph2("Ph-2 A"), make_ph2("Ph-2 B"),
        make_ph3("Ph-3 LS"), make_ph3("Ph-3 RS"),
        make_drone("Drone A"), make_drone("Drone B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ D5H Light Tactical Transport (klingon_d5h) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_klingon_d5h(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Klingon;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 16;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 16; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",8,8},{"R.Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 3;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 6; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 24;
    s.dac = dac_for_hull(25);
    s.weapons = {
        make_ph1("Ph-1 FX"), make_ph1("Ph-1 A"),
        make_ph2("Ph-2 A"), make_ph2("Ph-2 B"),
        make_ph3("Ph-3 LS"), make_ph3("Ph-3 RS"),
        make_disruptor("Disruptor A"), make_disruptor("Disruptor B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ C8V Heavy Carrier (kli_c8v) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_kli_c8v(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Klingon;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 28;
    s.sys.max_impulse_power = 6;
    s.eaf.warp_power = 28; s.eaf.impulse_power = 6;
    s.sys.warp_groups    = {{"L.Warp",14,14},{"R.Warp",14,14}};
    s.sys.impulse_groups = {{"Impulse",6,6}};
    s.sys.apr_rated = s.sys.apr_current = 5;
    s.sys.battery_cap = 3;
    s.sys.crew_total = 10; s.sys.shuttles = 24;
    s.sys.shields_max[0] = s.sys.shields[0] = 30;
    s.sys.shields_max[1] = s.sys.shields[1] = 20;
    s.sys.shields_max[2] = s.sys.shields[2] = 14;
    s.sys.shields_max[3] = s.sys.shields[3] = 18;
    s.sys.shields_max[4] = s.sys.shields[4] = 14;
    s.sys.shields_max[5] = s.sys.shields[5] = 20;
    s.sys.hull = s.sys.hull_max = 50;
    s.dac = dac_for_hull(50);
    s.weapons = {
        make_disruptor("Disruptor A"), make_disruptor("Disruptor B"),
        make_disruptor("Disruptor C"), make_disruptor("Disruptor D"),
        make_disruptor("Disruptor E"), make_disruptor("Disruptor F"),
        make_ph2("Ph-2 A"), make_ph2("Ph-2 B"),
        make_ph2("Ph-2 C"), make_ph2("Ph-2 D"),
        make_ph3("Ph-3 A"), make_ph3("Ph-3 B"),
        make_ph3("Ph-3 C"), make_ph3("Ph-3 D"),
        make_drone("Drone Rack A"), make_drone("Drone Rack B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ D5V Light Carrier (klingon_d5v) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_klingon_d5v(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Klingon;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 16;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 16; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",8,8},{"R.Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 3;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 10; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 24;
    s.dac = dac_for_hull(25);
    s.weapons = {
        make_ph1("Ph-1 FX"), make_ph1("Ph-1 A"),
        make_ph2("Ph-2 A"), make_ph2("Ph-2 B"),
        make_ph3("Ph-3 LS"), make_ph3("Ph-3 RS"),
        make_disruptor("Disruptor A"), make_disruptor("Disruptor B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ F5J Penal Frigate (klingon_f5j) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_klingon_f5j(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Klingon;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 8;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 8; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",4,4},{"R.Warp",4,4}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 1;
    s.sys.crew_total = 8; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 12;
    s.sys.shields_max[1] = s.sys.shields[1] = 9;
    s.sys.shields_max[2] = s.sys.shields[2] = 6;
    s.sys.shields_max[3] = s.sys.shields[3] = 8;
    s.sys.shields_max[4] = s.sys.shields[4] = 6;
    s.sys.shields_max[5] = s.sys.shields[5] = 9;
    s.sys.hull = s.sys.hull_max = 16;
    s.dac = dac_for_hull(15);
    s.weapons = {
        make_ph2("Ph-2 A"), make_ph2("Ph-2 B"),
        make_ph2("Ph-2 RX"),
        make_disruptor("Disruptor"),
        make_drone("Drone"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ D6J Penal Battlecruiser (klingon_d6j) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_klingon_d6j(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Klingon;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 18;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 18; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",9,9},{"R.Warp",9,9}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 3;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 10; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 20;
    s.sys.shields_max[1] = s.sys.shields[1] = 14;
    s.sys.shields_max[2] = s.sys.shields[2] = 10;
    s.sys.shields_max[3] = s.sys.shields[3] = 12;
    s.sys.shields_max[4] = s.sys.shields[4] = 10;
    s.sys.shields_max[5] = s.sys.shields[5] = 14;
    s.sys.hull = s.sys.hull_max = 28;
    s.dac = dac_for_hull(30);
    s.weapons = {
        make_ph2("Ph-2 FX"),
        make_ph2("Ph-2 A"), make_ph2("Ph-2 B"),
        make_disruptor("Disruptor A"), make_disruptor("Disruptor B"),
        make_drone("Drone A"), make_drone("Drone B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ F6 Battle Frigate (klingon_f6) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_klingon_f6(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Klingon;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 10;
    s.sys.max_impulse_power = 3;
    s.eaf.warp_power = 10; s.eaf.impulse_power = 3;
    s.sys.warp_groups    = {{"L.Warp",5,5},{"R.Warp",5,5}};
    s.sys.impulse_groups = {{"Impulse",3,3}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 1;
    s.sys.crew_total = 10; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 14;
    s.sys.shields_max[1] = s.sys.shields[1] = 9;
    s.sys.shields_max[2] = s.sys.shields[2] = 6;
    s.sys.shields_max[3] = s.sys.shields[3] = 8;
    s.sys.shields_max[4] = s.sys.shields[4] = 6;
    s.sys.shields_max[5] = s.sys.shields[5] = 9;
    s.sys.hull = s.sys.hull_max = 18;
    s.dac = dac_for_hull(20);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_ph1("Ph-1 RX"),
        make_disruptor("Disruptor FX"),
        make_drone("Drone A"), make_drone("Drone B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ D6E Survey Cruiser (klingon_d6e) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_klingon_d6e(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Klingon;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 16;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 16; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",8,8},{"R.Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 3;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 10; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 26;
    s.dac = dac_for_hull(25);
    s.weapons = {
        make_ph2("Ph-2 FX"),
        make_ph2("Ph-2 A"), make_ph2("Ph-2 B"),
        make_disruptor("Disruptor"),
        make_drone("Drone"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ MD5 War Cruiser Mauler (klingon_md5) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_klingon_md5(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Klingon;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 16;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 16; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",8,8},{"R.Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 3;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 8; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 24;
    s.dac = dac_for_hull(25);
    s.weapons = {
        make_ph1("Ph-1 FX"), make_ph1("Ph-1 A"),
        make_ph2("Ph-2 A"), make_ph2("Ph-2 B"),
        make_ph3("Ph-3 LS"), make_ph3("Ph-3 RS"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ D5F Anti-Fighter Cruiser (klingon_d5f) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_klingon_d5f(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Klingon;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 16;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 16; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",8,8},{"R.Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 3;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 8; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 24;
    s.dac = dac_for_hull(25);
    s.weapons = {
        make_ph1("Ph-1 FX"), make_ph1("Ph-1 A"),
        make_ph2("Ph-2 A"), make_ph2("Ph-2 B"),
        make_ph3("Ph-3 A"), make_ph3("Ph-3 B"),
        make_ph3("Ph-3 C"), make_ph3("Ph-3 D"),
        make_disruptor("Disruptor A"), make_disruptor("Disruptor B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ C7 Heavy Battlecruiser (klingon_c7) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_klingon_c7(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Klingon;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 22;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 22; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",11,11},{"R.Warp",11,11}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 4;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 10; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 24;
    s.sys.shields_max[1] = s.sys.shields[1] = 16;
    s.sys.shields_max[2] = s.sys.shields[2] = 12;
    s.sys.shields_max[3] = s.sys.shields[3] = 14;
    s.sys.shields_max[4] = s.sys.shields[4] = 12;
    s.sys.shields_max[5] = s.sys.shields[5] = 16;
    s.sys.hull = s.sys.hull_max = 36;
    s.dac = dac_for_hull(35);
    s.weapons = {
        make_ph1("Ph-1 FX"),
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"), make_ph1("Ph-1 C"),
        make_ph3("Ph-3 LS"), make_ph3("Ph-3 RS"),
        make_disruptor("Disruptor A"), make_disruptor("Disruptor B"),
        make_drone("Drone A"), make_drone("Drone B"),
        make_drone("Drone C"), make_drone("Drone D"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ C8S Space Control Ship (klingon_c8s) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_klingon_c8s(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Klingon;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 30;
    s.sys.max_impulse_power = 6;
    s.eaf.warp_power = 30; s.eaf.impulse_power = 6;
    s.sys.warp_groups    = {{"L.Warp",15,15},{"R.Warp",15,15}};
    s.sys.impulse_groups = {{"Impulse",6,6}};
    s.sys.apr_rated = s.sys.apr_current = 5;
    s.sys.battery_cap = 3;
    s.sys.crew_total = 10; s.sys.shuttles = 3;
    s.sys.shields_max[0] = s.sys.shields[0] = 30;
    s.sys.shields_max[1] = s.sys.shields[1] = 20;
    s.sys.shields_max[2] = s.sys.shields[2] = 14;
    s.sys.shields_max[3] = s.sys.shields[3] = 18;
    s.sys.shields_max[4] = s.sys.shields[4] = 14;
    s.sys.shields_max[5] = s.sys.shields[5] = 20;
    s.sys.hull = s.sys.hull_max = 46;
    s.dac = dac_for_hull(45);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"), make_ph1("Ph-1 C"),
        make_ph1("Ph-1 D"), make_ph1("Ph-1 E"), make_ph1("Ph-1 F"),
        make_ph3("Ph-3 A"), make_ph3("Ph-3 B"),
        make_disruptor("Disruptor FX"),
        make_drone("Drone A"), make_drone("Drone B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ D5K Improved War Cruiser (klingon_d5k) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_klingon_d5k(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Klingon;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 16;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 16; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",8,8},{"R.Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 3;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 8; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 24;
    s.dac = dac_for_hull(25);
    s.sys.cloak_installed = true; s.sys.cloak_cost = 6;
    s.weapons = {
        make_disruptor("Disruptor A"), make_disruptor("Disruptor B"),
        make_disruptor("Disruptor C"), make_disruptor("Disruptor D"),
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_ph3("Ph-3 LS"), make_ph3("Ph-3 RS"),
        make_drone("Drone Rack A"), make_drone("Drone Rack B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ AD5 Carrier Escort (kli_ad5) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_kli_ad5(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Klingon;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 16;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 16; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",8,8},{"R.Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 3;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 8; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 24;
    s.dac = dac_for_hull(25);
    s.weapons = {
        make_disruptor("Disruptor A"), make_disruptor("Disruptor B"),
        make_disruptor("Disruptor C"), make_disruptor("Disruptor D"),
        make_ph2("Ph-2 A"), make_ph2("Ph-2 B"),
        make_ph3("Ph-3 LS"), make_ph3("Ph-3 RS"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ D7M Mauler Cruiser (klingon_d7m) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_klingon_d7m(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Klingon;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 18;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 18; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",9,9},{"R.Warp",9,9}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 3;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 10; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 20;
    s.sys.shields_max[1] = s.sys.shields[1] = 14;
    s.sys.shields_max[2] = s.sys.shields[2] = 10;
    s.sys.shields_max[3] = s.sys.shields[3] = 12;
    s.sys.shields_max[4] = s.sys.shields[4] = 10;
    s.sys.shields_max[5] = s.sys.shields[5] = 14;
    s.sys.hull = s.sys.hull_max = 28;
    s.dac = dac_for_hull(30);
    s.sys.cloak_installed = true; s.sys.cloak_cost = 6;
    s.weapons = {
        make_ph3("Ph-3 A"), make_ph3("Ph-3 B"),
        make_ph3("Ph-3 C"), make_ph3("Ph-3 D"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ E4E Escort (kli_e4e) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_kli_e4e(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Klingon;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 6;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 6; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",3,3},{"R.Warp",3,3}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 1;
    s.sys.battery_cap = 1;
    s.sys.crew_total = 4; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 10;
    s.sys.shields_max[1] = s.sys.shields[1] = 7;
    s.sys.shields_max[2] = s.sys.shields[2] = 4;
    s.sys.shields_max[3] = s.sys.shields[3] = 6;
    s.sys.shields_max[4] = s.sys.shields[4] = 4;
    s.sys.shields_max[5] = s.sys.shields[5] = 7;
    s.sys.hull = s.sys.hull_max = 12;
    s.dac = dac_for_hull(10);
    s.weapons = {
        make_ph3("Ph-G A"), make_ph3("Ph-G B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ D7V Strike Carrier (klingon_d7v) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_klingon_d7v(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Klingon;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 18;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 18; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",9,9},{"R.Warp",9,9}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 3;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 10; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 28;
    s.dac = dac_for_hull(30);
    s.weapons = {
        make_ph2("Ph-2 FX2K"),
        make_ph1("Ph-1 LF"), make_ph1("Ph-1 A"),
        make_ph2("Ph-2 A"), make_ph2("Ph-2 B"),
        make_disruptor("Disruptor A"), make_disruptor("Disruptor B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ AF5 Aegis Frigate (klingon_af5) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_klingon_af5(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Klingon;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 8;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 8; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",4,4},{"R.Warp",4,4}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 1;
    s.sys.crew_total = 8; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 10;
    s.sys.shields_max[1] = s.sys.shields[1] = 7;
    s.sys.shields_max[2] = s.sys.shields[2] = 4;
    s.sys.shields_max[3] = s.sys.shields[3] = 6;
    s.sys.shields_max[4] = s.sys.shields[4] = 4;
    s.sys.shields_max[5] = s.sys.shields[5] = 7;
    s.sys.hull = s.sys.hull_max = 14;
    s.dac = dac_for_hull(15);
    s.weapons = {
        make_ph2("Ph-2K A"), make_ph2("Ph-2K B"),
        make_drone("Drone A"), make_drone("Drone B"),
        make_drone("Drone C"), make_drone("Drone D"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ E4D Drone Escort (klingon_e4d) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_klingon_e4d(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Klingon;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 6;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 6; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",3,3},{"R.Warp",3,3}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 1;
    s.sys.battery_cap = 1;
    s.sys.crew_total = 6; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 10;
    s.sys.shields_max[1] = s.sys.shields[1] = 7;
    s.sys.shields_max[2] = s.sys.shields[2] = 4;
    s.sys.shields_max[3] = s.sys.shields[3] = 6;
    s.sys.shields_max[4] = s.sys.shields[4] = 4;
    s.sys.shields_max[5] = s.sys.shields[5] = 7;
    s.sys.hull = s.sys.hull_max = 12;
    s.dac = dac_for_hull(10);
    s.weapons = {
        make_ph2("Ph-2 A"), make_ph2("Ph-2 B"),
        make_drone("Drone A"), make_drone("Drone B"),
        make_drone("Drone C"), make_drone("Drone D"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ E4V Escort Carrier (klingon_e4v) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_klingon_e4v(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Klingon;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 6;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 6; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",3,3},{"R.Warp",3,3}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 1;
    s.sys.battery_cap = 1;
    s.sys.crew_total = 6; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 10;
    s.sys.shields_max[1] = s.sys.shields[1] = 7;
    s.sys.shields_max[2] = s.sys.shields[2] = 4;
    s.sys.shields_max[3] = s.sys.shields[3] = 6;
    s.sys.shields_max[4] = s.sys.shields[4] = 4;
    s.sys.shields_max[5] = s.sys.shields[5] = 7;
    s.sys.hull = s.sys.hull_max = 12;
    s.dac = dac_for_hull(10);
    s.weapons = {
        make_ph2("Ph-2 A"), make_ph2("Ph-2 B"),
        make_disruptor("Disruptor A"), make_disruptor("Disruptor B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ D5J Penal War Cruiser (klingon_d5j) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_klingon_d5j(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Klingon;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 16;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 16; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",8,8},{"R.Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 3;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 8; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 24;
    s.dac = dac_for_hull(25);
    s.weapons = {
        make_ph1("Ph-1 FX"), make_ph1("Ph-1 A"),
        make_ph3("Ph-3 LS"), make_ph3("Ph-3 RS"),
        make_disruptor("Disruptor A"), make_disruptor("Disruptor B"),
        make_drone("Drone A"), make_drone("Drone B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ E5 Battle Escort (klingon_e5) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_klingon_e5(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Klingon;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 6;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 6; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",3,3},{"R.Warp",3,3}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 1;
    s.sys.crew_total = 6; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 10;
    s.sys.shields_max[1] = s.sys.shields[1] = 7;
    s.sys.shields_max[2] = s.sys.shields[2] = 4;
    s.sys.shields_max[3] = s.sys.shields[3] = 6;
    s.sys.shields_max[4] = s.sys.shields[4] = 4;
    s.sys.shields_max[5] = s.sys.shields[5] = 7;
    s.sys.hull = s.sys.hull_max = 14;
    s.dac = dac_for_hull(15);
    s.weapons = {
        make_ph2("Ph-2K A"), make_ph2("Ph-2K B"),
        make_disruptor("Disruptor FX"),
        make_drone("Drone A"), make_drone("Drone B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ D6G Commando Cruiser (klingon_d6g) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_klingon_d6g(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Klingon;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 18;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 18; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",9,9},{"R.Warp",9,9}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 3;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 10; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 20;
    s.sys.shields_max[1] = s.sys.shields[1] = 14;
    s.sys.shields_max[2] = s.sys.shields[2] = 10;
    s.sys.shields_max[3] = s.sys.shields[3] = 12;
    s.sys.shields_max[4] = s.sys.shields[4] = 10;
    s.sys.shields_max[5] = s.sys.shields[5] = 14;
    s.sys.hull = s.sys.hull_max = 28;
    s.dac = dac_for_hull(30);
    s.sys.cloak_installed = true; s.sys.cloak_cost = 6;
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_ph1("Ph-1 C"), make_ph1("Ph-1 D"),
        make_ph3("Ph-3 LS"), make_ph3("Ph-3 RS"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ C7A Heavy Stasis Battlecruiser (klingon_c7a) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_klingon_c7a(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Klingon;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 22;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 22; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",11,11},{"R.Warp",11,11}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 4;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 10; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 24;
    s.sys.shields_max[1] = s.sys.shields[1] = 16;
    s.sys.shields_max[2] = s.sys.shields[2] = 12;
    s.sys.shields_max[3] = s.sys.shields[3] = 14;
    s.sys.shields_max[4] = s.sys.shields[4] = 12;
    s.sys.shields_max[5] = s.sys.shields[5] = 16;
    s.sys.hull = s.sys.hull_max = 36;
    s.dac = dac_for_hull(35);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"), make_ph1("Ph-1 C"),
        make_ph1("Ph-1 D"), make_ph1("Ph-1 RF"),
        make_disruptor("Disruptor A"), make_disruptor("Disruptor B"),
        make_drone("Drone A"), make_drone("Drone B"),
        make_drone("Drone C"), make_drone("Drone D"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ E3D Drone Escort (klingon_e3d) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_klingon_e3d(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Klingon;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 4;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 4; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",2,2},{"R.Warp",2,2}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 1;
    s.sys.battery_cap = 1;
    s.sys.crew_total = 5; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 8;
    s.sys.shields_max[1] = s.sys.shields[1] = 5;
    s.sys.shields_max[2] = s.sys.shields[2] = 3;
    s.sys.shields_max[3] = s.sys.shields[3] = 4;
    s.sys.shields_max[4] = s.sys.shields[4] = 3;
    s.sys.shields_max[5] = s.sys.shields[5] = 5;
    s.sys.hull = s.sys.hull_max = 10;
    s.dac = dac_for_hull(10);
    s.weapons = {
        make_ph3("Ph-3 A"), make_ph3("Ph-3 B"),
        make_drone("Drone A"), make_drone("Drone B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ E4J Penal Escort (klingon_e4j) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_klingon_e4j(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Klingon;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 6;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 6; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",3,3},{"R.Warp",3,3}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 1;
    s.sys.battery_cap = 1;
    s.sys.crew_total = 6; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 10;
    s.sys.shields_max[1] = s.sys.shields[1] = 7;
    s.sys.shields_max[2] = s.sys.shields[2] = 4;
    s.sys.shields_max[3] = s.sys.shields[3] = 6;
    s.sys.shields_max[4] = s.sys.shields[4] = 4;
    s.sys.shields_max[5] = s.sys.shields[5] = 7;
    s.sys.hull = s.sys.hull_max = 12;
    s.dac = dac_for_hull(10);
    s.weapons = {
        make_ph2("Ph-2 A"), make_ph2("Ph-2 B"),
        make_ph2("Ph-2 RX"),
        make_disruptor("Disruptor"),
        make_drone("Drone"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ D7E Survey Cruiser (klingon_d7e) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_klingon_d7e(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Klingon;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 18;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 18; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",9,9},{"R.Warp",9,9}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 3;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 10; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 28;
    s.dac = dac_for_hull(30);
    s.weapons = {
        make_ph2("Ph-2 FX"),
        make_ph2("Ph-2 A"), make_ph2("Ph-2 B"),
        make_disruptor("Disruptor A"), make_disruptor("Disruptor B"),
        make_drone("Drone"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ D7V Strike Carrier (kli_d7v) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_kli_d7v(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Klingon;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 18;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 18; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",9,9},{"R.Warp",9,9}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 3;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 10; s.sys.shuttles = 8;
    s.sys.shields_max[0] = s.sys.shields[0] = 20;
    s.sys.shields_max[1] = s.sys.shields[1] = 14;
    s.sys.shields_max[2] = s.sys.shields[2] = 10;
    s.sys.shields_max[3] = s.sys.shields[3] = 12;
    s.sys.shields_max[4] = s.sys.shields[4] = 10;
    s.sys.shields_max[5] = s.sys.shields[5] = 14;
    s.sys.hull = s.sys.hull_max = 28;
    s.dac = dac_for_hull(30);
    s.weapons = {
        make_disruptor("Disruptor A"), make_disruptor("Disruptor B"),
        make_disruptor("Disruptor C"), make_disruptor("Disruptor D"),
        make_ph2("Ph-2 A"), make_ph2("Ph-2 B"),
        make_ph3("Ph-3 LS"), make_ph3("Ph-3 RS"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ F5E Combat Escort (klingon_f5e) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_klingon_f5e(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Klingon;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 8;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 8; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",4,4},{"R.Warp",4,4}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 1;
    s.sys.crew_total = 8; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 10;
    s.sys.shields_max[1] = s.sys.shields[1] = 7;
    s.sys.shields_max[2] = s.sys.shields[2] = 4;
    s.sys.shields_max[3] = s.sys.shields[3] = 6;
    s.sys.shields_max[4] = s.sys.shields[4] = 4;
    s.sys.shields_max[5] = s.sys.shields[5] = 7;
    s.sys.hull = s.sys.hull_max = 14;
    s.dac = dac_for_hull(15);
    s.weapons = {
        make_ph2("Ph-2K A"), make_ph2("Ph-2K B"),
        make_drone("Drone A"), make_drone("Drone B"),
        make_drone("Drone C"), make_drone("Drone D"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ F5V Light Carrier (kli_f5v) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_kli_f5v(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Klingon;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 8;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 8; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",4,4},{"R.Warp",4,4}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 1;
    s.sys.crew_total = 4; s.sys.shuttles = 8;
    s.sys.shields_max[0] = s.sys.shields[0] = 14;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 8;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 16;
    s.dac = dac_for_hull(15);
    s.weapons = {
        make_disruptor("Disruptor A"), make_disruptor("Disruptor B"),
        make_ph3("Ph-3 LS"), make_ph3("Ph-3 RS"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ AD6 Heavy Escort Cruiser (klingon_ad6) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_klingon_ad6(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Klingon;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 16;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 16; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",8,8},{"R.Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 3;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 10; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 26;
    s.dac = dac_for_hull(25);
    s.weapons = {
        make_ph2("Ph-2 FX2K"),
        make_ph2("Ph-2 A"), make_ph2("Ph-2 B"),
        make_ph2("Ph-2 C"), make_ph2("Ph-2 D"),
        make_ph2("Ph-2 E"), make_ph2("Ph-2 F"),
        make_drone("Drone A"), make_drone("Drone B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ D5D Drone Cruiser (klingon_d5d) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_klingon_d5d(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Klingon;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 16;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 16; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",8,8},{"R.Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 3;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 8; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 24;
    s.dac = dac_for_hull(25);
    s.weapons = {
        make_ph1("Ph-1 FX"), make_ph1("Ph-1 A"),
        make_ph2("Ph-2 A"), make_ph2("Ph-2 B"),
        make_ph3("Ph-3 LS"), make_ph3("Ph-3 RS"),
        make_drone("Drone A"), make_drone("Drone B"),
        make_drone("Drone C"), make_drone("Drone D"),
        make_drone("Drone E"), make_drone("Drone F"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ D7CX Battlecruiser DX (klingon_d7cx) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_klingon_d7cx(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Klingon;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 22;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 22; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",11,11},{"R.Warp",11,11}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 4;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 10; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 24;
    s.sys.shields_max[1] = s.sys.shields[1] = 16;
    s.sys.shields_max[2] = s.sys.shields[2] = 12;
    s.sys.shields_max[3] = s.sys.shields[3] = 14;
    s.sys.shields_max[4] = s.sys.shields[4] = 12;
    s.sys.shields_max[5] = s.sys.shields[5] = 16;
    s.sys.hull = s.sys.hull_max = 36;
    s.dac = dac_for_hull(35);
    s.sys.cloak_installed = true; s.sys.cloak_cost = 6;
    s.weapons = {
        make_disruptor("Disruptor A"), make_disruptor("Disruptor B"),
        make_disruptor("Disruptor C"), make_disruptor("Disruptor D"),
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_ph1("Ph-1 C"), make_ph1("Ph-1 D"),
        make_drone("Drone Rack GX A"), make_drone("Drone Rack GX B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ D7N Diplomatic Cruiser (klingon_d7n) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_klingon_d7n(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Klingon;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 18;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 18; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",9,9},{"R.Warp",9,9}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 3;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 10; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 28;
    s.dac = dac_for_hull(30);
    s.weapons = {
        make_ph2("Ph-2 FX2K"),
        make_ph2("Ph-2 A"), make_ph2("Ph-2 B"),
        make_ph2("Ph-2 C"), make_ph2("Ph-2 D"),
        make_disruptor("Disruptor A"), make_disruptor("Disruptor B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ D5S Scout Cruiser (klingon_d5s) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_klingon_d5s(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Klingon;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 16;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 16; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",8,8},{"R.Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 3;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 6; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 24;
    s.dac = dac_for_hull(25);
    s.weapons = {
        make_ph1("Ph-1 FX"), make_ph1("Ph-1 A"),
        make_ph2("Ph-2 A"), make_ph2("Ph-2 B"),
        make_ph3("Ph-3 LS"), make_ph3("Ph-3 RS"),
        make_drone("Drone A"), make_drone("Drone B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ D5M Minesweeper (klingon_d5m) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_klingon_d5m(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Klingon;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 16;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 16; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",8,8},{"R.Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 3;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 8; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 24;
    s.dac = dac_for_hull(25);
    s.weapons = {
        make_ph1("Ph-1 FX"), make_ph1("Ph-1 A"),
        make_ph2("Ph-2 A"), make_ph2("Ph-2 B"),
        make_ph3("Ph-3 LS"), make_ph3("Ph-3 RS"),
        make_drone("Drone A"), make_drone("Drone B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ D5C Light Command Cruiser (klingon_d5c) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_klingon_d5c(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Klingon;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 16;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 16; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",8,8},{"R.Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 3;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 10; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 24;
    s.dac = dac_for_hull(25);
    s.weapons = {
        make_ph1("Ph-1 FX"), make_ph1("Ph-1 A"),
        make_ph2("Ph-2 A"), make_ph2("Ph-2 B"),
        make_ph3("Ph-3 LS"), make_ph3("Ph-3 RS"),
        make_disruptor("Disruptor A"), make_disruptor("Disruptor B"),
        make_drone("Drone A"), make_drone("Drone B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ D5P War PF Tender (klingon_d5p) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_klingon_d5p(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Klingon;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 16;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 16; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",8,8},{"R.Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 3;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 8; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 24;
    s.dac = dac_for_hull(25);
    s.sys.cloak_installed = true; s.sys.cloak_cost = 6;
    s.weapons = {
        make_disruptor("Disruptor A"), make_disruptor("Disruptor B"),
        make_disruptor("Disruptor C"), make_disruptor("Disruptor D"),
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_ph3("Ph-3 LS"), make_ph3("Ph-3 RS"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ C9A Stasis Dreadnought (klingon_c9a) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_klingon_c9a(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Klingon;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 30;
    s.sys.max_impulse_power = 6;
    s.eaf.warp_power = 30; s.eaf.impulse_power = 6;
    s.sys.warp_groups    = {{"L.Warp",15,15},{"R.Warp",15,15}};
    s.sys.impulse_groups = {{"Impulse",6,6}};
    s.sys.apr_rated = s.sys.apr_current = 5;
    s.sys.battery_cap = 3;
    s.sys.crew_total = 10; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 30;
    s.sys.shields_max[1] = s.sys.shields[1] = 20;
    s.sys.shields_max[2] = s.sys.shields[2] = 14;
    s.sys.shields_max[3] = s.sys.shields[3] = 18;
    s.sys.shields_max[4] = s.sys.shields[4] = 14;
    s.sys.shields_max[5] = s.sys.shields[5] = 20;
    s.sys.hull = s.sys.hull_max = 44;
    s.dac = dac_for_hull(45);
    s.weapons = {
        make_ph1("Ph-1 LF"), make_ph1("Ph-1 A"),
        make_ph1("Ph-1 B"), make_ph1("Ph-1 C"),
        make_ph1("Ph-1 RF"),
        make_ph2("Ph-2 A"), make_ph2("Ph-2 B"),
        make_disruptor("Disruptor FX"),
        make_drone("Drone A"), make_drone("Drone B"),
        make_drone("Drone C"), make_drone("Drone D"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ D5N Diplomatic Cruiser (klingon_d5n) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_klingon_d5n(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Klingon;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 16;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 16; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",8,8},{"R.Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 3;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 10; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 24;
    s.dac = dac_for_hull(25);
    s.weapons = {
        make_ph1("Ph-1 FX"), make_ph1("Ph-1 A"),
        make_ph2("Ph-2 A"), make_ph2("Ph-2 B"),
        make_ph3("Ph-3 LS"), make_ph3("Ph-3 RS"),
        make_disruptor("Disruptor A"), make_disruptor("Disruptor B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ D5E Escort Cruiser (klingon_d5e) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_klingon_d5e(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Klingon;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 16;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 16; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",8,8},{"R.Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 3;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 8; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 24;
    s.dac = dac_for_hull(25);
    s.sys.cloak_installed = true; s.sys.cloak_cost = 6;
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_ph3("Ph-3 LS"), make_ph3("Ph-3 RS"),
        make_drone("Drone Rack A"), make_drone("Drone Rack B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ D5L War Cruiser Leader (klingon_d5l) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_klingon_d5l(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Klingon;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 16;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 16; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",8,8},{"R.Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 3;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 10; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 24;
    s.dac = dac_for_hull(25);
    s.weapons = {
        make_ph1("Ph-1 FX"), make_ph1("Ph-1 A"),
        make_ph2("Ph-2 A"), make_ph2("Ph-2 B"),
        make_ph3("Ph-3 LS"), make_ph3("Ph-3 RS"),
        make_disruptor("Disruptor A"), make_disruptor("Disruptor B"),
        make_drone("Drone A"), make_drone("Drone B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ RKL Sparrowhawk Light Cruiser (klingon_rkl) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_klingon_rkl(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Klingon;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 14;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 14; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",7,7},{"R.Warp",7,7}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 3;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 10; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 16;
    s.sys.shields_max[1] = s.sys.shields[1] = 10;
    s.sys.shields_max[2] = s.sys.shields[2] = 6;
    s.sys.shields_max[3] = s.sys.shields[3] = 8;
    s.sys.shields_max[4] = s.sys.shields[4] = 6;
    s.sys.shields_max[5] = s.sys.shields[5] = 10;
    s.sys.hull = s.sys.hull_max = 22;
    s.dac = dac_for_hull(20);
    s.weapons = {
        make_ph1("Ph-1 FX"),
        make_ph3("Ph-3 LS"), make_ph3("Ph-3 RS"),
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_disruptor("Disruptor A"), make_disruptor("Disruptor B"),
        make_drone("Drone A"), make_drone("Drone B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ D6S Heavy Scout Cruiser (klingon_d6s) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_klingon_d6s(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Klingon;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 16;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 16; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",8,8},{"R.Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 3;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 10; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 26;
    s.dac = dac_for_hull(25);
    s.weapons = {
        make_ph2("Ph-2 FX"),
        make_ph2("Ph-2 A"), make_ph2("Ph-2 B"),
        make_drone("Drone A"), make_drone("Drone B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ APA Apache Medium Command Cruiser (hydran_apa) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_hydran_apa(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Hydran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 16;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 16; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",8,8},{"R.Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 3;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 10; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 28;
    s.dac = dac_for_hull(30);
    s.weapons = {
        make_fusion("Fusion FA"),
        make_hellbore("Hellbore FA-1"), make_hellbore("Hellbore FA-2"),
        make_ph1("Ph-1 A-1"), make_ph1("Ph-1 A-2"),
        make_ph1("Ph-1 B-1"), make_ph1("Ph-1 B-2"),
        make_ph3("Ph-G 1"), make_ph3("Ph-G 2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ NVL Trooper New Light Carrier (hydran_nvl) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_hydran_nvl(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Hydran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 14;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 14; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",7,7},{"R.Warp",7,7}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 3;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 10; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 26;
    s.dac = dac_for_hull(25);
    s.weapons = {
        make_ph1("Ph-1 FA"),
        make_ph1("Ph-1"),
        make_ph2("Ph-2 A-1"), make_ph2("Ph-2 A-2"),
        make_ph2("Ph-2 B-1"), make_ph2("Ph-2 B-2"),
        make_ph3("Ph-G 1"), make_ph3("Ph-G 2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Dragoon Cruiser (hydran_dg) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_hydran_dg(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Hydran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 18;
    s.sys.max_impulse_power = 3;
    s.eaf.warp_power = 18; s.eaf.impulse_power = 3;
    s.sys.warp_groups    = {{"L.Warp",9,9},{"R.Warp",9,9}};
    s.sys.impulse_groups = {{"Impulse",3,3}};
    s.sys.apr_rated = s.sys.apr_current = 3;
    s.sys.battery_cap = 5;
    s.sys.crew_total = 10; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 20;
    s.sys.shields_max[1] = s.sys.shields[1] = 16;
    s.sys.shields_max[2] = s.sys.shields[2] = 12;
    s.sys.shields_max[3] = s.sys.shields[3] = 12;
    s.sys.shields_max[4] = s.sys.shields[4] = 12;
    s.sys.shields_max[5] = s.sys.shields[5] = 16;
    s.sys.hull = s.sys.hull_max = 36;
    s.dac = dac_for_hull(35);
    s.weapons = {
        make_ph3("Ph-G FA-1"), make_ph3("Ph-G FA-2"),
        make_ph1("Ph-1 LS-1"), make_ph1("Ph-1 LS-2"),
        make_ph1("Ph-1 RS-1"), make_ph1("Ph-1 RS-2"),
        make_hellbore("Hellbore FA-1"), make_hellbore("Hellbore FA-2"), make_hellbore("Hellbore FA-3"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ MNG Mongol Medium Cruiser (hydran_mng) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_hydran_mng(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Hydran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 16;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 16; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",8,8},{"R.Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 3;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 10; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 28;
    s.dac = dac_for_hull(30);
    s.weapons = {
        make_ph1("Ph-1 FA"),
        make_fusion("Fusion FA"),
        make_ph2("Ph-2 A-1"), make_ph2("Ph-2 A-2"),
        make_ph2("Ph-2 B-1"), make_ph2("Ph-2 B-2"),
        make_ph3("Ph-G 1"), make_ph3("Ph-G 2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Paladin Dreadnought (hydran_pal) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_hydran_pal(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Hydran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 24;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 24; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",12,12},{"R.Warp",12,12}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 4;
    s.sys.battery_cap = 6;
    s.sys.crew_total = 10; s.sys.shuttles = 3;
    s.sys.shields_max[0] = s.sys.shields[0] = 30;
    s.sys.shields_max[1] = s.sys.shields[1] = 24;
    s.sys.shields_max[2] = s.sys.shields[2] = 20;
    s.sys.shields_max[3] = s.sys.shields[3] = 20;
    s.sys.shields_max[4] = s.sys.shields[4] = 20;
    s.sys.shields_max[5] = s.sys.shields[5] = 24;
    s.sys.hull = s.sys.hull_max = 54;
    s.dac = dac_for_hull(55);
    s.weapons = {
        make_ph1("Ph-1 FA-1"), make_ph1("Ph-1 FA-2"),
        make_ph1("Ph-1 LS-1"), make_ph1("Ph-1 LS-2"),
        make_ph1("Ph-1 RS-1"), make_ph1("Ph-1 RS-2"),
        make_hellbore("Hellbore FA-1"), make_hellbore("Hellbore FA-2"),
        make_fusion("Fusion FA-1"), make_fusion("Fusion FA-2"),
        make_fusion("Fusion LS-1"), make_fusion("Fusion LS-2"),
        make_fusion("Fusion RS-1"), make_fusion("Fusion RS-2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ ID Iron Duke Heavy Carrier (hydran_id) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_hydran_id(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Hydran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 26;
    s.sys.max_impulse_power = 6;
    s.eaf.warp_power = 26; s.eaf.impulse_power = 6;
    s.sys.warp_groups    = {{"L.Warp",13,13},{"R.Warp",13,13}};
    s.sys.impulse_groups = {{"Impulse",6,6}};
    s.sys.apr_rated = s.sys.apr_current = 5;
    s.sys.battery_cap = 3;
    s.sys.crew_total = 10; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 28;
    s.sys.shields_max[1] = s.sys.shields[1] = 20;
    s.sys.shields_max[2] = s.sys.shields[2] = 14;
    s.sys.shields_max[3] = s.sys.shields[3] = 16;
    s.sys.shields_max[4] = s.sys.shields[4] = 14;
    s.sys.shields_max[5] = s.sys.shields[5] = 20;
    s.sys.hull = s.sys.hull_max = 44;
    s.dac = dac_for_hull(45);
    s.weapons = {
        make_fusion("Fusion"),
        make_ph1("Ph-1 FX"),
        make_ph1("Ph-1 A-1"), make_ph1("Ph-1 A-2"),
        make_hellbore("HB FA-1"), make_hellbore("HB FA-2"),
        make_ph3("Ph-G 1"), make_ph3("Ph-G 2"),
        make_ph1("Ph-1 360"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Hydran Picador Minesweeper (hydran_ms) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_hydran_ms(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Hydran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 9;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 9; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",4,4},{"R.Warp",5,5}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 4;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 4; s.sys.shuttles = 3;
    s.sys.shields_max[0] = s.sys.shields[0] = 13;
    s.sys.shields_max[1] = s.sys.shields[1] = 13;
    s.sys.shields_max[2] = s.sys.shields[2] = 10;
    s.sys.shields_max[3] = s.sys.shields[3] = 13;
    s.sys.shields_max[4] = s.sys.shields[4] = 10;
    s.sys.shields_max[5] = s.sys.shields[5] = 13;
    s.sys.hull = s.sys.hull_max = 9;
    s.dac = dac_for_hull(10);
    s.weapons = {
        make_ph3("Ph-G"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ CRU Crusader Frigate Leader (hydran_cru) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_hydran_cru(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Hydran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 8;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 8; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",4,4},{"R.Warp",4,4}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 1;
    s.sys.crew_total = 8; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 12;
    s.sys.shields_max[1] = s.sys.shields[1] = 8;
    s.sys.shields_max[2] = s.sys.shields[2] = 4;
    s.sys.shields_max[3] = s.sys.shields[3] = 6;
    s.sys.shields_max[4] = s.sys.shields[4] = 4;
    s.sys.shields_max[5] = s.sys.shields[5] = 8;
    s.sys.hull = s.sys.hull_max = 18;
    s.dac = dac_for_hull(20);
    s.weapons = {
        make_fusion("Fusion FA-1"), make_fusion("Fusion FA-2"),
        make_hellbore("HB FA"),
        make_ph2("Ph-2 A-1"), make_ph2("Ph-2 A-2"),
        make_hellbore("Hellbore"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ BAR Baron Light Command Cruiser (hydran_bar) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_hydran_bar(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Hydran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 14;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 14; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",7,7},{"R.Warp",7,7}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 3;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 10; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 26;
    s.dac = dac_for_hull(25);
    s.weapons = {
        make_hellbore("HB FA"),
        make_fusion("Fusion FA"),
        make_ph1("Ph-1 A-1"), make_ph1("Ph-1 A-2"),
        make_ph1("Ph-1 B-1"), make_ph1("Ph-1 B-2"),
        make_hellbore("Hellbore A"), make_hellbore("Hellbore B"),
        make_ph3("Ph-G 1"), make_ph3("Ph-G 2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Scout (hydran_sc) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_hydran_sc(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Hydran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 8;
    s.sys.max_impulse_power = 1;
    s.eaf.warp_power = 8; s.eaf.impulse_power = 1;
    s.sys.warp_groups    = {{"L.Warp",4,4},{"R.Warp",4,4}};
    s.sys.impulse_groups = {{"Impulse",1,1}};
    s.sys.apr_rated = s.sys.apr_current = 1;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 5; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 8;
    s.sys.shields_max[1] = s.sys.shields[1] = 6;
    s.sys.shields_max[2] = s.sys.shields[2] = 4;
    s.sys.shields_max[3] = s.sys.shields[3] = 4;
    s.sys.shields_max[4] = s.sys.shields[4] = 4;
    s.sys.shields_max[5] = s.sys.shields[5] = 6;
    s.sys.hull = s.sys.hull_max = 12;
    s.dac = dac_for_hull(10);
    s.weapons = {
        make_ph3("Ph-G FA"),
        make_ph3("Ph-G LS"),
        make_ph3("Ph-G RS"),
        make_hellbore("Hellbore FA"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Horseman Light Cruiser (hydran_hr) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_hydran_hr(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Hydran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 14;
    s.sys.max_impulse_power = 3;
    s.eaf.warp_power = 14; s.eaf.impulse_power = 3;
    s.sys.warp_groups    = {{"L.Warp",7,7},{"R.Warp",7,7}};
    s.sys.impulse_groups = {{"Impulse",3,3}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 10; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 16;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 10;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 10;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 30;
    s.dac = dac_for_hull(30);
    s.weapons = {
        make_ph3("Ph-G FA-1"), make_ph3("Ph-G FA-2"),
        make_ph3("Ph-G LS"),
        make_ph3("Ph-G RS"),
        make_fusion("Fusion FA-1"), make_fusion("Fusion FA-2"),
        make_hellbore("Hellbore FA"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ OV Overlord Heavy Battlecruiser (hydran_ov) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_hydran_ov(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Hydran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 18;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 18; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",9,9},{"R.Warp",9,9}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 4;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 10; s.sys.shuttles = 3;
    s.sys.shields_max[0] = s.sys.shields[0] = 22;
    s.sys.shields_max[1] = s.sys.shields[1] = 16;
    s.sys.shields_max[2] = s.sys.shields[2] = 10;
    s.sys.shields_max[3] = s.sys.shields[3] = 14;
    s.sys.shields_max[4] = s.sys.shields[4] = 10;
    s.sys.shields_max[5] = s.sys.shields[5] = 16;
    s.sys.hull = s.sys.hull_max = 32;
    s.dac = dac_for_hull(30);
    s.weapons = {
        make_hellbore("HB FA-1"), make_hellbore("HB FA-2"),
        make_hellbore("HB FA-3"), make_hellbore("HB FA-4"),
        make_fusion("Fusion FA"),
        make_ph1("Ph-1 A-1"), make_ph1("Ph-1 A-2"),
        make_ph1("Ph-1 B-1"), make_ph1("Ph-1 B-2"),
        make_ph1("Ph-1 360"),
        make_ph3("Ph-G 1"), make_ph3("Ph-G 2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ SAR Saracen Frigate Leader (hydran_sar) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_hydran_sar(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Hydran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 8;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 8; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",4,4},{"R.Warp",4,4}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 1;
    s.sys.crew_total = 8; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 12;
    s.sys.shields_max[1] = s.sys.shields[1] = 8;
    s.sys.shields_max[2] = s.sys.shields[2] = 4;
    s.sys.shields_max[3] = s.sys.shields[3] = 6;
    s.sys.shields_max[4] = s.sys.shields[4] = 4;
    s.sys.shields_max[5] = s.sys.shields[5] = 8;
    s.sys.hull = s.sys.hull_max = 18;
    s.dac = dac_for_hull(20);
    s.weapons = {
        make_fusion("Fusion FA-1"), make_fusion("Fusion FA-2"),
        make_fusion("Fusion FA-3"),
        make_ph2("Ph-2 A-1"), make_ph2("Ph-2 A-2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ LB Lord Bishop Command Cruiser (hydran_lb) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_hydran_lb(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Hydran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 16;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 16; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",8,8},{"R.Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 3;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 10; s.sys.shuttles = 3;
    s.sys.shields_max[0] = s.sys.shields[0] = 20;
    s.sys.shields_max[1] = s.sys.shields[1] = 14;
    s.sys.shields_max[2] = s.sys.shields[2] = 10;
    s.sys.shields_max[3] = s.sys.shields[3] = 12;
    s.sys.shields_max[4] = s.sys.shields[4] = 10;
    s.sys.shields_max[5] = s.sys.shields[5] = 14;
    s.sys.hull = s.sys.hull_max = 30;
    s.dac = dac_for_hull(30);
    s.weapons = {
        make_fusion("Fusion FA"),
        make_ph1("Ph-1 A-1"), make_ph1("Ph-1 A-2"),
        make_ph1("Ph-1 B-1"), make_ph1("Ph-1 B-2"),
        make_hellbore("Hellbore A"), make_hellbore("Hellbore B"),
        make_ph3("Ph-G 1"), make_ph3("Ph-G 2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Hunter (hydran_hn) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_hydran_hn(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Hydran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 8;
    s.sys.max_impulse_power = 1;
    s.eaf.warp_power = 8; s.eaf.impulse_power = 1;
    s.sys.warp_groups    = {{"L.Warp",4,4},{"R.Warp",4,4}};
    s.sys.impulse_groups = {{"Impulse",1,1}};
    s.sys.apr_rated = s.sys.apr_current = 1;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 6; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 8;
    s.sys.shields_max[1] = s.sys.shields[1] = 6;
    s.sys.shields_max[2] = s.sys.shields[2] = 4;
    s.sys.shields_max[3] = s.sys.shields[3] = 4;
    s.sys.shields_max[4] = s.sys.shields[4] = 4;
    s.sys.shields_max[5] = s.sys.shields[5] = 6;
    s.sys.hull = s.sys.hull_max = 10;
    s.dac = dac_for_hull(10);
    s.weapons = {
        make_ph3("Ph-G FA"),
        make_ph3("Ph-G LS"),
        make_ph3("Ph-G RS"),
        make_fusion("Fusion FA-1"), make_fusion("Fusion FA-2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ TAR Tartar Medium Cruiser (hydran_tar) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_hydran_tar(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Hydran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 16;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 16; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",8,8},{"R.Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 3;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 10; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 28;
    s.dac = dac_for_hull(30);
    s.weapons = {
        make_ph1("Ph-1 FA"),
        make_hellbore("HB FA"),
        make_ph2("Ph-2 A-1"), make_ph2("Ph-2 A-2"),
        make_ph2("Ph-2 B-1"), make_ph2("Ph-2 B-2"),
        make_ph3("Ph-G 1"), make_ph3("Ph-G 2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ NSC Chasseur New Scout Cruiser (hydran_nsc) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_hydran_nsc(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Hydran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 14;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 14; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",7,7},{"R.Warp",7,7}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 3;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 6; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 26;
    s.dac = dac_for_hull(25);
    s.weapons = {
        make_ph2("Ph-2 A-1"), make_ph2("Ph-2 A-2"),
        make_ph2("Ph-2 B-1"), make_ph2("Ph-2 B-2"),
        make_ph3("Ph-G 1"), make_ph3("Ph-G 2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ COS Cossack Medium Carrier (hydran_cos) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_hydran_cos(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Hydran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 16;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 16; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",8,8},{"R.Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 3;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 10; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 14;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 14;
    s.sys.hull = s.sys.hull_max = 28;
    s.dac = dac_for_hull(30);
    s.weapons = {
        make_ph1("Ph-1 FA"),
        make_ph2("Ph-2 A-1"), make_ph2("Ph-2 A-2"),
        make_ph2("Ph-2 B-1"), make_ph2("Ph-2 B-2"),
        make_ph3("Ph-G 1"), make_ph3("Ph-G 2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ COM Comanche Medium Command Cruiser (hydran_com) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_hydran_com(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Hydran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 16;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 16; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",8,8},{"R.Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 3;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 10; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 28;
    s.dac = dac_for_hull(30);
    s.weapons = {
        make_hellbore("HB FA-1"), make_hellbore("HB FA-2"),
        make_fusion("Fusion FA"),
        make_ph1("Ph-1 A-1"), make_ph1("Ph-1 A-2"),
        make_ph1("Ph-1 B-1"), make_ph1("Ph-1 B-2"),
        make_ph3("Ph-G 1"), make_ph3("Ph-G 2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ CVE Scythian Escort Carrier (hydran_cve) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_hydran_cve(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Hydran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 8;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 8; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",4,4},{"R.Warp",4,4}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 1;
    s.sys.crew_total = 6; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 12;
    s.sys.shields_max[1] = s.sys.shields[1] = 8;
    s.sys.shields_max[2] = s.sys.shields[2] = 4;
    s.sys.shields_max[3] = s.sys.shields[3] = 6;
    s.sys.shields_max[4] = s.sys.shields[4] = 4;
    s.sys.shields_max[5] = s.sys.shields[5] = 8;
    s.sys.hull = s.sys.hull_max = 18;
    s.dac = dac_for_hull(20);
    s.weapons = {
        make_ph3("Ph-G 1"), make_ph3("Ph-G 2"),
        make_ph2("Ph-2"),
        make_ph3("Ph-G FA"),
        make_ph1("Ph-1 360"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Traveler Light Cruiser (hydran_tr) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_hydran_tr(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Hydran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 16;
    s.sys.max_impulse_power = 3;
    s.eaf.warp_power = 16; s.eaf.impulse_power = 3;
    s.sys.warp_groups    = {{"L.Warp",8,8},{"R.Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",3,3}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 10; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 16;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 10;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 10;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 31;
    s.dac = dac_for_hull(30);
    s.weapons = {
        make_ph3("Ph-G FA-1"), make_ph3("Ph-G FA-2"),
        make_ph1("Ph-1 LS"),
        make_ph1("Ph-1 RS"),
        make_hellbore("Hellbore FA-1"), make_hellbore("Hellbore FA-2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ New PF Tender (hydran_npf) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_hydran_npf(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Hydran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 14;
    s.sys.max_impulse_power = 3;
    s.eaf.warp_power = 14; s.eaf.impulse_power = 3;
    s.sys.warp_groups    = {{"L.Warp",7,7},{"R.Warp",7,7}};
    s.sys.impulse_groups = {{"Impulse",3,3}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 10; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 16;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 10;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 10;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 30;
    s.dac = dac_for_hull(30);
    s.weapons = {
        make_fusion("Fusion Beam-1"), make_fusion("Fusion Beam-2"),
        make_ph3("Ph-G 1"), make_ph3("Ph-G 2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ New Aegis Cruiser (hydran_nac) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_hydran_nac(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Hydran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 14;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 14; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",7,7},{"R.Warp",7,7}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 3;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 6; s.sys.shuttles = 6;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 26;
    s.dac = dac_for_hull(25);
    s.weapons = {
        make_fusion("Fusion Beam-1"), make_fusion("Fusion Beam-2"),
        make_ph1("Ph-1 A-1"), make_ph1("Ph-1 A-2"),
        make_ph3("Ph-G 1"), make_ph3("Ph-G 2"),
        make_ph3("Ph-G 3"), make_ph3("Ph-G 4"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Outrider Light Carrier (hydran_srv) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_hydran_srv(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Hydran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 8;
    s.sys.max_impulse_power = 3;
    s.eaf.warp_power = 8; s.eaf.impulse_power = 3;
    s.sys.warp_groups    = {{"L.Warp",4,4},{"R.Warp",4,4}};
    s.sys.impulse_groups = {{"Impulse",3,3}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 3;
    s.sys.crew_total = 8; s.sys.shuttles = 8;
    s.sys.shields_max[0] = s.sys.shields[0] = 14;
    s.sys.shields_max[1] = s.sys.shields[1] = 10;
    s.sys.shields_max[2] = s.sys.shields[2] = 6;
    s.sys.shields_max[3] = s.sys.shields[3] = 8;
    s.sys.shields_max[4] = s.sys.shields[4] = 6;
    s.sys.shields_max[5] = s.sys.shields[5] = 10;
    s.sys.hull = s.sys.hull_max = 20;
    s.dac = dac_for_hull(20);
    s.weapons = {
        make_fusion("Fusion Beam-1"), make_fusion("Fusion Beam-2"),
        make_ph3("Ph-G 1"), make_ph3("Ph-G 2"),
        make_ph3("Ph-G 3"), make_ph3("Ph-G 4"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Cuirassier (hydran_cu) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_hydran_cu(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Hydran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 8;
    s.sys.max_impulse_power = 1;
    s.eaf.warp_power = 8; s.eaf.impulse_power = 1;
    s.sys.warp_groups    = {{"L.Warp",4,4},{"R.Warp",4,4}};
    s.sys.impulse_groups = {{"Impulse",1,1}};
    s.sys.apr_rated = s.sys.apr_current = 1;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 7; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 8;
    s.sys.shields_max[1] = s.sys.shields[1] = 6;
    s.sys.shields_max[2] = s.sys.shields[2] = 4;
    s.sys.shields_max[3] = s.sys.shields[3] = 4;
    s.sys.shields_max[4] = s.sys.shields[4] = 4;
    s.sys.shields_max[5] = s.sys.shields[5] = 6;
    s.sys.hull = s.sys.hull_max = 10;
    s.dac = dac_for_hull(10);
    s.weapons = {
        make_ph3("Ph-G FA"),
        make_ph3("Ph-G LS"),
        make_ph3("Ph-G RS"),
        make_hellbore("Hellbore FA"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Cataphract Commando Cruiser (hydran_cat) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_hydran_cat(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Hydran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 14;
    s.sys.max_impulse_power = 3;
    s.eaf.warp_power = 14; s.eaf.impulse_power = 3;
    s.sys.warp_groups    = {{"L.Warp",7,7},{"R.Warp",7,7}};
    s.sys.impulse_groups = {{"Impulse",3,3}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 10; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 16;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 10;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 10;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 30;
    s.dac = dac_for_hull(30);
    s.weapons = {
        make_fusion("Fusion Beam-1"), make_fusion("Fusion Beam-2"),
        make_fusion("Fusion Beam-3"), make_fusion("Fusion Beam-4"),
        make_ph3("Ph-G 1"), make_ph3("Ph-G 2"),
        make_ph3("Ph-G 3"), make_ph3("Ph-G 4"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ LP Lord Paladin Space Control Ship (hydran_lp) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_hydran_lp(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Hydran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 27;
    s.sys.max_impulse_power = 6;
    s.eaf.warp_power = 27; s.eaf.impulse_power = 6;
    s.sys.warp_groups    = {{"L.Warp",13,13},{"R.Warp",14,14}};
    s.sys.impulse_groups = {{"Impulse",6,6}};
    s.sys.apr_rated = s.sys.apr_current = 5;
    s.sys.battery_cap = 3;
    s.sys.crew_total = 10; s.sys.shuttles = 3;
    s.sys.shields_max[0] = s.sys.shields[0] = 27;
    s.sys.shields_max[1] = s.sys.shields[1] = 20;
    s.sys.shields_max[2] = s.sys.shields[2] = 14;
    s.sys.shields_max[3] = s.sys.shields[3] = 18;
    s.sys.shields_max[4] = s.sys.shields[4] = 14;
    s.sys.shields_max[5] = s.sys.shields[5] = 20;
    s.sys.hull = s.sys.hull_max = 46;
    s.dac = dac_for_hull(45);
    s.weapons = {
        make_fusion("Fusion"),
        make_ph1("Ph-1 FX"),
        make_ph1("Ph-1 A-1"), make_ph1("Ph-1 A-2"),
        make_hellbore("HB FA-1"), make_hellbore("HB FA-2"),
        make_ph3("Ph-G 1"), make_ph3("Ph-G 2"),
        make_ph1("Ph-1 360"),
        make_fusion("Fusion Flag-1"), make_fusion("Fusion Flag-2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Ranger Cruiser (hydran_rn) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_hydran_rn(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Hydran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 16;
    s.sys.max_impulse_power = 3;
    s.eaf.warp_power = 16; s.eaf.impulse_power = 3;
    s.sys.warp_groups    = {{"L.Warp",8,8},{"R.Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",3,3}};
    s.sys.apr_rated = s.sys.apr_current = 3;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 10; s.sys.shuttles = 3;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 14;
    s.sys.shields_max[2] = s.sys.shields[2] = 10;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 10;
    s.sys.shields_max[5] = s.sys.shields[5] = 14;
    s.sys.hull = s.sys.hull_max = 35;
    s.dac = dac_for_hull(35);
    s.weapons = {
        make_ph3("Ph-G FA-1"), make_ph3("Ph-G FA-2"),
        make_ph3("Ph-G LS-1"), make_ph3("Ph-G LS-2"),
        make_ph3("Ph-G RS-1"), make_ph3("Ph-G RS-2"),
        make_fusion("Fusion FA-1"), make_fusion("Fusion FA-2"), make_fusion("Fusion FA-3"),
        make_fusion("Fusion LS-1"), make_fusion("Fusion LS-2"),
        make_fusion("Fusion RS-1"), make_fusion("Fusion RS-2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Uhlan Patrol Carrier (hydran_uh) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_hydran_uh(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Hydran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 12;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 12; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",6,6},{"R.Warp",6,6}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 3;
    s.sys.crew_total = 10; s.sys.shuttles = 4;
    s.sys.shields_max[0] = s.sys.shields[0] = 14;
    s.sys.shields_max[1] = s.sys.shields[1] = 10;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 8;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 10;
    s.sys.hull = s.sys.hull_max = 26;
    s.dac = dac_for_hull(25);
    s.weapons = {
        make_ph3("Ph-G FA-1"), make_ph3("Ph-G FA-2"),
        make_ph3("Ph-G LS"),
        make_ph3("Ph-G RS"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Outrider Commando Ship (hydran_srg) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_hydran_srg(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Hydran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 8;
    s.sys.max_impulse_power = 3;
    s.eaf.warp_power = 8; s.eaf.impulse_power = 3;
    s.sys.warp_groups    = {{"L.Warp",4,4},{"R.Warp",4,4}};
    s.sys.impulse_groups = {{"Impulse",3,3}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 3;
    s.sys.crew_total = 8; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 14;
    s.sys.shields_max[1] = s.sys.shields[1] = 10;
    s.sys.shields_max[2] = s.sys.shields[2] = 6;
    s.sys.shields_max[3] = s.sys.shields[3] = 8;
    s.sys.shields_max[4] = s.sys.shields[4] = 6;
    s.sys.shields_max[5] = s.sys.shields[5] = 10;
    s.sys.hull = s.sys.hull_max = 20;
    s.dac = dac_for_hull(20);
    s.weapons = {
        make_fusion("Fusion Beam-1"), make_fusion("Fusion Beam-2"),
        make_ph3("Ph-G 1"), make_ph3("Ph-G 2"),
        make_ph3("Ph-G 3"), make_ph3("Ph-G 4"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Knight Destroyer (hydran_kn) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_hydran_kn(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Hydran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 12;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 12; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",6,6},{"R.Warp",6,6}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 3;
    s.sys.crew_total = 9; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 14;
    s.sys.shields_max[1] = s.sys.shields[1] = 10;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 8;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 10;
    s.sys.hull = s.sys.hull_max = 23;
    s.dac = dac_for_hull(25);
    s.weapons = {
        make_ph3("Ph-G FA-1"), make_ph3("Ph-G FA-2"),
        make_ph1("Ph-1 LS"),
        make_ph1("Ph-1 RS"),
        make_hellbore("Hellbore FA-1"), make_hellbore("Hellbore FA-2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ GEN Gendarme Police Ship (hydran_gen) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_hydran_gen(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Hydran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 6;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 6; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",3,3},{"R.Warp",3,3}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 1;
    s.sys.battery_cap = 1;
    s.sys.crew_total = 9; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 10;
    s.sys.shields_max[1] = s.sys.shields[1] = 6;
    s.sys.shields_max[2] = s.sys.shields[2] = 4;
    s.sys.shields_max[3] = s.sys.shields[3] = 4;
    s.sys.shields_max[4] = s.sys.shields[4] = 4;
    s.sys.shields_max[5] = s.sys.shields[5] = 6;
    s.sys.hull = s.sys.hull_max = 14;
    s.dac = dac_for_hull(15);
    s.weapons = {
        make_fusion("Fusion"),
        make_ph2("Ph-2"),
        make_ph3("Ph-G RX"),
        make_ph3("Ph-G"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ LC Lord Commander Command Cruiser (hydran_lc) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_hydran_lc(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Hydran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 16;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 16; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",8,8},{"R.Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 3;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 10; s.sys.shuttles = 3;
    s.sys.shields_max[0] = s.sys.shields[0] = 20;
    s.sys.shields_max[1] = s.sys.shields[1] = 14;
    s.sys.shields_max[2] = s.sys.shields[2] = 10;
    s.sys.shields_max[3] = s.sys.shields[3] = 12;
    s.sys.shields_max[4] = s.sys.shields[4] = 10;
    s.sys.shields_max[5] = s.sys.shields[5] = 14;
    s.sys.hull = s.sys.hull_max = 30;
    s.dac = dac_for_hull(30);
    s.weapons = {
        make_fusion("Fusion FA"),
        make_ph1("Ph-1 A-1"), make_ph1("Ph-1 A-2"),
        make_ph1("Ph-1 B-1"), make_ph1("Ph-1 B-2"),
        make_hellbore("Hellbore A"), make_hellbore("Hellbore B"),
        make_fusion("Fusion A"), make_fusion("Fusion B"),
        make_ph3("Ph-G 1"), make_ph3("Ph-G 2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ LTT Mule Light Tactical Transport (hydran_ltt) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_hydran_ltt(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Hydran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 14;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 14; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",7,7},{"R.Warp",7,7}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 3;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 4; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 26;
    s.dac = dac_for_hull(25);
    s.weapons = {
        make_ph2("Ph-2 A-1"), make_ph2("Ph-2 A-2"),
        make_ph2("Ph-2 B-1"), make_ph2("Ph-2 B-2"),
        make_ph3("Ph-G 1"), make_ph3("Ph-G 2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Aegis Lancer (hydran_da) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_hydran_da(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Hydran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 10;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 10; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",5,5},{"R.Warp",5,5}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 3;
    s.sys.crew_total = 8; s.sys.shuttles = 4;
    s.sys.shields_max[0] = s.sys.shields[0] = 12;
    s.sys.shields_max[1] = s.sys.shields[1] = 8;
    s.sys.shields_max[2] = s.sys.shields[2] = 6;
    s.sys.shields_max[3] = s.sys.shields[3] = 6;
    s.sys.shields_max[4] = s.sys.shields[4] = 6;
    s.sys.shields_max[5] = s.sys.shields[5] = 8;
    s.sys.hull = s.sys.hull_max = 22;
    s.dac = dac_for_hull(20);
    s.weapons = {
        make_ph3("Ph-G FA-1"), make_ph3("Ph-G FA-2"),
        make_ph1("Ph-1 LS"),
        make_ph1("Ph-1 RS"),
        make_fusion("Fusion FA-1"), make_fusion("Fusion FA-2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Escort Hunter (hydran_eh) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_hydran_eh(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Hydran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 8;
    s.sys.max_impulse_power = 1;
    s.eaf.warp_power = 8; s.eaf.impulse_power = 1;
    s.sys.warp_groups    = {{"L.Warp",4,4},{"R.Warp",4,4}};
    s.sys.impulse_groups = {{"Impulse",1,1}};
    s.sys.apr_rated = s.sys.apr_current = 1;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 6; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 8;
    s.sys.shields_max[1] = s.sys.shields[1] = 6;
    s.sys.shields_max[2] = s.sys.shields[2] = 4;
    s.sys.shields_max[3] = s.sys.shields[3] = 4;
    s.sys.shields_max[4] = s.sys.shields[4] = 4;
    s.sys.shields_max[5] = s.sys.shields[5] = 6;
    s.sys.hull = s.sys.hull_max = 10;
    s.dac = dac_for_hull(10);
    s.weapons = {
        make_ph1("Ph-1 FA-1"), make_ph1("Ph-1 FA-2"),
        make_ph3("Ph-G LS"),
        make_ph3("Ph-G RS"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Lord Marshal Command Cruiser (hydran_lm) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_hydran_lm(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Hydran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 18;
    s.sys.max_impulse_power = 3;
    s.eaf.warp_power = 18; s.eaf.impulse_power = 3;
    s.sys.warp_groups    = {{"L.Warp",9,9},{"R.Warp",9,9}};
    s.sys.impulse_groups = {{"Impulse",3,3}};
    s.sys.apr_rated = s.sys.apr_current = 3;
    s.sys.battery_cap = 5;
    s.sys.crew_total = 10; s.sys.shuttles = 3;
    s.sys.shields_max[0] = s.sys.shields[0] = 22;
    s.sys.shields_max[1] = s.sys.shields[1] = 18;
    s.sys.shields_max[2] = s.sys.shields[2] = 14;
    s.sys.shields_max[3] = s.sys.shields[3] = 14;
    s.sys.shields_max[4] = s.sys.shields[4] = 14;
    s.sys.shields_max[5] = s.sys.shields[5] = 18;
    s.sys.hull = s.sys.hull_max = 40;
    s.dac = dac_for_hull(40);
    s.weapons = {
        make_ph1("Ph-1 FA-1"), make_ph1("Ph-1 FA-2"),
        make_ph1("Ph-1 LS"),
        make_ph1("Ph-1 RS"),
        make_hellbore("Hellbore FA-1"), make_hellbore("Hellbore FA-2"),
        make_fusion("Fusion FA-1"), make_fusion("Fusion FA-2"),
        make_ph3("Ph-G 1"), make_ph3("Ph-G 2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Lancer Destroyer (hydran_ln) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_hydran_ln(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Hydran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 10;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 10; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",5,5},{"R.Warp",5,5}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 3;
    s.sys.crew_total = 8; s.sys.shuttles = 3;
    s.sys.shields_max[0] = s.sys.shields[0] = 12;
    s.sys.shields_max[1] = s.sys.shields[1] = 8;
    s.sys.shields_max[2] = s.sys.shields[2] = 6;
    s.sys.shields_max[3] = s.sys.shields[3] = 6;
    s.sys.shields_max[4] = s.sys.shields[4] = 6;
    s.sys.shields_max[5] = s.sys.shields[5] = 8;
    s.sys.hull = s.sys.hull_max = 22;
    s.dac = dac_for_hull(20);
    s.weapons = {
        make_ph3("Ph-G FA-1"), make_ph3("Ph-G FA-2"),
        make_ph3("Ph-G LS"),
        make_ph3("Ph-G RS"),
        make_fusion("Fusion FA-1"), make_fusion("Fusion FA-2"),
        make_fusion("Fusion LS"),
        make_fusion("Fusion RS"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Hydran D7H Cruiser Anarchist (hydran_d7h) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_hydran_d7h(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Hydran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 18;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 18; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",9,9},{"R.Warp",9,9}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 5;
    s.sys.battery_cap = 3;
    s.sys.crew_total = 8; s.sys.shuttles = 5;
    s.sys.shields_max[0] = s.sys.shields[0] = 22;
    s.sys.shields_max[1] = s.sys.shields[1] = 22;
    s.sys.shields_max[2] = s.sys.shields[2] = 18;
    s.sys.shields_max[3] = s.sys.shields[3] = 22;
    s.sys.shields_max[4] = s.sys.shields[4] = 18;
    s.sys.shields_max[5] = s.sys.shields[5] = 22;
    s.sys.hull = s.sys.hull_max = 8;
    s.dac = dac_for_hull(10);
    s.weapons = {
        make_ph2("Ph-2 A-1"), make_ph2("Ph-2 A-2"),
        make_ph2("Ph-2 B-1"), make_ph2("Ph-2 B-2"),
        make_fusion("Fusion A"), make_fusion("Fusion B"),
        make_ph3("Ph-G 1"), make_ph3("Ph-G 2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ WAR Warrior Destroyer Leader (hydran_war) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_hydran_war(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Hydran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 8;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 8; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",4,4},{"R.Warp",4,4}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 1;
    s.sys.crew_total = 10; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 12;
    s.sys.shields_max[1] = s.sys.shields[1] = 8;
    s.sys.shields_max[2] = s.sys.shields[2] = 4;
    s.sys.shields_max[3] = s.sys.shields[3] = 6;
    s.sys.shields_max[4] = s.sys.shields[4] = 4;
    s.sys.shields_max[5] = s.sys.shields[5] = 8;
    s.sys.hull = s.sys.hull_max = 18;
    s.dac = dac_for_hull(20);
    s.weapons = {
        make_ph1("Ph-1 FA"),
        make_hellbore("HB FA-1"), make_hellbore("HB FA-2"),
        make_fusion("Fusion A"), make_fusion("Fusion B"),
        make_ph1("Ph-1"),
        make_ph3("Ph-G 1"), make_ph3("Ph-G 2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Cavalier Interdiction Carrier (hydran_cav) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_hydran_cav(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Hydran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 18;
    s.sys.max_impulse_power = 3;
    s.eaf.warp_power = 18; s.eaf.impulse_power = 3;
    s.sys.warp_groups    = {{"L.Warp",9,9},{"R.Warp",9,9}};
    s.sys.impulse_groups = {{"Impulse",3,3}};
    s.sys.apr_rated = s.sys.apr_current = 3;
    s.sys.battery_cap = 5;
    s.sys.crew_total = 10; s.sys.shuttles = 4;
    s.sys.shields_max[0] = s.sys.shields[0] = 22;
    s.sys.shields_max[1] = s.sys.shields[1] = 18;
    s.sys.shields_max[2] = s.sys.shields[2] = 14;
    s.sys.shields_max[3] = s.sys.shields[3] = 14;
    s.sys.shields_max[4] = s.sys.shields[4] = 14;
    s.sys.shields_max[5] = s.sys.shields[5] = 18;
    s.sys.hull = s.sys.hull_max = 40;
    s.dac = dac_for_hull(40);
    s.weapons = {
        make_ph3("Ph-G 1"), make_ph3("Ph-G 2"),
        make_ph3("Ph-G 3"), make_ph3("Ph-G 4"),
        make_ph1("Ph-1 A-1"), make_ph1("Ph-1 A-2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ NMS New Minesweeper (hydran_nms) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_hydran_nms(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Hydran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 14;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 14; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",7,7},{"R.Warp",7,7}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 3;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 8; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 26;
    s.dac = dac_for_hull(25);
    s.weapons = {
        make_ph2("Ph-2 FA"),
        make_ph2("Ph-2 A-1"), make_ph2("Ph-2 A-2"),
        make_ph2("Ph-2 B-1"), make_ph2("Ph-2 B-2"),
        make_ph3("Ph-G 1"), make_ph3("Ph-G 2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ NEC New Escort Cruiser (hydran_nec) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_hydran_nec(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Hydran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 14;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 14; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",7,7},{"R.Warp",7,7}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 3;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 6; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 26;
    s.dac = dac_for_hull(25);
    s.weapons = {
        make_ph1("Ph-1 FA"),
        make_ph3("Ph-G FX"),
        make_ph2("Ph-2 A-1"), make_ph2("Ph-2 A-2"),
        make_ph2("Ph-2 B-1"), make_ph2("Ph-2 B-2"),
        make_ph3("Ph-G 1"), make_ph3("Ph-G 2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Escort Lancer (hydran_de) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_hydran_de(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Hydran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 10;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 10; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",5,5},{"R.Warp",5,5}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 3;
    s.sys.crew_total = 8; s.sys.shuttles = 4;
    s.sys.shields_max[0] = s.sys.shields[0] = 12;
    s.sys.shields_max[1] = s.sys.shields[1] = 8;
    s.sys.shields_max[2] = s.sys.shields[2] = 6;
    s.sys.shields_max[3] = s.sys.shields[3] = 6;
    s.sys.shields_max[4] = s.sys.shields[4] = 6;
    s.sys.shields_max[5] = s.sys.shields[5] = 8;
    s.sys.hull = s.sys.hull_max = 22;
    s.dac = dac_for_hull(20);
    s.weapons = {
        make_ph3("Ph-G FA-1"), make_ph3("Ph-G FA-2"),
        make_ph1("Ph-1 LS"),
        make_ph1("Ph-1 RS"),
        make_fusion("Fusion FA-1"), make_fusion("Fusion FA-2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ SR Outrider Survey Ship (hydran_sr) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_hydran_sr(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Hydran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 8;
    s.sys.max_impulse_power = 3;
    s.eaf.warp_power = 8; s.eaf.impulse_power = 3;
    s.sys.warp_groups    = {{"L.Warp",4,4},{"R.Warp",4,4}};
    s.sys.impulse_groups = {{"Impulse",3,3}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 3;
    s.sys.crew_total = 8; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 14;
    s.sys.shields_max[1] = s.sys.shields[1] = 10;
    s.sys.shields_max[2] = s.sys.shields[2] = 6;
    s.sys.shields_max[3] = s.sys.shields[3] = 8;
    s.sys.shields_max[4] = s.sys.shields[4] = 6;
    s.sys.shields_max[5] = s.sys.shields[5] = 10;
    s.sys.hull = s.sys.hull_max = 20;
    s.dac = dac_for_hull(20);
    s.weapons = {
        make_ph2("Ph-2"),
        make_ph3("Ph-G A"), make_ph3("Ph-G B"),
        make_ph1("Ph-1"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ TGC Cougar Battle Tug (lyran_tgc) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_lyran_tgc(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Lyran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 19;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 19; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",9,9},{"R.Warp",10,10}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 4;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 10; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 28;
    s.dac = dac_for_hull(30);
    s.weapons = {
        make_ph2("Ph-2 FA1"), make_ph2("Ph-2 FA2"),
        make_ph2("Ph-2 FA3"), make_ph2("Ph-2 FA4"),
        make_ph3("Ph-3 A"), make_ph3("Ph-3 B"), make_ph3("Ph-3 C"), make_ph3("Ph-3 D"),
        make_disruptor("Disruptor A"), make_disruptor("Disruptor B"),
        make_disruptor("Disruptor C"), make_disruptor("Disruptor D"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ DWL War Destroyer Leader (lyran_dwl) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_lyran_dwl(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Lyran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 9;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 9; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",4,4},{"R.Warp",5,5}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 1;
    s.sys.crew_total = 10; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 12;
    s.sys.shields_max[1] = s.sys.shields[1] = 9;
    s.sys.shields_max[2] = s.sys.shields[2] = 6;
    s.sys.shields_max[3] = s.sys.shields[3] = 8;
    s.sys.shields_max[4] = s.sys.shields[4] = 6;
    s.sys.shields_max[5] = s.sys.shields[5] = 9;
    s.sys.hull = s.sys.hull_max = 18;
    s.dac = dac_for_hull(20);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_ph1("Ph-1 C"),
        make_ph3("Ph-3 A"), make_ph3("Ph-3 B"),
        make_disruptor("Disruptor A"),
        make_esg("ESG FX"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Cheetah-E Escort Frigate (lyran_ffe) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_lyran_ffe(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Lyran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 8;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 8; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",4,4},{"R.Warp",4,4}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 1;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 10; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 8;
    s.sys.shields_max[1] = s.sys.shields[1] = 6;
    s.sys.shields_max[2] = s.sys.shields[2] = 4;
    s.sys.shields_max[3] = s.sys.shields[3] = 4;
    s.sys.shields_max[4] = s.sys.shields[4] = 4;
    s.sys.shields_max[5] = s.sys.shields[5] = 6;
    s.sys.hull = s.sys.hull_max = 18;
    s.dac = dac_for_hull(20);
    s.weapons = {
        make_disruptor("Disruptor FA1"), make_disruptor("Disruptor FA2"),
        make_ph2("Ph-2 LS"),
        make_ph2("Ph-2 RS"),
        make_esg("ESG"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ SCS Siberian Lion Space Control Ship (lyran_scs) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_lyran_scs(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Lyran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 30;
    s.sys.max_impulse_power = 6;
    s.eaf.warp_power = 30; s.eaf.impulse_power = 6;
    s.sys.warp_groups    = {{"L.Warp",15,15},{"R.Warp",15,15}};
    s.sys.impulse_groups = {{"Impulse",6,6}};
    s.sys.apr_rated = s.sys.apr_current = 5;
    s.sys.battery_cap = 3;
    s.sys.crew_total = 10; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 30;
    s.sys.shields_max[1] = s.sys.shields[1] = 20;
    s.sys.shields_max[2] = s.sys.shields[2] = 14;
    s.sys.shields_max[3] = s.sys.shields[3] = 18;
    s.sys.shields_max[4] = s.sys.shields[4] = 14;
    s.sys.shields_max[5] = s.sys.shields[5] = 20;
    s.sys.hull = s.sys.hull_max = 48;
    s.dac = dac_for_hull(50);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"), make_ph1("Ph-1 C"), make_ph1("Ph-1 D"),
        make_ph3("Ph-3 A"), make_ph3("Ph-3 B"), make_ph3("Ph-3 C"), make_ph3("Ph-3 D"),
        make_disruptor("Disruptor A"), make_disruptor("Disruptor B"),
        make_ph3("DSR-FX"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Siberian Tiger Carrier (lyran_cv) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_lyran_cv(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Lyran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 18;
    s.sys.max_impulse_power = 3;
    s.eaf.warp_power = 18; s.eaf.impulse_power = 3;
    s.sys.warp_groups    = {{"L.Warp",9,9},{"R.Warp",9,9}};
    s.sys.impulse_groups = {{"Impulse",3,3}};
    s.sys.apr_rated = s.sys.apr_current = 3;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 10; s.sys.shuttles = 4;
    s.sys.shields_max[0] = s.sys.shields[0] = 22;
    s.sys.shields_max[1] = s.sys.shields[1] = 18;
    s.sys.shields_max[2] = s.sys.shields[2] = 14;
    s.sys.shields_max[3] = s.sys.shields[3] = 14;
    s.sys.shields_max[4] = s.sys.shields[4] = 14;
    s.sys.shields_max[5] = s.sys.shields[5] = 18;
    s.sys.hull = s.sys.hull_max = 44;
    s.dac = dac_for_hull(45);
    s.weapons = {
        make_disruptor("Disruptor FA1"), make_disruptor("Disruptor FA2"),
        make_ph1("Ph-1 FA1"), make_ph1("Ph-1 FA2"),
        make_ph2("Ph-2 LS1"), make_ph2("Ph-2 LS2"),
        make_ph2("Ph-2 RS1"), make_ph2("Ph-2 RS2"),
        make_esg("ESG A"), make_esg("ESG B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ TGP Puma Transport Tug with Cargo Pallets (lyran_tgp) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_lyran_tgp(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Lyran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 19;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 19; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",9,9},{"R.Warp",10,10}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 4;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 6; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 26;
    s.dac = dac_for_hull(25);
    s.weapons = {
        make_ph2("Ph-2 A"), make_ph2("Ph-2 B"),
        make_ph3("Ph-3 A"), make_ph3("Ph-3 B"),
        make_disruptor("Disruptor A"), make_disruptor("Disruptor B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ CWA Aegis War Cruiser (lyran_cwa) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_lyran_cwa(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Lyran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 16;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 16; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",8,8},{"R.Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 3;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 8; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 26;
    s.dac = dac_for_hull(25);
    s.weapons = {
        make_ph2("Ph-2 A"), make_ph2("Ph-2 B"),
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_ph3("Ph-3 A"), make_ph3("Ph-3 B"),
        make_disruptor("Disruptor A"), make_disruptor("Disruptor B"),
        make_ph1("Ph-1 FX"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ DWM War Destroyer Minesweeper (lyran_dwm) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_lyran_dwm(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Lyran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 9;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 9; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",4,4},{"R.Warp",5,5}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 1;
    s.sys.crew_total = 6; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 12;
    s.sys.shields_max[1] = s.sys.shields[1] = 9;
    s.sys.shields_max[2] = s.sys.shields[2] = 6;
    s.sys.shields_max[3] = s.sys.shields[3] = 8;
    s.sys.shields_max[4] = s.sys.shields[4] = 6;
    s.sys.shields_max[5] = s.sys.shields[5] = 9;
    s.sys.hull = s.sys.hull_max = 18;
    s.dac = dac_for_hull(20);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_ph2("Ph-2 A"), make_ph2("Ph-2 B"),
        make_ph3("Ph-3 A"), make_ph3("Ph-3 B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Alleycat War Destroyer (lyran_dw) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_lyran_dw(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Lyran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 10;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 10; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",5,5},{"R.Warp",5,5}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 3;
    s.sys.crew_total = 9; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 12;
    s.sys.shields_max[1] = s.sys.shields[1] = 10;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 8;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 10;
    s.sys.hull = s.sys.hull_max = 27;
    s.dac = dac_for_hull(25);
    s.weapons = {
        make_disruptor("Disruptor FA1"), make_disruptor("Disruptor FA2"), make_disruptor("Disruptor FA3"),
        make_ph2("Ph-2 LS1"), make_ph2("Ph-2 LS2"),
        make_ph2("Ph-2 RS1"), make_ph2("Ph-2 RS2"),
        make_ph3("Ph-3 LS1"), make_ph3("Ph-3 LS2"),
        make_ph3("Ph-3 RS1"), make_ph3("Ph-3 RS2"),
        make_esg("ESG A"), make_esg("ESG B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ BCH Hellcat Heavy Battlecruiser (lyran_bch) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_lyran_bch(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Lyran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 23;
    s.sys.max_impulse_power = 6;
    s.eaf.warp_power = 23; s.eaf.impulse_power = 6;
    s.sys.warp_groups    = {{"L.Warp",11,11},{"R.Warp",12,12}};
    s.sys.impulse_groups = {{"Impulse",6,6}};
    s.sys.apr_rated = s.sys.apr_current = 4;
    s.sys.battery_cap = 3;
    s.sys.crew_total = 10; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 26;
    s.sys.shields_max[1] = s.sys.shields[1] = 18;
    s.sys.shields_max[2] = s.sys.shields[2] = 12;
    s.sys.shields_max[3] = s.sys.shields[3] = 14;
    s.sys.shields_max[4] = s.sys.shields[4] = 12;
    s.sys.shields_max[5] = s.sys.shields[5] = 18;
    s.sys.hull = s.sys.hull_max = 40;
    s.dac = dac_for_hull(40);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"), make_ph1("Ph-1 C"), make_ph1("Ph-1 D"),
        make_ph3("Ph-3 A"), make_ph3("Ph-3 B"), make_ph3("Ph-3 C"), make_ph3("Ph-3 D"),
        make_disruptor("Disruptor A"), make_disruptor("Disruptor B"),
        make_ph3("DSR-FX"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Lion Dreadnought (lyran_dn) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_lyran_dn(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Lyran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 28;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 28; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",14,14},{"R.Warp",14,14}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 4;
    s.sys.battery_cap = 6;
    s.sys.crew_total = 10; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 30;
    s.sys.shields_max[1] = s.sys.shields[1] = 26;
    s.sys.shields_max[2] = s.sys.shields[2] = 20;
    s.sys.shields_max[3] = s.sys.shields[3] = 20;
    s.sys.shields_max[4] = s.sys.shields[4] = 20;
    s.sys.shields_max[5] = s.sys.shields[5] = 26;
    s.sys.hull = s.sys.hull_max = 62;
    s.dac = dac_for_hull(60);
    s.weapons = {
        make_disruptor("Disruptor FA1"), make_disruptor("Disruptor FA2"),
        make_disruptor("Disruptor FA3"), make_disruptor("Disruptor FA4"),
        make_ph1("Ph-1 FA1"), make_ph1("Ph-1 FA2"),
        make_ph1("Ph-1 LS1"), make_ph1("Ph-1 LS2"),
        make_ph1("Ph-1 RS1"), make_ph1("Ph-1 RS2"),
        make_ph2("Ph-2 LS1"), make_ph2("Ph-2 LS2"),
        make_ph2("Ph-2 RS1"), make_ph2("Ph-2 RS2"),
        make_esg("ESG A"), make_esg("ESG B"),
        make_drone("Drone A"), make_drone("Drone B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ MP Military Police Ship (lyran_mp) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_lyran_mp(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Lyran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 9;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 9; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",4,4},{"R.Warp",5,5}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 1;
    s.sys.crew_total = 8; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 12;
    s.sys.shields_max[1] = s.sys.shields[1] = 8;
    s.sys.shields_max[2] = s.sys.shields[2] = 4;
    s.sys.shields_max[3] = s.sys.shields[3] = 6;
    s.sys.shields_max[4] = s.sys.shields[4] = 4;
    s.sys.shields_max[5] = s.sys.shields[5] = 8;
    s.sys.hull = s.sys.hull_max = 18;
    s.dac = dac_for_hull(20);
    s.weapons = {
        make_ph2("Ph-2 FA"),
        make_ph2("Ph-2 A"), make_ph2("Ph-2 B"),
        make_ph3("Ph-3 A"), make_ph3("Ph-3 B"),
        make_disruptor("Disruptor"),
        make_ph3("DSR-FX"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ CVL Yaguarundi Light Carrier (lyran_cvl) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_lyran_cvl(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Lyran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 16;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 16; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",8,8},{"R.Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 3;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 10; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 26;
    s.dac = dac_for_hull(25);
    s.weapons = {
        make_ph2("Ph-2 A"), make_ph2("Ph-2 B"),
        make_ph3("Ph-3 A"), make_ph3("Ph-3 B"),
        make_ph1("Ph-1 FX"),
        make_ph3("DSR-FX"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ LTT Light Tactical Transport (lyran_ltt) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_lyran_ltt(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Lyran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 16;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 16; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",8,8},{"R.Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 3;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 8; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 26;
    s.dac = dac_for_hull(25);
    s.weapons = {
        make_ph2("Ph-2 A"), make_ph2("Ph-2 B"),
        make_ph3("Ph-3 A"), make_ph3("Ph-3 B"),
        make_disruptor("Disruptor"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ POL Manx Police Corvette (lyran_pol) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_lyran_pol(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Lyran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 6;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 6; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",3,3},{"R.Warp",3,3}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 1;
    s.sys.battery_cap = 1;
    s.sys.crew_total = 4; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 10;
    s.sys.shields_max[1] = s.sys.shields[1] = 6;
    s.sys.shields_max[2] = s.sys.shields[2] = 4;
    s.sys.shields_max[3] = s.sys.shields[3] = 4;
    s.sys.shields_max[4] = s.sys.shields[4] = 4;
    s.sys.shields_max[5] = s.sys.shields[5] = 6;
    s.sys.hull = s.sys.hull_max = 12;
    s.dac = dac_for_hull(10);
    s.weapons = {
        make_ph2("Ph-2 A"), make_ph2("Ph-2 B"),
        make_ph3("Ph-3"),
        make_disruptor("Disruptor"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ DWA War Destroyer Aegis Escort (lyran_dwa) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_lyran_dwa(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Lyran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 9;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 9; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",4,4},{"R.Warp",5,5}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 1;
    s.sys.crew_total = 6; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 12;
    s.sys.shields_max[1] = s.sys.shields[1] = 9;
    s.sys.shields_max[2] = s.sys.shields[2] = 6;
    s.sys.shields_max[3] = s.sys.shields[3] = 8;
    s.sys.shields_max[4] = s.sys.shields[4] = 6;
    s.sys.shields_max[5] = s.sys.shields[5] = 9;
    s.sys.hull = s.sys.hull_max = 18;
    s.dac = dac_for_hull(20);
    s.weapons = {
        make_ph2("Ph-2 A"), make_ph2("Ph-2 B"),
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_ph3("Ph-3 A"), make_ph3("Ph-3 B"),
        make_ph2("Ph-2 FX"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Saber-Tooth Tiger Mauler (lyran_stt) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_lyran_stt(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Lyran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 20;
    s.sys.max_impulse_power = 3;
    s.eaf.warp_power = 20; s.eaf.impulse_power = 3;
    s.sys.warp_groups    = {{"L.Warp",10,10},{"R.Warp",10,10}};
    s.sys.impulse_groups = {{"Impulse",3,3}};
    s.sys.apr_rated = s.sys.apr_current = 3;
    s.sys.battery_cap = 5;
    s.sys.crew_total = 10; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 24;
    s.sys.shields_max[1] = s.sys.shields[1] = 20;
    s.sys.shields_max[2] = s.sys.shields[2] = 16;
    s.sys.shields_max[3] = s.sys.shields[3] = 16;
    s.sys.shields_max[4] = s.sys.shields[4] = 16;
    s.sys.shields_max[5] = s.sys.shields[5] = 20;
    s.sys.hull = s.sys.hull_max = 44;
    s.dac = dac_for_hull(45);
    s.weapons = {
        make_fusion("Mauler FA1"), make_fusion("Mauler FA2"),
        make_ph1("Ph-1 LS1"), make_ph1("Ph-1 LS2"),
        make_ph1("Ph-1 RS1"), make_ph1("Ph-1 RS2"),
        make_ph2("Ph-2 A"), make_ph2("Ph-2 B"),
        make_esg("ESG A"), make_esg("ESG B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Minesweeper (lyran_ms) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_lyran_ms(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Lyran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 10;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 10; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",5,5},{"R.Warp",5,5}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 10; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 10;
    s.sys.shields_max[1] = s.sys.shields[1] = 8;
    s.sys.shields_max[2] = s.sys.shields[2] = 6;
    s.sys.shields_max[3] = s.sys.shields[3] = 6;
    s.sys.shields_max[4] = s.sys.shields[4] = 6;
    s.sys.shields_max[5] = s.sys.shields[5] = 8;
    s.sys.hull = s.sys.hull_max = 22;
    s.dac = dac_for_hull(20);
    s.weapons = {
        make_disruptor("Disruptor FA"),
        make_ph2("Ph-2 LS1"), make_ph2("Ph-2 LS2"),
        make_ph2("Ph-2 RS1"), make_ph2("Ph-2 RS2"),
        make_esg("ESG"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ CWL War Cruiser Leader (lyran_cwl) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_lyran_cwl(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Lyran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 16;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 16; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",8,8},{"R.Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 3;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 10; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 26;
    s.dac = dac_for_hull(25);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_ph1("Ph-1 C"), make_ph1("Ph-1 D"),
        make_ph3("Ph-3 A"), make_ph3("Ph-3 B"),
        make_disruptor("Disruptor A"), make_disruptor("Disruptor B"),
        make_ph3("DSR-FX"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ SR Prairie Cat Survey Ship (lyran_sr) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_lyran_sr(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Lyran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 19;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 19; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",9,9},{"R.Warp",10,10}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 4;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 8; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 24;
    s.dac = dac_for_hull(25);
    s.weapons = {
        make_ph2("Ph-2 A"), make_ph2("Ph-2 B"),
        make_ph3("Ph-3 A"), make_ph3("Ph-3 B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ DWE War Destroyer Escort (lyran_dwe) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_lyran_dwe(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Lyran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 9;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 9; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",4,4},{"R.Warp",5,5}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 1;
    s.sys.crew_total = 6; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 12;
    s.sys.shields_max[1] = s.sys.shields[1] = 9;
    s.sys.shields_max[2] = s.sys.shields[2] = 6;
    s.sys.shields_max[3] = s.sys.shields[3] = 8;
    s.sys.shields_max[4] = s.sys.shields[4] = 6;
    s.sys.shields_max[5] = s.sys.shields[5] = 9;
    s.sys.hull = s.sys.hull_max = 18;
    s.dac = dac_for_hull(20);
    s.weapons = {
        make_ph2("Ph-2 A"), make_ph2("Ph-2 B"),
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_ph3("Ph-3 A"), make_ph3("Ph-3 B"),
        make_ph1("Ph-1 FX"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ CWE Escort War Cruiser (lyran_cwe) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_lyran_cwe(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Lyran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 16;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 16; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",8,8},{"R.Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 3;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 8; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 26;
    s.dac = dac_for_hull(25);
    s.weapons = {
        make_ph2("Ph-2 A"), make_ph2("Ph-2 B"),
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_ph3("Ph-3 A"), make_ph3("Ph-3 B"),
        make_disruptor("Disruptor A"), make_disruptor("Disruptor B"),
        make_ph1("Ph-1 FX"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ War PF Tender (lyran_pfw) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_lyran_pfw(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Lyran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 16;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 16; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",8,8},{"R.Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 3;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 8; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 26;
    s.dac = dac_for_hull(25);
    s.weapons = {
        make_esg("ESG A"), make_esg("ESG B"),
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_ph2("Ph-2 A"), make_ph2("Ph-2 B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ DWS War Destroyer Scout (lyran_dws) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_lyran_dws(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Lyran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 9;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 9; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",4,4},{"R.Warp",5,5}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 1;
    s.sys.crew_total = 6; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 12;
    s.sys.shields_max[1] = s.sys.shields[1] = 9;
    s.sys.shields_max[2] = s.sys.shields[2] = 6;
    s.sys.shields_max[3] = s.sys.shields[3] = 8;
    s.sys.shields_max[4] = s.sys.shields[4] = 6;
    s.sys.shields_max[5] = s.sys.shields[5] = 9;
    s.sys.hull = s.sys.hull_max = 18;
    s.dac = dac_for_hull(20);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_ph2("Ph-2 A"), make_ph2("Ph-2 B"),
        make_ph3("Ph-3 A"), make_ph3("Ph-3 B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ STJ Single-Tooth Jaguar War Mauler (lyran_stj) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_lyran_stj(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Lyran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 16;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 16; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",8,8},{"R.Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 3;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 10; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 26;
    s.dac = dac_for_hull(25);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_ph1("Ph-1 C"), make_ph1("Ph-1 D"),
        make_ph3("Ph-3 A"), make_ph3("Ph-3 B"), make_ph3("Ph-3 C"), make_ph3("Ph-3 D"),
        make_fusion("Mauler"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Prairie Lion Survey Carrier (lyran_srv) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_lyran_srv(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Lyran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 19;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 19; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",9,9},{"R.Warp",10,10}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 4;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 10; s.sys.shuttles = 12;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 26;
    s.dac = dac_for_hull(25);
    s.weapons = {
        make_esg("ESG A"), make_esg("ESG B"),
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_ph2("Ph-2 A"), make_ph2("Ph-2 B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ CC Bengal Tiger Command Cruiser (lyran_cc) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_lyran_cc(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Lyran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 18;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 18; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",9,9},{"R.Warp",9,9}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 4;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 10; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 20;
    s.sys.shields_max[1] = s.sys.shields[1] = 14;
    s.sys.shields_max[2] = s.sys.shields[2] = 10;
    s.sys.shields_max[3] = s.sys.shields[3] = 12;
    s.sys.shields_max[4] = s.sys.shields[4] = 10;
    s.sys.shields_max[5] = s.sys.shields[5] = 14;
    s.sys.hull = s.sys.hull_max = 30;
    s.dac = dac_for_hull(30);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_ph1("Ph-1 C"), make_ph1("Ph-1 D"),
        make_ph3("Ph-3 A"), make_ph3("Ph-3 B"), make_ph3("Ph-3 C"), make_ph3("Ph-3 D"),
        make_disruptor("Disruptor A"), make_disruptor("Disruptor B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Jaguar War Cruiser (lyran_cw) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_lyran_cw(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Lyran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 14;
    s.sys.max_impulse_power = 3;
    s.eaf.warp_power = 14; s.eaf.impulse_power = 3;
    s.sys.warp_groups    = {{"L.Warp",7,7},{"R.Warp",7,7}};
    s.sys.impulse_groups = {{"Impulse",3,3}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 10; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 16;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 10;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 10;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 34;
    s.dac = dac_for_hull(35);
    s.weapons = {
        make_disruptor("Disruptor FA1"), make_disruptor("Disruptor FA2"), make_disruptor("Disruptor FA3"),
        make_ph2("Ph-2 LS1"), make_ph2("Ph-2 LS2"),
        make_ph2("Ph-2 RS1"), make_ph2("Ph-2 RS2"),
        make_ph3("Ph-3 LS1"), make_ph3("Ph-3 LS2"),
        make_ph3("Ph-3 RS1"), make_ph3("Ph-3 RS2"),
        make_esg("ESG A"), make_esg("ESG B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ LTV Light Carrier Transport (lyran_ltv) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_lyran_ltv(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Lyran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 16;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 16; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",8,8},{"R.Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 3;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 10; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 26;
    s.dac = dac_for_hull(25);
    s.weapons = {
        make_ph2("Ph-2 A"), make_ph2("Ph-2 B"),
        make_ph3("Ph-3 A"), make_ph3("Ph-3 B"),
        make_disruptor("Disruptor A"), make_disruptor("Disruptor B"),
        make_ph3("DSR-FX"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Cheetah-A Aegis Frigate (lyran_ffa) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_lyran_ffa(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Lyran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 8;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 8; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",4,4},{"R.Warp",4,4}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 1;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 10; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 8;
    s.sys.shields_max[1] = s.sys.shields[1] = 6;
    s.sys.shields_max[2] = s.sys.shields[2] = 4;
    s.sys.shields_max[3] = s.sys.shields[3] = 4;
    s.sys.shields_max[4] = s.sys.shields[4] = 4;
    s.sys.shields_max[5] = s.sys.shields[5] = 6;
    s.sys.hull = s.sys.hull_max = 18;
    s.dac = dac_for_hull(20);
    s.weapons = {
        make_disruptor("Disruptor FA1"), make_disruptor("Disruptor FA2"),
        make_ph2("Ph-2 LS"),
        make_ph2("Ph-2 RS"),
        make_esg("ESG"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Ocelot Scout (lyran_sc) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_lyran_sc(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Lyran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 10;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 10; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",5,5},{"R.Warp",5,5}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 10; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 10;
    s.sys.shields_max[1] = s.sys.shields[1] = 8;
    s.sys.shields_max[2] = s.sys.shields[2] = 6;
    s.sys.shields_max[3] = s.sys.shields[3] = 6;
    s.sys.shields_max[4] = s.sys.shields[4] = 6;
    s.sys.shields_max[5] = s.sys.shields[5] = 8;
    s.sys.hull = s.sys.hull_max = 25;
    s.dac = dac_for_hull(25);
    s.weapons = {
        make_disruptor("Disruptor FA"),
        make_ph2("Ph-2 LS1"), make_ph2("Ph-2 LS2"),
        make_ph2("Ph-2 RS1"), make_ph2("Ph-2 RS2"),
        make_esg("ESG"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ CWM War Minesweeper (lyran_cwm) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_lyran_cwm(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Lyran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 16;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 16; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",8,8},{"R.Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 3;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 8; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 26;
    s.dac = dac_for_hull(25);
    s.weapons = {
        make_ph2("Ph-2 A"), make_ph2("Ph-2 B"),
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_ph3("Ph-3 A"), make_ph3("Ph-3 B"),
        make_ph2("Ph-2 FX"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ CWS Serval War Cruiser Scout (lyran_cws) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_lyran_cws(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Lyran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 16;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 16; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",8,8},{"R.Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 3;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 8; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 26;
    s.dac = dac_for_hull(25);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_ph2("Ph-2 A"), make_ph2("Ph-2 B"),
        make_ph3("Ph-3 A"), make_ph3("Ph-3 B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ WYN Orion Battle Raider (OBR) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_wyn_obr(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Orion;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 16;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 16; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",8,8},{"R.Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 10; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 14;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 10;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 10;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 26;
    s.dac = dac_for_hull(25);
    s.weapons = {
        make_disruptor("Disruptor FA1"), make_disruptor("Disruptor FA2"),
        make_ph3("Ph-G LS1"), make_ph3("Ph-G LS2"),
        make_ph3("Ph-G RS1"), make_ph3("Ph-G RS2"),
        make_drone("Drone 1"), make_drone("Drone 2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ WYN (Lyran) Destroyer (LDD) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_wyn_ldd(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Lyran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 12;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 12; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",6,6},{"R.Warp",6,6}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 3;
    s.sys.crew_total = 10; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 14;
    s.sys.shields_max[1] = s.sys.shields[1] = 10;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 8;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 10;
    s.sys.hull = s.sys.hull_max = 24;
    s.dac = dac_for_hull(25);
    s.weapons = {
        make_disruptor("Disruptor FA1"), make_disruptor("Disruptor FA2"),
        make_ph1("Ph-1 FA"),
        make_ph2("Ph-2 LS1"), make_ph2("Ph-2 LS2"),
        make_ph2("Ph-2 RS1"), make_ph2("Ph-2 RS2"),
        make_esg("ESG 1"), make_esg("ESG 2"),
        make_drone("Drone 1"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ WYN Orion Light Raider (OLR) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_wyn_olr(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Orion;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 8;
    s.sys.max_impulse_power = 1;
    s.eaf.warp_power = 8; s.eaf.impulse_power = 1;
    s.sys.warp_groups    = {{"L.Warp",4,4},{"R.Warp",4,4}};
    s.sys.impulse_groups = {{"Impulse",1,1}};
    s.sys.apr_rated = s.sys.apr_current = 1;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 6; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 8;
    s.sys.shields_max[1] = s.sys.shields[1] = 6;
    s.sys.shields_max[2] = s.sys.shields[2] = 4;
    s.sys.shields_max[3] = s.sys.shields[3] = 4;
    s.sys.shields_max[4] = s.sys.shields[4] = 4;
    s.sys.shields_max[5] = s.sys.shields[5] = 6;
    s.sys.hull = s.sys.hull_max = 12;
    s.dac = dac_for_hull(10);
    s.weapons = {
        make_disruptor("Disruptor FA"),
        make_ph3("Ph-G LS"),
        make_ph3("Ph-G RS"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ WYN (Klingon) Gunboat (KG2) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_wyn_kg2(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Klingon;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 6;
    s.sys.max_impulse_power = 1;
    s.eaf.warp_power = 6; s.eaf.impulse_power = 1;
    s.sys.warp_groups    = {{"L.Warp",3,3},{"R.Warp",3,3}};
    s.sys.impulse_groups = {{"Impulse",1,1}};
    s.sys.apr_rated = s.sys.apr_current = 1;
    s.sys.battery_cap = 1;
    s.sys.crew_total = 5; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 6;
    s.sys.shields_max[1] = s.sys.shields[1] = 5;
    s.sys.shields_max[2] = s.sys.shields[2] = 4;
    s.sys.shields_max[3] = s.sys.shields[3] = 4;
    s.sys.shields_max[4] = s.sys.shields[4] = 4;
    s.sys.shields_max[5] = s.sys.shields[5] = 5;
    s.sys.hull = s.sys.hull_max = 10;
    s.dac = dac_for_hull(10);
    s.weapons = {
        make_disruptor("Disruptor FA"),
        make_ph3("Ph-3 LS1"), make_ph3("Ph-3 LS2"),
        make_ph3("Ph-3 RS1"), make_ph3("Ph-3 RS2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ WYN Auxiliary Minesweeper (AxMS) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_wyn_axms(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 6;
    s.sys.max_impulse_power = 1;
    s.eaf.warp_power = 6; s.eaf.impulse_power = 1;
    s.sys.warp_groups    = {{"L.Warp",3,3},{"R.Warp",3,3}};
    s.sys.impulse_groups = {{"Impulse",1,1}};
    s.sys.apr_rated = s.sys.apr_current = 1;
    s.sys.battery_cap = 1;
    s.sys.crew_total = 10; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 6;
    s.sys.shields_max[1] = s.sys.shields[1] = 4;
    s.sys.shields_max[2] = s.sys.shields[2] = 3;
    s.sys.shields_max[3] = s.sys.shields[3] = 3;
    s.sys.shields_max[4] = s.sys.shields[4] = 3;
    s.sys.shields_max[5] = s.sys.shields[5] = 4;
    s.sys.hull = s.sys.hull_max = 8;
    s.dac = dac_for_hull(10);
    s.weapons = {
        make_ph2("Ph-2 FA"),
        make_ph3("Ph-3 LS"),
        make_ph3("Ph-3 RS"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ ODR Double Raider â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_wyn_odr(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Orion;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 8;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 8; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",4,4},{"R.Warp",4,4}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 1;
    s.sys.crew_total = 10; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 12;
    s.sys.shields_max[1] = s.sys.shields[1] = 8;
    s.sys.shields_max[2] = s.sys.shields[2] = 4;
    s.sys.shields_max[3] = s.sys.shields[3] = 6;
    s.sys.shields_max[4] = s.sys.shields[4] = 4;
    s.sys.shields_max[5] = s.sys.shields[5] = 8;
    s.sys.hull = s.sys.hull_max = 16;
    s.dac = dac_for_hull(15);
    s.weapons = {
        make_ph1("Ph-1"),
        make_ph3("Ph-3 1"), make_ph3("Ph-3 2"),
        make_drone("Drone 1"), make_drone("Drone 2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ WYN Auxiliary Battlecruiser (AxBC) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_wyn_axbc(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 12;
    s.sys.max_impulse_power = 3;
    s.eaf.warp_power = 12; s.eaf.impulse_power = 3;
    s.sys.warp_groups    = {{"L.Warp",6,6},{"R.Warp",6,6}};
    s.sys.impulse_groups = {{"Impulse",3,3}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 3;
    s.sys.crew_total = 10; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 14;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 10;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 10;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 20;
    s.dac = dac_for_hull(20);
    s.weapons = {
        make_ph1("Ph-1 FA1"), make_ph1("Ph-1 FA2"),
        make_ph2("Ph-2 LS1"), make_ph2("Ph-2 LS2"),
        make_ph2("Ph-2 RS1"), make_ph2("Ph-2 RS2"),
        make_disruptor("Disruptor FA1"), make_disruptor("Disruptor FA2"),
        make_drone("Drone 1"), make_drone("Drone 2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ WYN Pocket Battleship (PBB) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_wyn_pbb(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Lyran;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 16;
    s.sys.max_impulse_power = 3;
    s.eaf.warp_power = 16; s.eaf.impulse_power = 3;
    s.sys.warp_groups    = {{"L.Warp",8,8},{"R.Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",3,3}};
    s.sys.apr_rated = s.sys.apr_current = 3;
    s.sys.battery_cap = 5;
    s.sys.crew_total = 10; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 20;
    s.sys.shields_max[1] = s.sys.shields[1] = 16;
    s.sys.shields_max[2] = s.sys.shields[2] = 14;
    s.sys.shields_max[3] = s.sys.shields[3] = 14;
    s.sys.shields_max[4] = s.sys.shields[4] = 14;
    s.sys.shields_max[5] = s.sys.shields[5] = 16;
    s.sys.hull = s.sys.hull_max = 40;
    s.dac = dac_for_hull(40);
    s.weapons = {
        make_disruptor("Disruptor FA1"), make_disruptor("Disruptor FA2"), make_disruptor("Disruptor FA3"),
        make_ph1("Ph-1 FA1"), make_ph1("Ph-1 FA2"),
        make_ph1("Ph-1 LS1"), make_ph1("Ph-1 LS2"),
        make_ph1("Ph-1 RS1"), make_ph1("Ph-1 RS2"),
        make_esg("ESG 1"), make_esg("ESG 2"),
        make_drone("Drone 1"), make_drone("Drone 2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ KE4 Frigate (Captured Klingon Penal Ship) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_wyn_ke4(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Klingon;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 6;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 6; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",3,3},{"R.Warp",3,3}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 1;
    s.sys.battery_cap = 1;
    s.sys.crew_total = 4; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 10;
    s.sys.shields_max[1] = s.sys.shields[1] = 7;
    s.sys.shields_max[2] = s.sys.shields[2] = 4;
    s.sys.shields_max[3] = s.sys.shields[3] = 6;
    s.sys.shields_max[4] = s.sys.shields[4] = 4;
    s.sys.shields_max[5] = s.sys.shields[5] = 7;
    s.sys.hull = s.sys.hull_max = 12;
    s.dac = dac_for_hull(10);
    s.weapons = {
        make_ph2("Ph-2 A"), make_ph2("Ph-2 B"),
        make_ph2("Ph-2-RX"),
        make_drone("Drone 1"), make_drone("Drone 2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ WYN Auxiliary Cruiser (AxC) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_wyn_axc(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 8;
    s.sys.max_impulse_power = 1;
    s.eaf.warp_power = 8; s.eaf.impulse_power = 1;
    s.sys.warp_groups    = {{"L.Warp",4,4},{"R.Warp",4,4}};
    s.sys.impulse_groups = {{"Impulse",1,1}};
    s.sys.apr_rated = s.sys.apr_current = 1;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 10; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 8;
    s.sys.shields_max[1] = s.sys.shields[1] = 6;
    s.sys.shields_max[2] = s.sys.shields[2] = 4;
    s.sys.shields_max[3] = s.sys.shields[3] = 4;
    s.sys.shields_max[4] = s.sys.shields[4] = 4;
    s.sys.shields_max[5] = s.sys.shields[5] = 6;
    s.sys.hull = s.sys.hull_max = 8;
    s.dac = dac_for_hull(10);
    s.weapons = {
        make_ph2("Ph-2 FA1"), make_ph2("Ph-2 FA2"),
        make_ph2("Ph-2 LS"),
        make_ph2("Ph-2 RS"),
        make_drone("Drone 1"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ WYN Auxiliary Carrier (AxCV) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_wyn_axcv(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 8;
    s.sys.max_impulse_power = 1;
    s.eaf.warp_power = 8; s.eaf.impulse_power = 1;
    s.sys.warp_groups    = {{"L.Warp",4,4},{"R.Warp",4,4}};
    s.sys.impulse_groups = {{"Impulse",1,1}};
    s.sys.apr_rated = s.sys.apr_current = 1;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 7; s.sys.shuttles = 4;
    s.sys.shields_max[0] = s.sys.shields[0] = 8;
    s.sys.shields_max[1] = s.sys.shields[1] = 6;
    s.sys.shields_max[2] = s.sys.shields[2] = 4;
    s.sys.shields_max[3] = s.sys.shields[3] = 4;
    s.sys.shields_max[4] = s.sys.shields[4] = 4;
    s.sys.shields_max[5] = s.sys.shields[5] = 6;
    s.sys.hull = s.sys.hull_max = 20;
    s.dac = dac_for_hull(20);
    s.weapons = {
        make_ph2("Ph-2 FA"),
        make_ph2("Ph-2 LS"),
        make_ph2("Ph-2 RS"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ WYN Auxiliary Heavy Carrier (AxCVA) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_wyn_axcva(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Kzinti;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 14;
    s.sys.max_impulse_power = 3;
    s.eaf.warp_power = 14; s.eaf.impulse_power = 3;
    s.sys.warp_groups    = {{"L.Warp",7,7},{"R.Warp",7,7}};
    s.sys.impulse_groups = {{"Impulse",3,3}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 10; s.sys.shuttles = 6;
    s.sys.shields_max[0] = s.sys.shields[0] = 16;
    s.sys.shields_max[1] = s.sys.shields[1] = 14;
    s.sys.shields_max[2] = s.sys.shields[2] = 12;
    s.sys.shields_max[3] = s.sys.shields[3] = 12;
    s.sys.shields_max[4] = s.sys.shields[4] = 12;
    s.sys.shields_max[5] = s.sys.shields[5] = 14;
    s.sys.hull = s.sys.hull_max = 40;
    s.dac = dac_for_hull(40);
    s.weapons = {
        make_ph2("Ph-2 FA1"), make_ph2("Ph-2 FA2"),
        make_ph2("Ph-2 LS1"), make_ph2("Ph-2 LS2"),
        make_ph2("Ph-2 RS1"), make_ph2("Ph-2 RS2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ WYN Orion Cruiser Raider (OCR) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_wyn_ocr(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Orion;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 12;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 12; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",6,6},{"R.Warp",6,6}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 3;
    s.sys.crew_total = 10; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 12;
    s.sys.shields_max[1] = s.sys.shields[1] = 10;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 8;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 10;
    s.sys.hull = s.sys.hull_max = 20;
    s.dac = dac_for_hull(20);
    s.weapons = {
        make_disruptor("Disruptor FA1"), make_disruptor("Disruptor FA2"),
        make_ph3("Ph-G LS"),
        make_ph3("Ph-G RS"),
        make_drone("Drone 1"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ WYN (Kzinti) Frigate (ZFF) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_wyn_zff(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Kzinti;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 8;
    s.sys.max_impulse_power = 2;
    s.eaf.warp_power = 8; s.eaf.impulse_power = 2;
    s.sys.warp_groups    = {{"L.Warp",4,4},{"R.Warp",4,4}};
    s.sys.impulse_groups = {{"Impulse",2,2}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 4; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 10;
    s.sys.shields_max[1] = s.sys.shields[1] = 8;
    s.sys.shields_max[2] = s.sys.shields[2] = 5;
    s.sys.shields_max[3] = s.sys.shields[3] = 5;
    s.sys.shields_max[4] = s.sys.shields[4] = 5;
    s.sys.shields_max[5] = s.sys.shields[5] = 8;
    s.sys.hull = s.sys.hull_max = 22;
    s.dac = dac_for_hull(20);
    s.weapons = {
        make_ph1("Ph-1 FA1"), make_ph1("Ph-1 FA2"),
        make_ph2("Ph-2 LS1"), make_ph2("Ph-2 LS2"),
        make_ph2("Ph-2 RS1"), make_ph2("Ph-2 RS2"),
        make_drone("Drone 1"), make_drone("Drone 2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Orion Strike Carrier (CVS) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_orion_cvs(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Orion;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 20;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 20; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",10,10},{"R.Warp",10,10}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 8; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 20;
    s.sys.shields_max[1] = s.sys.shields[1] = 16;
    s.sys.shields_max[2] = s.sys.shields[2] = 12;
    s.sys.shields_max[3] = s.sys.shields[3] = 12;
    s.sys.shields_max[4] = s.sys.shields[4] = 12;
    s.sys.shields_max[5] = s.sys.shields[5] = 16;
    s.sys.hull = s.sys.hull_max = 30;
    s.dac = dac_for_hull(30);
    s.sys.cloak_installed = true; s.sys.cloak_cost = 6;
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Battle PF Tender (BRP) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_orion_brp(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Orion;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 20;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 20; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",10,10},{"R.Warp",10,10}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 8; s.sys.shuttles = 0;
    s.sys.shields_max[0] = s.sys.shields[0] = 20;
    s.sys.shields_max[1] = s.sys.shields[1] = 16;
    s.sys.shields_max[2] = s.sys.shields[2] = 12;
    s.sys.shields_max[3] = s.sys.shields[3] = 12;
    s.sys.shields_max[4] = s.sys.shields[4] = 12;
    s.sys.shields_max[5] = s.sys.shields[5] = 16;
    s.sys.hull = s.sys.hull_max = 30;
    s.dac = dac_for_hull(30);
    s.sys.cloak_installed = true; s.sys.cloak_cost = 6;
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Heavy Raider (HR) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_orion_hr(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Orion;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 24;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 24; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",12,12},{"R.Warp",12,12}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 5;
    s.sys.crew_total = 10; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 22;
    s.sys.shields_max[1] = s.sys.shields[1] = 18;
    s.sys.shields_max[2] = s.sys.shields[2] = 14;
    s.sys.shields_max[3] = s.sys.shields[3] = 14;
    s.sys.shields_max[4] = s.sys.shields[4] = 14;
    s.sys.shields_max[5] = s.sys.shields[5] = 18;
    s.sys.hull = s.sys.hull_max = 23;
    s.dac = dac_for_hull(25);
    s.sys.cloak_installed = true; s.sys.cloak_cost = 16;
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Double Raider (DBR) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_orion_dbr(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Orion;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 20;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 20; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",10,10},{"R.Warp",10,10}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 10; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 14;
    s.sys.shields_max[2] = s.sys.shields[2] = 10;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 10;
    s.sys.shields_max[5] = s.sys.shields[5] = 14;
    s.sys.hull = s.sys.hull_max = 20;
    s.dac = dac_for_hull(20);
    s.sys.cloak_installed = true; s.sys.cloak_cost = 15;
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Orion Heavy Battlecruiser (BCH) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_orion_bch(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Orion;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 28;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 28; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",14,14},{"R.Warp",14,14}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 6;
    s.sys.crew_total = 10; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 24;
    s.sys.shields_max[1] = s.sys.shields[1] = 20;
    s.sys.shields_max[2] = s.sys.shields[2] = 16;
    s.sys.shields_max[3] = s.sys.shields[3] = 16;
    s.sys.shields_max[4] = s.sys.shields[4] = 16;
    s.sys.shields_max[5] = s.sys.shields[5] = 20;
    s.sys.hull = s.sys.hull_max = 40;
    s.dac = dac_for_hull(40);
    s.sys.cloak_installed = true; s.sys.cloak_cost = 27;
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_ph1("Ph-1 C"), make_ph1("Ph-1 D"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Orion War Destroyer (DW) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_orion_dw(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Orion;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 16;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 16; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",8,8},{"R.Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 0;
    s.sys.battery_cap = 3;
    s.sys.crew_total = 10; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 14;
    s.sys.shields_max[2] = s.sys.shields[2] = 10;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 10;
    s.sys.shields_max[5] = s.sys.shields[5] = 14;
    s.sys.hull = s.sys.hull_max = 24;
    s.dac = dac_for_hull(25);
    s.sys.cloak_installed = true; s.sys.cloak_cost = 12;
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Light Raider Scout (LRS) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_orion_lrs(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Orion;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 12;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 12; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",6,6},{"R.Warp",6,6}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 0;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 10; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 14;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 8;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 12;
    s.dac = dac_for_hull(10);
    s.sys.cloak_installed = true; s.sys.cloak_cost = 13;
    s.weapons = {
        make_ph3("Ph-3 A"), make_ph3("Ph-3 B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Attack Raider (AR) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_orion_ar(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Orion;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 20;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 20; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",10,10},{"R.Warp",10,10}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 10; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 14;
    s.sys.shields_max[2] = s.sys.shields[2] = 10;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 10;
    s.sys.shields_max[5] = s.sys.shields[5] = 14;
    s.sys.hull = s.sys.hull_max = 26;
    s.dac = dac_for_hull(25);
    s.sys.cloak_installed = true; s.sys.cloak_cost = 15;
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_ph1("Ph-1 C"), make_ph1("Ph-1 D"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Orion War Destroyer Scout (DWS) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_orion_dws(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Orion;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 16;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 16; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",8,8},{"R.Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 0;
    s.sys.battery_cap = 3;
    s.sys.crew_total = 10; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 14;
    s.sys.shields_max[2] = s.sys.shields[2] = 10;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 10;
    s.sys.shields_max[5] = s.sys.shields[5] = 14;
    s.sys.hull = s.sys.hull_max = 24;
    s.dac = dac_for_hull(25);
    s.sys.cloak_installed = true; s.sys.cloak_cost = 15;
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Double Raider PF Tender (DBP) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_orion_dbp(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Orion;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 20;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 20; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",10,10},{"R.Warp",10,10}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 10; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 14;
    s.sys.shields_max[2] = s.sys.shields[2] = 10;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 10;
    s.sys.shields_max[5] = s.sys.shields[5] = 14;
    s.sys.hull = s.sys.hull_max = 24;
    s.dac = dac_for_hull(25);
    s.sys.cloak_installed = true; s.sys.cloak_cost = 17;
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Medium Raider (MR) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_orion_mr(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Orion;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 16;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 16; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",8,8},{"R.Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 10; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 14;
    s.sys.shields_max[2] = s.sys.shields[2] = 10;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 10;
    s.sys.shields_max[5] = s.sys.shields[5] = 14;
    s.sys.hull = s.sys.hull_max = 20;
    s.dac = dac_for_hull(20);
    s.sys.cloak_installed = true; s.sys.cloak_cost = 17;
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ OK6 Cruiser (Captured Klingon D6) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_orion_ok6(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Orion;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 24;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 24; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",12,12},{"R.Warp",12,12}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 5;
    s.sys.crew_total = 10; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 22;
    s.sys.shields_max[1] = s.sys.shields[1] = 18;
    s.sys.shields_max[2] = s.sys.shields[2] = 14;
    s.sys.shields_max[3] = s.sys.shields[3] = 14;
    s.sys.shields_max[4] = s.sys.shields[4] = 14;
    s.sys.shields_max[5] = s.sys.shields[5] = 18;
    s.sys.hull = s.sys.hull_max = 42;
    s.dac = dac_for_hull(40);
    s.sys.cloak_installed = true; s.sys.cloak_cost = 20;
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_ph1("Ph-1 C"), make_ph1("Ph-1 D"),
        make_disruptor("Disruptor A"), make_disruptor("Disruptor B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Anaconda Heavy Scout (ANA) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_andro_ana(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 16;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 16; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",8,8},{"R.Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 0;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 10; s.sys.shuttles = 0;
    s.sys.shields_max[0] = s.sys.shields[0] = 14;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 8;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 16;
    s.dac = dac_for_hull(15);
    s.weapons = {
        make_drone("Displacement Device"),
        make_drone("Tractor A"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Asp Mauler Frigate (ASP) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_andro_asp(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 8;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 8; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",4,4},{"R.Warp",4,4}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 0;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 8; s.sys.shuttles = 0;
    s.sys.shields_max[0] = s.sys.shields[0] = 14;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 8;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 8;
    s.dac = dac_for_hull(10);
    s.weapons = {
        make_hellbore("Mauler"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ King Cobra satellite ship (KIN) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_andro_kin(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 8;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 8; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",4,4},{"R.Warp",4,4}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 0;
    s.sys.battery_cap = 2;
    s.sys.crew_total = 10; s.sys.shuttles = 0;
    s.sys.shields_max[0] = s.sys.shields[0] = 12;
    s.sys.shields_max[1] = s.sys.shields[1] = 10;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 8;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 10;
    s.sys.hull = s.sys.hull_max = 10;
    s.dac = dac_for_hull(10);
    s.weapons = {
        make_drone("Displacement Device"),
        make_drone("Tractor A"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Conquistador Tournament Cruiser (KRA) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_andro_kra(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 16;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 16; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",8,8},{"R.Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 6; s.sys.shuttles = 0;
    s.sys.shields_max[0] = s.sys.shields[0] = 20;
    s.sys.shields_max[1] = s.sys.shields[1] = 16;
    s.sys.shields_max[2] = s.sys.shields[2] = 12;
    s.sys.shields_max[3] = s.sys.shields[3] = 12;
    s.sys.shields_max[4] = s.sys.shields[4] = 12;
    s.sys.shields_max[5] = s.sys.shields[5] = 16;
    s.sys.hull = s.sys.hull_max = 25;
    s.dac = dac_for_hull(25);
    s.weapons = {
        make_drone("Displacement Device"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ AxCVL Small Auxiliary Carrier â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_gen_axcvl(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 12;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 12; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",6,6},{"R.Warp",6,6}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 6; s.sys.shuttles = 12;
    s.sys.shields_max[0] = s.sys.shields[0] = 20;
    s.sys.shields_max[1] = s.sys.shields[1] = 16;
    s.sys.shields_max[2] = s.sys.shields[2] = 12;
    s.sys.shields_max[3] = s.sys.shields[3] = 12;
    s.sys.shields_max[4] = s.sys.shields[4] = 12;
    s.sys.shields_max[5] = s.sys.shields[5] = 16;
    s.sys.hull = s.sys.hull_max = 30;
    s.dac = dac_for_hull(30);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_drone("Drone/ADD 1"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ AxCVA Large Auxiliary Carrier â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_gen_axcva(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Federation;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 16;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 16; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",8,8},{"R.Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 5;
    s.sys.crew_total = 8; s.sys.shuttles = 24;
    s.sys.shields_max[0] = s.sys.shields[0] = 24;
    s.sys.shields_max[1] = s.sys.shields[1] = 20;
    s.sys.shields_max[2] = s.sys.shields[2] = 16;
    s.sys.shields_max[3] = s.sys.shields[3] = 16;
    s.sys.shields_max[4] = s.sys.shields[4] = 16;
    s.sys.shields_max[5] = s.sys.shields[5] = 20;
    s.sys.hull = s.sys.hull_max = 40;
    s.dac = dac_for_hull(40);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_ph1("Ph-1 C"), make_ph1("Ph-1 D"),
        make_drone("Drone/ADD 1"), make_drone("Drone/ADD 2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Heavy Cruiser (CA) (kzin_ca) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_kzin_ca(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Kzinti;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 24;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 24; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",8,8},{"C.Warp",8,8},{"R.Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 6; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 22;
    s.sys.shields_max[1] = s.sys.shields[1] = 18;
    s.sys.shields_max[2] = s.sys.shields[2] = 14;
    s.sys.shields_max[3] = s.sys.shields[3] = 14;
    s.sys.shields_max[4] = s.sys.shields[4] = 14;
    s.sys.shields_max[5] = s.sys.shields[5] = 18;
    s.sys.hull = s.sys.hull_max = 40;
    s.dac = dac_for_hull(40);
    s.weapons = {
        make_disruptor("Disruptor A"), make_disruptor("Disruptor B"),
        make_disruptor("Disruptor C"), make_disruptor("Disruptor D"),
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_drone("Drone Rack A"), make_drone("Drone Rack B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ First Carrier (DDV) (kzin_ddv) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_kzin_ddv(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Kzinti;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 18;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 18; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",6,6},{"C.Warp",6,6},{"R.Warp",6,6}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 0;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 5; s.sys.shuttles = 12;
    s.sys.shields_max[0] = s.sys.shields[0] = 20;
    s.sys.shields_max[1] = s.sys.shields[1] = 16;
    s.sys.shields_max[2] = s.sys.shields[2] = 12;
    s.sys.shields_max[3] = s.sys.shields[3] = 12;
    s.sys.shields_max[4] = s.sys.shields[4] = 12;
    s.sys.shields_max[5] = s.sys.shields[5] = 16;
    s.sys.hull = s.sys.hull_max = 34;
    s.dac = dac_for_hull(35);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_drone("Drone Rack A"), make_drone("Drone Rack B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ War Destroyer Scout (DWS) (kzin_dws) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_kzin_dws(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Kzinti;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 18;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 18; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",6,6},{"C.Warp",6,6},{"R.Warp",6,6}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 0;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 4; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 14;
    s.sys.shields_max[2] = s.sys.shields[2] = 10;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 10;
    s.sys.shields_max[5] = s.sys.shields[5] = 14;
    s.sys.hull = s.sys.hull_max = 26;
    s.dac = dac_for_hull(25);
    s.weapons = {
        make_disruptor("Disruptor A"), make_disruptor("Disruptor B"),
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_drone("Drone Rack A"), make_drone("Drone Rack B"),
        make_drone("Drone Rack C"), make_drone("Drone Rack D"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ War Destroyer Aegis Escort (DWA) (kzin_dwa) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_kzin_dwa(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Kzinti;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 18;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 18; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",6,6},{"C.Warp",6,6},{"R.Warp",6,6}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 0;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 4; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 14;
    s.sys.shields_max[2] = s.sys.shields[2] = 10;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 10;
    s.sys.shields_max[5] = s.sys.shields[5] = 14;
    s.sys.hull = s.sys.hull_max = 28;
    s.dac = dac_for_hull(30);
    s.weapons = {
        make_disruptor("Disruptor A"), make_disruptor("Disruptor B"),
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_drone("Drone Rack G1"), make_drone("Drone Rack G2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Medium Escort Cruiser (MEC) (kzin_mec) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_kzin_mec(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Kzinti;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 18;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 18; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",6,6},{"C.Warp",6,6},{"R.Warp",6,6}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 5; s.sys.shuttles = 0;
    s.sys.shields_max[0] = s.sys.shields[0] = 20;
    s.sys.shields_max[1] = s.sys.shields[1] = 16;
    s.sys.shields_max[2] = s.sys.shields[2] = 12;
    s.sys.shields_max[3] = s.sys.shields[3] = 12;
    s.sys.shields_max[4] = s.sys.shields[4] = 12;
    s.sys.shields_max[5] = s.sys.shields[5] = 16;
    s.sys.hull = s.sys.hull_max = 30;
    s.dac = dac_for_hull(30);
    s.weapons = {
        make_disruptor("Disruptor A"), make_disruptor("Disruptor B"),
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_drone("Drone Rack G1"), make_drone("Drone Rack G2"),
        make_drone("Drone Rack G3"), make_drone("Drone Rack G4"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Heavy Carrier (CVA) (kzin_cva) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_kzin_cva(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Kzinti;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 30;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 30; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",10,10},{"C.Warp",10,10},{"R.Warp",10,10}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 6;
    s.sys.crew_total = 10; s.sys.shuttles = 6;
    s.sys.shields_max[0] = s.sys.shields[0] = 24;
    s.sys.shields_max[1] = s.sys.shields[1] = 20;
    s.sys.shields_max[2] = s.sys.shields[2] = 16;
    s.sys.shields_max[3] = s.sys.shields[3] = 16;
    s.sys.shields_max[4] = s.sys.shields[4] = 16;
    s.sys.shields_max[5] = s.sys.shields[5] = 20;
    s.sys.hull = s.sys.hull_max = 50;
    s.dac = dac_for_hull(50);
    s.weapons = {
        make_disruptor("Disruptor A"), make_disruptor("Disruptor B"),
        make_disruptor("Disruptor C"), make_disruptor("Disruptor D"),
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_drone("Drone Rack A1"), make_drone("Drone Rack A2"),
        make_drone("Drone Rack A3"), make_drone("Drone Rack A4"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Police Corvette (POL) (kzin_pol) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_kzin_pol(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Kzinti;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 12;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 12; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",4,4},{"C.Warp",4,4},{"R.Warp",4,4}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 0;
    s.sys.battery_cap = 3;
    s.sys.crew_total = 3; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 14;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 8;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 20;
    s.dac = dac_for_hull(20);
    s.weapons = {
        make_disruptor("Disruptor A"),
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_drone("Drone Rack A"), make_drone("Drone Rack B"), make_drone("Drone Rack C"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Survey Carrier (SRV) (kzin_srv) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_kzin_srv(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Kzinti;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 24;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 24; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",8,8},{"C.Warp",8,8},{"R.Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 9; s.sys.shuttles = 4;
    s.sys.shields_max[0] = s.sys.shields[0] = 22;
    s.sys.shields_max[1] = s.sys.shields[1] = 18;
    s.sys.shields_max[2] = s.sys.shields[2] = 14;
    s.sys.shields_max[3] = s.sys.shields[3] = 14;
    s.sys.shields_max[4] = s.sys.shields[4] = 14;
    s.sys.shields_max[5] = s.sys.shields[5] = 18;
    s.sys.hull = s.sys.hull_max = 40;
    s.dac = dac_for_hull(40);
    s.weapons = {
        make_disruptor("Disruptor A"), make_disruptor("Disruptor B"),
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_drone("Drone Rack A"), make_drone("Drone Rack B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ War Drone Destroyer (DWD) (kzin_dwd) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_kzin_dwd(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Kzinti;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 18;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 18; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",6,6},{"C.Warp",6,6},{"R.Warp",6,6}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 0;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 4; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 14;
    s.sys.shields_max[2] = s.sys.shields[2] = 10;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 10;
    s.sys.shields_max[5] = s.sys.shields[5] = 14;
    s.sys.hull = s.sys.hull_max = 28;
    s.dac = dac_for_hull(30);
    s.weapons = {
        make_disruptor("Disruptor A"), make_disruptor("Disruptor B"),
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_drone("Drone Rack B1"), make_drone("Drone Rack B2"),
        make_drone("Drone Rack B3"), make_drone("Drone Rack B4"),
        make_drone("Drone Rack C1"), make_drone("Drone Rack C2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Medium PF Tender (MPF) (kzin_mpf) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_kzin_mpf(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Kzinti;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 21;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 21; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",7,7},{"C.Warp",7,7},{"R.Warp",7,7}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 5; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 20;
    s.sys.shields_max[1] = s.sys.shields[1] = 16;
    s.sys.shields_max[2] = s.sys.shields[2] = 12;
    s.sys.shields_max[3] = s.sys.shields[3] = 12;
    s.sys.shields_max[4] = s.sys.shields[4] = 12;
    s.sys.shields_max[5] = s.sys.shields[5] = 16;
    s.sys.hull = s.sys.hull_max = 34;
    s.dac = dac_for_hull(35);
    s.weapons = {
        make_disruptor("Disruptor A"), make_disruptor("Disruptor B"),
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Destroyer (DD) (kzin_dd) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_kzin_dd(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Kzinti;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 18;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 18; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",6,6},{"C.Warp",6,6},{"R.Warp",6,6}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 0;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 4; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 14;
    s.sys.shields_max[2] = s.sys.shields[2] = 10;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 10;
    s.sys.shields_max[5] = s.sys.shields[5] = 14;
    s.sys.hull = s.sys.hull_max = 30;
    s.dac = dac_for_hull(30);
    s.weapons = {
        make_disruptor("Disruptor A"), make_disruptor("Disruptor B"),
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_drone("Drone Rack B1"), make_drone("Drone Rack B2"),
        make_drone("Drone Rack C1"), make_drone("Drone Rack C2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Scout Drone Frigate (SDF) (kzin_sdf) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_kzin_sdf(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Kzinti;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 12;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 12; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",4,4},{"C.Warp",4,4},{"R.Warp",4,4}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 0;
    s.sys.battery_cap = 3;
    s.sys.crew_total = 3; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 14;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 8;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 20;
    s.dac = dac_for_hull(20);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_drone("Drone Rack A"), make_drone("Drone Rack B"),
        make_drone("Drone Rack C"), make_drone("Drone Rack D"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Drone Cruiser (CD) (kzin_cd) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_kzin_cd(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Kzinti;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 24;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 24; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",8,8},{"C.Warp",8,8},{"R.Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 6; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 22;
    s.sys.shields_max[1] = s.sys.shields[1] = 18;
    s.sys.shields_max[2] = s.sys.shields[2] = 14;
    s.sys.shields_max[3] = s.sys.shields[3] = 14;
    s.sys.shields_max[4] = s.sys.shields[4] = 14;
    s.sys.shields_max[5] = s.sys.shields[5] = 18;
    s.sys.hull = s.sys.hull_max = 40;
    s.dac = dac_for_hull(40);
    s.weapons = {
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_drone("Drone Rack A1"), make_drone("Drone Rack A2"),
        make_drone("Drone Rack A3"), make_drone("Drone Rack A4"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Survey Cruiser (SR) (kzin_sr) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_kzin_sr(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Kzinti;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 24;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 24; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",8,8},{"C.Warp",8,8},{"R.Warp",8,8}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 6; s.sys.shuttles = 4;
    s.sys.shields_max[0] = s.sys.shields[0] = 22;
    s.sys.shields_max[1] = s.sys.shields[1] = 18;
    s.sys.shields_max[2] = s.sys.shields[2] = 14;
    s.sys.shields_max[3] = s.sys.shields[3] = 14;
    s.sys.shields_max[4] = s.sys.shields[4] = 14;
    s.sys.shields_max[5] = s.sys.shields[5] = 18;
    s.sys.hull = s.sys.hull_max = 40;
    s.dac = dac_for_hull(40);
    s.weapons = {
        make_disruptor("Disruptor A"), make_disruptor("Disruptor B"),
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_drone("Drone Rack A"), make_drone("Drone Rack B"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Medium Drone Cruiser (MDC) (kzin_mdc) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_kzin_mdc(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Kzinti;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 18;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 18; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",6,6},{"C.Warp",6,6},{"R.Warp",6,6}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 5; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 20;
    s.sys.shields_max[1] = s.sys.shields[1] = 16;
    s.sys.shields_max[2] = s.sys.shields[2] = 12;
    s.sys.shields_max[3] = s.sys.shields[3] = 12;
    s.sys.shields_max[4] = s.sys.shields[4] = 12;
    s.sys.shields_max[5] = s.sys.shields[5] = 16;
    s.sys.hull = s.sys.hull_max = 32;
    s.dac = dac_for_hull(30);
    s.weapons = {
        make_disruptor("Disruptor A"), make_disruptor("Disruptor B"),
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_drone("Drone Rack A"), make_drone("Drone Rack B"),
        make_drone("Drone Rack C"), make_drone("Drone Rack D"),
        make_drone("Drone Rack E"), make_drone("Drone Rack F"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Medium Command Cruiser (MCC) (kzin_mcc) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_kzin_mcc(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Kzinti;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 21;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 21; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",7,7},{"C.Warp",7,7},{"R.Warp",7,7}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 5; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 20;
    s.sys.shields_max[1] = s.sys.shields[1] = 16;
    s.sys.shields_max[2] = s.sys.shields[2] = 12;
    s.sys.shields_max[3] = s.sys.shields[3] = 12;
    s.sys.shields_max[4] = s.sys.shields[4] = 12;
    s.sys.shields_max[5] = s.sys.shields[5] = 16;
    s.sys.hull = s.sys.hull_max = 36;
    s.dac = dac_for_hull(35);
    s.weapons = {
        make_disruptor("Disruptor A"), make_disruptor("Disruptor B"),
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_drone("Drone Rack B1"), make_drone("Drone Rack B2"),
        make_drone("Drone Rack C1"), make_drone("Drone Rack C2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ War Destroyer (DW) (kzin_dw) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_kzin_dw(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Kzinti;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 18;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 18; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",6,6},{"C.Warp",6,6},{"R.Warp",6,6}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 0;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 4; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 14;
    s.sys.shields_max[2] = s.sys.shields[2] = 10;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 10;
    s.sys.shields_max[5] = s.sys.shields[5] = 14;
    s.sys.hull = s.sys.hull_max = 28;
    s.dac = dac_for_hull(30);
    s.weapons = {
        make_disruptor("Disruptor A"), make_disruptor("Disruptor B"),
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_drone("Drone Rack A"), make_drone("Drone Rack B"),
        make_drone("Drone Rack C"), make_drone("Drone Rack D"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Medium Carrier (MCV) (kzin_mcv) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_kzin_mcv(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Kzinti;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 21;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 21; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",7,7},{"C.Warp",7,7},{"R.Warp",7,7}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 7; s.sys.shuttles = 3;
    s.sys.shields_max[0] = s.sys.shields[0] = 20;
    s.sys.shields_max[1] = s.sys.shields[1] = 16;
    s.sys.shields_max[2] = s.sys.shields[2] = 12;
    s.sys.shields_max[3] = s.sys.shields[3] = 12;
    s.sys.shields_max[4] = s.sys.shields[4] = 12;
    s.sys.shields_max[5] = s.sys.shields[5] = 16;
    s.sys.hull = s.sys.hull_max = 34;
    s.dac = dac_for_hull(35);
    s.weapons = {
        make_disruptor("Disruptor A"), make_disruptor("Disruptor B"),
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_drone("Drone Rack B1"), make_drone("Drone Rack B2"),
        make_drone("Drone Rack C1"), make_drone("Drone Rack C2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ War Destroyer Escort (DWE) (kzin_dwe) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_kzin_dwe(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Kzinti;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 18;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 18; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",6,6},{"C.Warp",6,6},{"R.Warp",6,6}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 0;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 4; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 14;
    s.sys.shields_max[2] = s.sys.shields[2] = 10;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 10;
    s.sys.shields_max[5] = s.sys.shields[5] = 14;
    s.sys.hull = s.sys.hull_max = 28;
    s.dac = dac_for_hull(30);
    s.weapons = {
        make_disruptor("Disruptor A"), make_disruptor("Disruptor B"),
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_drone("Drone Rack G1"), make_drone("Drone Rack G2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Medium Aegis Cruiser (MAC) (kzin_mac) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_kzin_mac(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Kzinti;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 18;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 18; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",6,6},{"C.Warp",6,6},{"R.Warp",6,6}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 5; s.sys.shuttles = 0;
    s.sys.shields_max[0] = s.sys.shields[0] = 20;
    s.sys.shields_max[1] = s.sys.shields[1] = 16;
    s.sys.shields_max[2] = s.sys.shields[2] = 12;
    s.sys.shields_max[3] = s.sys.shields[3] = 12;
    s.sys.shields_max[4] = s.sys.shields[4] = 12;
    s.sys.shields_max[5] = s.sys.shields[5] = 16;
    s.sys.hull = s.sys.hull_max = 30;
    s.dac = dac_for_hull(30);
    s.weapons = {
        make_disruptor("Disruptor A"), make_disruptor("Disruptor B"),
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_drone("Drone Rack G1"), make_drone("Drone Rack G2"),
        make_drone("Drone Rack G3"), make_drone("Drone Rack G4"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Dreadnought (DN) (kzin_dn) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_kzin_dn(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Kzinti;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 30;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 30; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",10,10},{"C.Warp",10,10},{"R.Warp",10,10}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 6;
    s.sys.crew_total = 9; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 30;
    s.sys.shields_max[1] = s.sys.shields[1] = 28;
    s.sys.shields_max[2] = s.sys.shields[2] = 22;
    s.sys.shields_max[3] = s.sys.shields[3] = 22;
    s.sys.shields_max[4] = s.sys.shields[4] = 22;
    s.sys.shields_max[5] = s.sys.shields[5] = 28;
    s.sys.hull = s.sys.hull_max = 50;
    s.dac = dac_for_hull(50);
    s.weapons = {
        make_disruptor("Disruptor A"), make_disruptor("Disruptor B"),
        make_disruptor("Disruptor C"), make_disruptor("Disruptor D"),
        make_disruptor("Disruptor E"), make_disruptor("Disruptor F"),
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_ph1("Ph-1 C"), make_ph1("Ph-1 D"),
        make_drone("Drone Rack B1"), make_drone("Drone Rack B2"),
        make_drone("Drone Rack B3"), make_drone("Drone Rack B4"),
        make_drone("Drone Rack C1"), make_drone("Drone Rack C2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Medium Minesweeper (MMS) (kzin_mms) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_kzin_mms(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Kzinti;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 18;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 18; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",6,6},{"C.Warp",6,6},{"R.Warp",6,6}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 5; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 20;
    s.sys.shields_max[1] = s.sys.shields[1] = 16;
    s.sys.shields_max[2] = s.sys.shields[2] = 12;
    s.sys.shields_max[3] = s.sys.shields[3] = 12;
    s.sys.shields_max[4] = s.sys.shields[4] = 12;
    s.sys.shields_max[5] = s.sys.shields[5] = 16;
    s.sys.hull = s.sys.hull_max = 30;
    s.dac = dac_for_hull(30);
    s.weapons = {
        make_disruptor("Disruptor A"), make_disruptor("Disruptor B"),
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_drone("Drone Rack A"), make_drone("Drone Rack B"),
        make_drone("Drone Rack C"), make_drone("Drone Rack D"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ FFK Frigate (C-9 Refit) (kzin_ffk) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_kzin_ffk(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Kzinti;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 12;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 12; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",4,4},{"C.Warp",4,4},{"R.Warp",4,4}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 0;
    s.sys.battery_cap = 3;
    s.sys.crew_total = 3; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 14;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 8;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 20;
    s.dac = dac_for_hull(20);
    s.weapons = {
        make_disruptor("Disruptor A"), make_disruptor("Disruptor B"),
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_drone("Drone Rack A"), make_drone("Drone Rack B"),
        make_drone("Drone Rack C"), make_drone("Drone Rack D"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Heavy Battlecruiser (BCH) (kzin_bch) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_kzin_bch(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Kzinti;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 33;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 33; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",11,11},{"C.Warp",11,11},{"R.Warp",11,11}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 6;
    s.sys.crew_total = 7; s.sys.shuttles = 2;
    s.sys.shields_max[0] = s.sys.shields[0] = 24;
    s.sys.shields_max[1] = s.sys.shields[1] = 20;
    s.sys.shields_max[2] = s.sys.shields[2] = 18;
    s.sys.shields_max[3] = s.sys.shields[3] = 18;
    s.sys.shields_max[4] = s.sys.shields[4] = 18;
    s.sys.shields_max[5] = s.sys.shields[5] = 20;
    s.sys.hull = s.sys.hull_max = 54;
    s.dac = dac_for_hull(55);
    s.weapons = {
        make_disruptor("Disruptor A"), make_disruptor("Disruptor B"),
        make_disruptor("Disruptor C"), make_disruptor("Disruptor D"),
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_ph1("Ph-1 C"), make_ph1("Ph-1 D"),
        make_drone("Drone Rack B1"), make_drone("Drone Rack B2"),
        make_drone("Drone Rack C1"), make_drone("Drone Rack C2"),
        make_drone("Drone Rack G1"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Heavy Frigate (FH) (kzin_fh) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_kzin_fh(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Kzinti;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 15;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 15; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",5,5},{"C.Warp",5,5},{"R.Warp",5,5}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 0;
    s.sys.battery_cap = 3;
    s.sys.crew_total = 3; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 14;
    s.sys.shields_max[1] = s.sys.shields[1] = 12;
    s.sys.shields_max[2] = s.sys.shields[2] = 8;
    s.sys.shields_max[3] = s.sys.shields[3] = 8;
    s.sys.shields_max[4] = s.sys.shields[4] = 8;
    s.sys.shields_max[5] = s.sys.shields[5] = 12;
    s.sys.hull = s.sys.hull_max = 24;
    s.dac = dac_for_hull(25);
    s.weapons = {
        make_disruptor("Disruptor A"), make_disruptor("Disruptor B"),
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_drone("Drone Rack A"), make_drone("Drone Rack B"),
        make_drone("Drone Rack C"), make_drone("Drone Rack D"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Super Space Control Ship (SSCS) (kzin_sscs) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_kzin_sscs(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Kzinti;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 30;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 30; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",10,10},{"C.Warp",10,10},{"R.Warp",10,10}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 6;
    s.sys.crew_total = 10; s.sys.shuttles = 12;
    s.sys.shields_max[0] = s.sys.shields[0] = 30;
    s.sys.shields_max[1] = s.sys.shields[1] = 28;
    s.sys.shields_max[2] = s.sys.shields[2] = 22;
    s.sys.shields_max[3] = s.sys.shields[3] = 22;
    s.sys.shields_max[4] = s.sys.shields[4] = 22;
    s.sys.shields_max[5] = s.sys.shields[5] = 28;
    s.sys.hull = s.sys.hull_max = 55;
    s.dac = dac_for_hull(55);
    s.weapons = {
        make_disruptor("Disruptor A"), make_disruptor("Disruptor B"),
        make_disruptor("Disruptor C"), make_disruptor("Disruptor D"),
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_ph1("Ph-1 C"), make_ph1("Ph-1 D"),
        make_drone("Drone Rack B1"), make_drone("Drone Rack B2"),
        make_drone("Drone Rack B3"), make_drone("Drone Rack B4"),
        make_drone("Drone Rack C1"), make_drone("Drone Rack C2"),
        make_drone("Drone Rack G1"), make_drone("Drone Rack G2"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Medium Scout Cruiser (MSC) (kzin_msc) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_kzin_msc(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Kzinti;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 18;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 18; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",6,6},{"C.Warp",6,6},{"R.Warp",6,6}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 5; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 20;
    s.sys.shields_max[1] = s.sys.shields[1] = 16;
    s.sys.shields_max[2] = s.sys.shields[2] = 12;
    s.sys.shields_max[3] = s.sys.shields[3] = 12;
    s.sys.shields_max[4] = s.sys.shields[4] = 12;
    s.sys.shields_max[5] = s.sys.shields[5] = 16;
    s.sys.hull = s.sys.hull_max = 32;
    s.dac = dac_for_hull(30);
    s.weapons = {
        make_disruptor("Disruptor A"), make_disruptor("Disruptor B"),
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_drone("Drone Rack A"), make_drone("Drone Rack B"),
        make_drone("Drone Rack C"), make_drone("Drone Rack D"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ War Destroyer Leader (DWL) (kzin_dwl) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_kzin_dwl(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Kzinti;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 18;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 18; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",6,6},{"C.Warp",6,6},{"R.Warp",6,6}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 0;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 4; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 18;
    s.sys.shields_max[1] = s.sys.shields[1] = 14;
    s.sys.shields_max[2] = s.sys.shields[2] = 10;
    s.sys.shields_max[3] = s.sys.shields[3] = 10;
    s.sys.shields_max[4] = s.sys.shields[4] = 10;
    s.sys.shields_max[5] = s.sys.shields[5] = 14;
    s.sys.hull = s.sys.hull_max = 32;
    s.dac = dac_for_hull(30);
    s.weapons = {
        make_disruptor("Disruptor A"), make_disruptor("Disruptor B"),
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_ph1("Ph-1 C"), make_ph1("Ph-1 D"),
        make_drone("Drone Rack A"), make_drone("Drone Rack B"),
        make_drone("Drone Rack C"), make_drone("Drone Rack D"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}

// â”€â”€ Medium Tactical Transport (MTT) (kzin_mtt) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
Ship make_kzin_mtt(std::string name, Hex pos, int facing) {
    Ship s;
    s.name = std::move(name); s.faction = Faction::Kzinti;
    s.position = pos; s.facing = facing;
    s.sys.max_warp_power    = 18;
    s.sys.max_impulse_power = 4;
    s.eaf.warp_power = 18; s.eaf.impulse_power = 4;
    s.sys.warp_groups    = {{"L.Warp",6,6},{"C.Warp",6,6},{"R.Warp",6,6}};
    s.sys.impulse_groups = {{"Impulse",4,4}};
    s.sys.apr_rated = s.sys.apr_current = 2;
    s.sys.battery_cap = 4;
    s.sys.crew_total = 5; s.sys.shuttles = 1;
    s.sys.shields_max[0] = s.sys.shields[0] = 20;
    s.sys.shields_max[1] = s.sys.shields[1] = 16;
    s.sys.shields_max[2] = s.sys.shields[2] = 12;
    s.sys.shields_max[3] = s.sys.shields[3] = 12;
    s.sys.shields_max[4] = s.sys.shields[4] = 12;
    s.sys.shields_max[5] = s.sys.shields[5] = 16;
    s.sys.hull = s.sys.hull_max = 30;
    s.dac = dac_for_hull(30);
    s.weapons = {
        make_disruptor("Disruptor A"), make_disruptor("Disruptor B"),
        make_ph1("Ph-1 A"), make_ph1("Ph-1 B"),
        make_drone("Drone Rack A"), make_drone("Drone Rack B"),
        make_drone("Drone Rack C"), make_drone("Drone Rack D"),
    };    s.assign_weapon_arcs();
    s.turn_mode_cat = 4;  // C3.31

    return s;
}



