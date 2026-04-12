#pragma once

#include <vector>
#include "position.h"

struct WorldState {
    int rows{ 0 };
    int cols{ 0 };
    Position heroPosition{};
    std::vector<Position> monsterPositions;

    bool isInBounds(const Position& pos) const;
    bool isOccupied(const Position& pos) const;
};
