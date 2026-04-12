#include "combatant.h"
#include "monster.h"
#include "random.h"

Monster::Monster(const std::string& name, int hitPoints, int strength, int accuracy, int agility, 
    int defense, const Position& position, const std::array<std::string, 6>& portrait, char symbol)
    : Combatant(name, hitPoints, strength, accuracy, agility, defense, position, portrait, symbol) {}

std::string Monster::speak() const {
    return "Grrrr";
}
