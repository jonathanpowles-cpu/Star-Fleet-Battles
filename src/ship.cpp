#include "ship.h"

Ship make_federation_ca(std::string name, Hex pos, int facing) {
    Ship s;
    s.name    = std::move(name);
    s.faction = Faction::Federation;
    s.position= pos;
    s.facing  = facing;

    // CA stats
    s.sys.shields_max[0] = s.sys.shields[0] = 30; // fwd
    s.sys.shields_max[1] = s.sys.shields[1] = 25;
    s.sys.shields_max[2] = s.sys.shields[2] = 20;
    s.sys.shields_max[3] = s.sys.shields[3] = 20; // aft
    s.sys.shields_max[4] = s.sys.shields[4] = 20;
    s.sys.shields_max[5] = s.sys.shields[5] = 25;
    s.sys.hull = s.sys.hull_max = 100;
    s.sys.total_power = 28;

    // Weapons: 2× Ph-1 (fwd/side), 2× Ph-2 (rear/side), 2× Photon torp (fwd)
    s.weapons = {
        make_ph1("Ph-1 (P)"),   // port forward
        make_ph1("Ph-1 (S)"),   // starboard forward
        make_ph2("Ph-2 (P)"),   // port aft
        make_ph2("Ph-2 (S)"),   // starboard aft
        make_photon("Phot (A)"),
        make_photon("Phot (B)"),
    };
    return s;
}

Ship make_klingon_d7(std::string name, Hex pos, int facing) {
    Ship s;
    s.name    = std::move(name);
    s.faction = Faction::Klingon;
    s.position= pos;
    s.facing  = facing;

    // D7 stats
    s.sys.shields_max[0] = s.sys.shields[0] = 28;
    s.sys.shields_max[1] = s.sys.shields[1] = 22;
    s.sys.shields_max[2] = s.sys.shields[2] = 18;
    s.sys.shields_max[3] = s.sys.shields[3] = 18;
    s.sys.shields_max[4] = s.sys.shields[4] = 18;
    s.sys.shields_max[5] = s.sys.shields[5] = 22;
    s.sys.hull = s.sys.hull_max = 90;
    s.sys.total_power = 24;

    // Weapons: 2× Ph-2, 4× Disruptor
    s.weapons = {
        make_ph2("Ph-2 (P)"),
        make_ph2("Ph-2 (S)"),
        make_disruptor("Dis (PA)"),
        make_disruptor("Dis (SA)"),
        make_disruptor("Dis (PF)"),
        make_disruptor("Dis (SF)"),
    };
    return s;
}
