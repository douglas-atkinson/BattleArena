#include "skeleton.h"
#include "position.h"
#include "worldstate.h"

Skeleton::Skeleton(const std::string& name, int hitPoints, int strength, int accuracy, int agility,
	int defense, int speed, const Position& position, const std::array<std::string, 6>& portrait)
	: Monster(name, hitPoints, strength, accuracy, agility, defense, speed, position, portrait) {}

AttackResult Skeleton::attack(Combatant& target) {
	return performBasicAttack(target);
}

// Unalerted: patrol one random square, then check whether the hero is now in range.
// Alerted: pursue the hero for up to speed squares. Alerted never resets.
// Returns true if the skeleton moved at least one square.
bool Skeleton::move(const WorldState& world) {
	if (!alerted && canSeeHero(world)) {
		alerted = true;
	}

	if (!alerted) {
		bool moved = stepRandom(world);
		if (canSeeHero(world)) {
			alerted = true;  // pursuit begins on the next turn
		}
		return moved;
	}

	bool moved = false;
	for (int step = 0; step < getSpeed(); ++step) {
		if (!stepTowardHero(world)) {
			break;
		}
		moved = true;
	}

	return moved;
}

bool Skeleton::canSeeHero(const WorldState& world) const {
	return manhattanDistance(position, world.heroPosition) <= DetectionRange;
}
