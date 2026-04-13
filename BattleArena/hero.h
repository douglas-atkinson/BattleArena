#pragma once

#include "combatant.h"
#include "herotitle.h"
#include "worldstate.h"
#include "movementresult.h"

class Hero : public Combatant {
public:
    Hero(const std::string& name,
        HeroTitle title,
        int hitPoints,
        int strength,
        int accuracy,
        int agility,
        int defense,
        const Position& position,
        const std::array<std::string, 6>& portrait);

    MovementResult moveByPlayerCommand(char command, const WorldState& world);
    AttackResult attack(Combatant& target) override;
    std::string speak() const override;

    HeroTitle getTitle() const;
    std::string getTitleString() const;
    int getLevel() const;
    int getVictories() const;

    void recordVictory();

private:
    HeroTitle title;
    int level;
    int victories;
};
