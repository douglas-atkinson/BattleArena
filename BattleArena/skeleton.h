#pragma once

#include "monster.h"
#include "attackresult.h"

class Skeleton : public Monster {
public:
	Skeleton(const std::string& name, int hitPoints, int strength, int accuracy, int agility,
		int defense, int speed, const Position& position, const std::array<std::string, 6>& portrait);

	AttackResult attack(Combatant& target) override;
	bool move(const WorldState& world) override;

private:
	static constexpr int DetectionRange = 3;

	bool alerted = false;  // once true, stays true for the rest of the battle

	bool canSeeHero(const WorldState& world) const;
};
