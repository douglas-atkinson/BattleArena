#pragma once

#include <string>
#include <array>
#include "position.h"

class Combatant {
public:
    Combatant(const std::string& name,
        int hitPoints,
        int strength,
        int accuracy,
        int agility,
        int defense,
        const Position& position,
        const std::array<std::string, 6>& portrait,
        char symbol);

    virtual ~Combatant() = default;

    virtual void move(int arenaRows, int arenaCols) = 0;
    virtual void attack(Combatant& target) = 0;
    virtual void speak() const = 0;

    virtual bool isAlive() const;
    virtual void takeDamage(int amount);
    virtual int calculateHitChance(const Combatant& target) const;
    virtual bool attackHits(const Combatant& target) const;
    virtual int calculateDamage(const Combatant& target) const;

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
	char symbol;

};