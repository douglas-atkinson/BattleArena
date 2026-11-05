#include <algorithm>
#include <array>
#include <cstdlib>
#include "combatant.h"
#include "monster.h"
#include "position.h"
#include "random.h"
#include "worldstate.h"

namespace {
    // The four squares a combatant can reach in one orthogonal step.
    std::array<Position, 4> neighborsOf(const Position& pos) {
        return { Position{ pos.row - 1, pos.col }, Position{ pos.row, pos.col - 1 },
                 Position{ pos.row + 1, pos.col }, Position{ pos.row, pos.col + 1 } };
    }

    bool isOpen(const Position& pos, const WorldState& world) {
        return world.isInBounds(pos) && !world.isOccupied(pos);
    }
}

Monster::Monster(const std::string& name, int hitPoints, int strength, int accuracy, int agility,
    int defense, int speed, const Position& position, const std::array<std::string, 6>& portrait)
    : Combatant(name, hitPoints, strength, accuracy, agility, defense, speed, position, portrait) {}

std::string Monster::speak() const {
    return "Grrrr";
}

// Picks the open square closest to the hero. If the direct routes are blocked,
// the closest open square may be farther away, which lets the monster sidestep.
bool Monster::stepTowardHero(const WorldState& world) {
    const Position hero = world.heroPosition;

    if (areOrthogonallyAdjacent(position, hero)) {
        return false;  // already in attack range
    }

    bool found = false;
    Position best;
    int bestDistance = 0;
    int bestLargerGap = 0;

    for (const Position& square : neighborsOf(position)) {
        if (!isOpen(square, world)) {
            continue;
        }

        int distance = manhattanDistance(square, hero);
        int largerGap = std::max(std::abs(square.row - hero.row), std::abs(square.col - hero.col));

        // On a tie, close the larger gap so the monster keeps moving around a blocker
        // instead of stepping back to the square it just left.
        if (!found || distance < bestDistance ||
            (distance == bestDistance && largerGap < bestLargerGap)) {
            found = true;
            best = square;
            bestDistance = distance;
            bestLargerGap = largerGap;
        }
    }

    return found && tryMoveTo(best, world);
}

// Picks the open square that increases the distance from the hero the most.
// A cornered monster holds its position rather than step toward the hero.
bool Monster::stepAwayFromHero(const WorldState& world) {
    const Position hero = world.heroPosition;

    bool found = false;
    Position best;
    int bestDistance = manhattanDistance(position, hero);

    for (const Position& square : neighborsOf(position)) {
        if (!isOpen(square, world)) {
            continue;
        }

        int distance = manhattanDistance(square, hero);
        if (distance > bestDistance) {
            found = true;
            best = square;
            bestDistance = distance;
        }
    }

    return found && tryMoveTo(best, world);
}
