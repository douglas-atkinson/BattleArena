#include "monster.h"
#include "dummymonster.h"
#include "position.h"
#include "worldstate.h"
#include "random.h"

DummyMonster::DummyMonster(const std::string& name, int hitPoints, int strength, int accuracy, int agility, 
    int defense, const Position& position, const std::array<std::string, 6>& portrait)
    : Monster(name, hitPoints, strength, accuracy, agility, defense, position, portrait) {}

AttackResult DummyMonster::attack(Combatant& target) {
    return performBasicAttack(target);
}

bool DummyMonster::move(const WorldState& world) {
    Position heroPos = world.heroPosition;
    Position newPos = position;
    int moveDecision = Random::getUniformInt(0, 2);
    if (moveDecision == 0) {
        // Move closer to player by row
        if (newPos.row < heroPos.row) {
            newPos.row++;
        }
        else if (newPos.row > heroPos.row) {
            newPos.row--;
        }
    }
    else if (moveDecision == 1) {
        // Move closer to player by col
        if (newPos.col < heroPos.col) {
            newPos.col++;
        }
        else if (newPos.col > heroPos.col) {
            newPos.col--;
        }
    }
    else {
        return true;  // stays in place
    }
    if (tryMoveTo(newPos, world)) {
        return true;
    }

    return false;
}
