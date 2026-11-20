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
        int speed,
        const Position& position,
        const std::array<std::string, 6>& portrait);

    MovementResult moveByPlayerCommand(const std::string& commands, const WorldState& world);
    AttackResult attack(Combatant& target) override;
    std::string speak() const override;

    HeroTitle getTitle() const;
    std::string getTitleString() const;
    int getLevel() const;
    int getVictories() const;

    void recordVictory();

    // Restores up to amount HP without exceeding the maximum. Returns the HP actually restored.
    int recoverHitPoints(int amount);

private:
    HeroTitle title;
    int level;
    int victories;
};
