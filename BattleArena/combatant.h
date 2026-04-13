#pragma once

#include <string>
#include <array>
#include "position.h"
#include "attackresult.h"
#include "worldstate.h"

class Combatant {
public:
    Combatant(const std::string& name,
        int hitPoints,
        int strength,
        int accuracy,
        int agility,
        int defense,
        const Position& position,
        const std::array<std::string, 6>& portrait);

    virtual ~Combatant() = default;

    virtual AttackResult attack(Combatant& target) = 0;
    virtual std::string speak() const = 0;

    virtual bool isAlive() const;
    virtual void takeDamage(int amount);
    virtual int calculateHitChance(const Combatant& target) const;
    virtual bool attackHits(const Combatant& target) const;
    virtual int calculateDamage(const Combatant& target) const;

    virtual bool tryMoveTo(const Position& desiredPosition, const WorldState& world);

    const std::string& getName() const;
    int getHitPoints() const;
    int getMaxHitPoints() const;
    int getStrength() const;
    int getAccuracy() const;
    int getAgility() const;
    int getDefense() const;
    char getSymbol() const;
    const Position& getPosition() const;
    void setPosition(const Position& newPosition);
    void setSymbol(const char symbol);
    const std::array<std::string, 6>& getPortrait() const;

protected:
    std::string name;
    int hitPoints;
    int maxHitPoints;
    int strength;
    int accuracy;
    int agility;
    int defense;
    Position position;
    std::array<std::string, 6> portrait;
    char symbol = '?';

    AttackResult performBasicAttack(Combatant& target);
    bool isOrthogonallyAdjacent(const Combatant& other) const;
};