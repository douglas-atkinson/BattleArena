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
        const Position& position,
        const std::array<std::string, 6>& portrait,
        char symbol);

    virtual bool move(const WorldState& world) = 0;
    std::string speak() const override;
};
