#pragma once
#include "hex.h"
#include "weapons.h"
#include <string>
#include <vector>

enum class Faction { Federation, Klingon, Romulan };

struct ShipSystems {
    int shields[6]     = {30,30,30,30,30,30};
    int shields_max[6] = {30,30,30,30,30,30};
    int hull           = 100;
    int hull_max       = 100;
    int total_power    = 28;  // power available per turn
};

struct EnergyAllocation {
    int speed       = 0;     // hexes per turn (1 power each)
    int reinforce[6]= {};    // extra power added to shields each impulse
    int ecm         = 0;
    int eccm        = 0;
};

struct Ship {
    std::string name;
    Faction     faction;
    Hex         position;
    int         facing        = 0;    // 0–5, clockwise from right

    ShipSystems         sys;
    EnergyAllocation    eaf;
    std::vector<Weapon> weapons;
    bool                eaf_committed = false;  // committed this turn?

    // Power budget
    int power_used() const {
        int u = eaf.speed + eaf.ecm + eaf.eccm;
        for (int i = 0; i < 6; ++i) u += eaf.reinforce[i];
        for (auto& w : weapons) u += w.eaf_cost();
        return u;
    }
    int power_remaining() const { return sys.total_power - power_used(); }

    // Apply damage to the shield facing a given absolute direction
    void apply_damage(int dmg, int hit_facing) {
        int shield = hit_facing % 6;
        int absorbed = std::min(dmg, sys.shields[shield]);
        sys.shields[shield] -= absorbed;
        dmg -= absorbed;
        if (dmg > 0) sys.hull = std::max(0, sys.hull - dmg);
    }
};

// Factory functions — create ships with faction-appropriate loadouts
Ship make_federation_ca(std::string name, Hex pos, int facing);
Ship make_klingon_d7(std::string name, Hex pos, int facing);
