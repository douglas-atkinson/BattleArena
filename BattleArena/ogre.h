#pragma once

#include "monster.h"
#include "attackresult.h"

class Ogre : public Monster {
public:
	Ogre(const std::string& name, int hitPoints, int strength, int accuracy, int agility,
		int defense, int speed, const Position& position, const std::array<std::string, 6>& portrait);

	AttackResult attack(Combatant& target) override;
	bool move(const WorldState& world) override;

	// Adjust the basic combat formulas while a crushing smash is in progress.
	int calculateHitChance(const Combatant& target) const override;
	int calculateDamage(const Combatant& target) const override;

private:
	static constexpr int SmashChancePercent = 25;
	static constexpr int SmashAccuracyPenalty = 20;

	bool smashing = false;  // true only during a crushing smash
};
