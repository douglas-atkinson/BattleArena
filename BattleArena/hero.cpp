#include "combatant.h"
#include "hero.h"
#include "attackresult.h"
#include "herotitle.h"
#include "position.h"
#include "movementresult.h"
#include <cctype>

Hero::Hero(const std::string& name, HeroTitle title, int hitPoints, int strength, int accuracy, 
           int agility, int defense, const Position& position, 
           const std::array<std::string, 6>& portrait)
    : Combatant(name, hitPoints, strength, accuracy, agility, defense, position, portrait),
    title(title), level(1), victories(0) {}

MovementResult Hero::moveByPlayerCommand(char command, const WorldState& world) {
    Position newPos = position;
    command = static_cast<char>(std::tolower(static_cast<unsigned char>(command)));
    switch (command) {
    case 'w': newPos.row -= 1; break;   // north
    case 'a': newPos.col -= 1; break;   // west
    case 's': newPos.row += 1; break;   // south
    case 'd': newPos.col += 1; break;   // east
    case 'r': return MovementResult::Success;   // explicit rest
    default:
        return MovementResult::InvalidCommand;
    }

    if (tryMoveTo(newPos, world)) {
        return MovementResult::Success;
    }
    return MovementResult::Blocked;
}

AttackResult Hero::attack(Combatant& target) {
    return performBasicAttack(target);
}

std::string Hero::speak() const {
    std::string words = getTitleString() + " " + name + ", onward to victory!";
    return words;
}

HeroTitle Hero::getTitle() const {
    return title;
}

std::string Hero::getTitleString() const {
    return heroTitleToString(title);
}

int Hero::getLevel() const {
    return level;
}

int Hero::getVictories() const {
    return victories;
}

void Hero::recordVictory() {
    ++victories;
    if (victories % 3 == 0) {
        ++level;
        if (level % 2 == 0) {
            // This is intentional
            // maxHitPoints will increase about 20% give or take a truncated fraction
            // hitPoints is not updated until a new Battle Arena round starts
            maxHitPoints = maxHitPoints * 6 / 5;
        }
    }
}
