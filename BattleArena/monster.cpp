#include "combatant.h"
#include "monster.h"
#include "random.h"

Monster::Monster(const std::string& name, int hitPoints, int strength, int accuracy, int agility, 
    int defense, int speed, const Position& position, const std::array<std::string, 6>& portrait)
    : Combatant(name, hitPoints, strength, accuracy, agility, defense, speed, position, portrait) {}

std::string Monster::speak() const {
    return "Grrrr";
}
