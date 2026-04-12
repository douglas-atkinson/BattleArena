#include "monster.h"
#include "dummymonster.h"
#include "position.h"
#include "worldstate.h"
#include "random.h"

DummyMonster::DummyMonster(const std::string& name, int hitPoints, int strength, int accuracy, int agility, 
    int defense, const Position& position, const std::array<std::string, 6>& portrait, char symbol)
    : Monster(name, hitPoints, strength, accuracy, agility, defense, position, portrait, symbol) {}

AttackResult DummyMonster::attack(Combatant& target) {
    AttackResult result;
    if (attackHits(target)) {
        result.hit = true;
        result.damage = calculateDamage(target);
        target.takeDamage(result.damage);
        if (target.getHitPoints() <= 0) {
            result.targetDefeated = true;
        }
    }
    return result;
}

bool DummyMonster::move(const WorldState& world) {
    Position heroPos = world.heroPosition;
    Position newPos = position;
    if (Random::getUniformInt(0, 100) < 50) {
        // Move closer to player by row
        if (newPos.row < heroPos.row) {
            newPos.row++;
        }
        else if (newPos.row > heroPos.row) {
            newPos.row--;
        }
    }
    else {
        // Move closer to player by col
        if (newPos.col < heroPos.col) {
            newPos.col++;
        }
        else if (newPos.col > heroPos.col) {
            newPos.col--;
        }
    }
    if (tryMoveTo(newPos, world)) {
        position = newPos;
        return true;
    }

    return false;
}
