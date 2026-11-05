#include "goblin.h"
#include "worldstate.h"

Goblin::Goblin(const std::string& name, int hitPoints, int strength, int accuracy, int agility,
	int defense, int speed, const Position& position, const std::array<std::string, 6>& portrait)
	: Monster(name, hitPoints, strength, accuracy, agility, defense, speed, position, portrait) {}

AttackResult Goblin::attack(Combatant& target) {
	return performBasicAttack(target);
}

// Takes up to speed steps, stopping early when no useful step remains.
// Returns true if the goblin moved at least one square.
bool Goblin::move(const WorldState& world) {
	bool moved = false;

	for (int step = 0; step < getSpeed(); ++step) {
		bool stepped = isRetreating() ? stepAwayFromHero(world) : stepTowardHero(world);
		if (!stepped) {
			break;
		}
		moved = true;
	}

	return moved;
}

bool Goblin::isRetreating() const {
	// Multiplying avoids the rounding that hitPoints < maxHitPoints / 3 would cause.
	return hitPoints * 3 < maxHitPoints;
}
