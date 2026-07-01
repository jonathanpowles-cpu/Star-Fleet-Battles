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

enum class Terrain { Space, Nebula, Asteroid };

struct Cell {
    Terrain terrain = Terrain::Space;
};

class Board {
public:
    Board(int radius);

    bool contains(Hex h) const;
    Cell& at(Hex h);
    const Cell& at(Hex h) const;

    void for_each(std::function<void(Hex, const Cell&)> fn) const;

private:
    int radius_;
    std::unordered_map<Hex, Cell, HexHash> cells_;
};
