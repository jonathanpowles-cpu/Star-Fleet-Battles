#include "board.h"
#include <stdexcept>

Board::Board(int radius) : radius_(radius) {
    for (int q = -radius; q <= radius; ++q)
        for (int r = std::max(-radius, -q - radius); r <= std::min(radius, -q + radius); ++r)
            cells_[{q, r}] = Cell{};
}

bool Board::contains(Hex h) const { return cells_.contains(h); }

Cell& Board::at(Hex h) {
    auto it = cells_.find(h);
    if (it == cells_.end()) throw std::out_of_range("hex not on board");
    return it->second;
}

const Cell& Board::at(Hex h) const {
    auto it = cells_.find(h);
    if (it == cells_.end()) throw std::out_of_range("hex not on board");
    return it->second;
}

void Board::for_each(std::function<void(Hex, const Cell&)> fn) const {
    for (auto& [h, c] : cells_) fn(h, c);
}
