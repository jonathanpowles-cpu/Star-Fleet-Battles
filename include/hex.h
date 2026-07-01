#pragma once
#include <compare>
#include <cstdlib>

// Cube coordinates for a pointy-top hex grid.
// See: https://www.redblobgames.com/grids/hexagons/
struct Hex {
    int q, r, s;

    constexpr Hex() : q(0), r(0), s(0) {}
    constexpr Hex(int q, int r) : q(q), r(r), s(-q - r) {}
    constexpr Hex(int q, int r, int s) : q(q), r(r), s(s) {}

    auto operator<=>(const Hex&) const = default;

    Hex operator+(const Hex& o) const { return {q + o.q, r + o.r, s + o.s}; }
    Hex operator-(const Hex& o) const { return {q - o.q, r - o.r, s - o.s}; }

    int length() const { return (abs(q) + abs(r) + abs(s)) / 2; }
    int distance(const Hex& o) const { return (*this - o).length(); }
};

// Six neighbours in clockwise order starting from right (direction 0).
inline constexpr Hex HEX_DIRS[6] = {
    {1, 0}, {1, -1}, {0, -1}, {-1, 0}, {-1, 1}, {0, 1}
};

inline Hex hex_neighbour(Hex h, int dir) { return h + HEX_DIRS[dir % 6]; }
