#pragma once

#include "combatant.h"
#include "worldstate.h"

class Monster : public Combatant {
public:
    Monster(const std::string& name,
        int hitPoints,
        int strength,
        int accuracy,
        int agility,
        int defense,
        int speed,
        const Position& position,
        const std::array<std::string, 6>& portrait);

    virtual bool move(const WorldState& world) = 0;
    std::string speak() const override;

protected:
    // Single-step movement helpers. Each returns true if the monster moved one square.
    bool stepTowardHero(const WorldState& world);
    bool stepAwayFromHero(const WorldState& world);
    bool stepRandom(const WorldState& world);
};
