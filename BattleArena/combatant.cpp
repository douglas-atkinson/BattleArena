#include <algorithm>
#include <cmath>
#include "combatant.h"
#include "random.h"
#include "worldstate.h"

Combatant::Combatant(const std::string& name, int hitPoints, int strength, int accuracy, 
                    int agility, int defense, const Position& position, 
                    const std::array<std::string, 6>& portrait)
    : name(name), hitPoints(std::max(1, hitPoints)), maxHitPoints(std::max(1, hitPoints)), 
      strength(std::max(1, strength)), accuracy(std::max(1, accuracy)), agility(std::max(1, agility)),
      defense(std::max(1, defense)), position(position),
      portrait(portrait) { }


bool Combatant::isAlive() const {
    return hitPoints > 0;
}

void Combatant::takeDamage(int amount) {
    // Saving throw??? Maybe
    hitPoints = (amount > hitPoints ? 0 : hitPoints - amount);
}

int Combatant::calculateHitChance(const Combatant& target) const {
    int chance = 50 + accuracy - target.agility;
    return std::clamp(chance, 20, 85);
}

bool Combatant::attackHits(const Combatant& target) const {
    int chance = calculateHitChance(target);
    int roll = Random::getUniformInt(1, 100);
    return roll <= chance;
}

int Combatant::calculateDamage(const Combatant& target) const {
    int rawDamage = strength + Random::getUniformInt(0, 5);
    int reducedDamage = rawDamage - target.defense;
    return std::max(1, reducedDamage);
}

bool Combatant::tryMoveTo(const Position& desiredPosition, const WorldState& world) {
    if (desiredPosition == position) return true;
    if (!world.isInBounds(desiredPosition)) return false;
    if (world.isOccupied(desiredPosition)) return false;

    position = desiredPosition;
    return true;
}

const std::string& Combatant::getName() const {
    return name;
}

int Combatant::getHitPoints() const {
    return hitPoints;
}

int Combatant::getMaxHitPoints() const {
    return maxHitPoints;
}

int Combatant::getStrength() const {
    return strength;
}

int Combatant::getAccuracy() const {
    return accuracy;
}

int Combatant::getAgility() const {
    return agility;
}

int Combatant::getDefense() const {
    return defense;
}

char Combatant::getSymbol() const {
    return symbol;
}

const Position& Combatant::getPosition() const {
    return position;
}

void Combatant::setPosition(const Position& newPosition) {
    position = newPosition;
}

void Combatant::setSymbol(const char symbol) {
    this->symbol = symbol;
}

const std::array<std::string, 6>& Combatant::getPortrait() const {
    return portrait;
}

AttackResult Combatant::performBasicAttack(Combatant& target) {
    AttackResult result;

    if (!isAlive()) return result;
    if (!target.isAlive()) return result;
    if (&target == this) return result;

    if (attackHits(target)) {
        result.hit = true;
        result.damage = calculateDamage(target);
        target.takeDamage(result.damage);
        result.targetDefeated = !target.isAlive();
    }

    return result;
}

bool Combatant::isOrthogonallyAdjacent(const Combatant& other) const {
    int dr = std::abs(position.row - other.position.row);
    int dc = std::abs(position.col - other.position.col);
    return (dr + dc == 1);
}