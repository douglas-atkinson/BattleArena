#pragma once

#include "monster.h"
#include "attackresult.h"

class DummyMonster : public Monster {
public:
	DummyMonster(const std::string& name, int hitPoints, int strength, int accuracy, int agility,
		int defense, const Position& position, const std::array<std::string, 6>& portrait, char symbol);

	AttackResult attack(Combatant& target) override;
	bool move(const WorldState& world) override;
};
