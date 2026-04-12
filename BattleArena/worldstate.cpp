#include "worldstate.h"

bool WorldState::isInBounds(const Position& pos) const {
    return pos.row >= 0 && pos.row < rows &&
        pos.col >= 0 && pos.col < cols;
}

bool WorldState::isOccupied(const Position& pos) const {
    if (pos == heroPosition) return true;

    for (const Position& monsterPos : monsterPositions) {
        if (pos == monsterPos) return true;
    }
    return false;
}