#pragma once

#include "monster.h"
#include "attackresult.h"

class Goblin : public Monster {
public:
	Goblin(const std::string& name, int hitPoints, int strength, int accuracy, int agility,
		int defense, int speed, const Position& position, const std::array<std::string, 6>& portrait);

	AttackResult attack(Combatant& target) override;
	bool move(const WorldState& world) override;

private:
	bool isRetreating() const;
};
