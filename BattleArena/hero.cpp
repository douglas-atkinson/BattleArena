#include "combatant.h"
#include "hero.h"
#include "attackresult.h"
#include "herotitle.h"
#include "position.h"
#include <cctype>

Hero::Hero(const std::string& name, HeroTitle title, int hitPoints, int strength, int accuracy, 
           int agility, int defense, const Position& position, const std::array<std::string, 6>& portrait, 
           char symbol)
    : Combatant(name, hitPoints, strength, accuracy, agility, defense, position, portrait, symbol),
    title(title), level(1), victories(0) {}

bool Hero::moveByPlayerCommand(char command, const WorldState& world) {
    Position newPos = position;
    command = tolower(command);
    if (command == 'w') newPos.row--;   // Move north
    if (command == 'a') newPos.col--;   // Move west
    if (command == 's') newPos.row++;   // Move south
    if (command == 'd') newPos.col++;   // Move east
    if (tryMoveTo(newPos, world)) {
        position = newPos;
        return true;
    }
    return false;
}

AttackResult Hero::attack(Combatant& target) {
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
            maxHitPoints *= 1.2;
        }
    }
}
