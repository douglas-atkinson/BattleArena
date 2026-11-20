#pragma once

#include "monster.h"
#include "attackresult.h"

class Bat : public Monster {
public:
	Bat(const std::string& name, int hitPoints, int strength, int accuracy, int agility,
		int defense, int speed, const Position& position, const std::array<std::string, 6>& portrait);

	AttackResult attack(Combatant& target) override;
	bool move(const WorldState& world) override;
	std::string getTypeName() const override;

	// Adds the sting bonus to the basic damage formula while a sting is in progress.
	int calculateDamage(const Combatant& target) const override;

private:
	static constexpr int TwoStepChancePercent = 20;
	static constexpr int TowardHeroChancePercent = 20;
	static constexpr int StingChancePercent = 15;
	static constexpr int StingBonusDamage = 2;

	bool stinging = false;  // true only during a sting
};
