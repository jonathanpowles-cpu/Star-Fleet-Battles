#pragma once
#include "hex.h"
#include <unordered_map>
#include <functional>

struct HexHash {
    size_t operator()(const Hex& h) const {
        size_t seed = 0;
        auto combine = [&](int v) {
            seed ^= std::hash<int>{}(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        };
        combine(h.q); combine(h.r);
        return seed;
    }
};

enum class Terrain {
    Space, Nebula, Asteroid, Planet,
    BlackHole,    // P8: gravitational pull; ships within 3 hexes drawn in
    DustCloud,    // P9: sensor range halved; phaser range +5 penalty
    IonStorm,     // P10: drains 1 battery/impulse; 1 hull dmg if depleted
    GravityRift,  // P11: pushes ships one hex per impulse in rift_dir
    TholianWeb,   // P12: impassable barrier hex
};

struct Cell {
    Terrain terrain  = Terrain::Space;
    int     rift_dir = 0;  // GravityRift: push direction (0-5)
};

class Board {
public:
    Board(int radius);

    bool contains(Hex h) const;
    Cell& at(Hex h);
    const Cell& at(Hex h) const;

    void for_each(std::function<void(Hex, const Cell&)> fn) const;
    void for_each_mut(std::function<void(Hex, Cell&)> fn);

private:
    int radius_;
    std::unordered_map<Hex, Cell, HexHash> cells_;
};
