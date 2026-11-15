#include <algorithm>
#include "bat.h"
#include "random.h"
#include "worldstate.h"

Bat::Bat(const std::string& name, int hitPoints, int strength, int accuracy, int agility,
	int defense, int speed, const Position& position, const std::array<std::string, 6>& portrait)
	: Monster(name, hitPoints, strength, accuracy, agility, defense, speed, position, portrait) {}

// Like Ogre's smash, a sting reuses performBasicAttack(). The normal hit roll applies,
// and the calculateDamage() override adds the bonus while the stinging flag is set.
AttackResult Bat::attack(Combatant& target) {
	stinging = Random::getUniformInt(1, 100) <= StingChancePercent;
	AttackResult result = performBasicAttack(target);
	bool wasSting = stinging;
	stinging = false;

	if (!wasSting) {
		result.messages.push_back(name + " flutters in and nips.");
	}
	else if (result.hit) {
		result.messages.push_back(name + " dives in for a sting...");
		result.messages.push_back("The sting stabs home!");
	}
	else {
		result.messages.push_back(name + " dives in for a sting...");
		result.messages.push_back("The sting misses as the bat veers away.");
	}

	return result;
}

// Erratic flight: usually one step, sometimes two (never more than speed).
// Each step is usually random, occasionally toward the hero. Stops when a step fails.
// Returns true if the bat moved at least one square.
bool Bat::move(const WorldState& world) {
	int steps = Random::getUniformInt(1, 100) <= TwoStepChancePercent ? 2 : 1;
	steps = std::min(steps, getSpeed());

	bool moved = false;
	for (int step = 0; step < steps; ++step) {
		bool towardHero = Random::getUniformInt(1, 100) <= TowardHeroChancePercent;
		bool stepped = towardHero ? stepTowardHero(world) : stepRandom(world);
		if (!stepped) {
			break;
		}
		moved = true;
	}

	return moved;
}

int Bat::calculateDamage(const Combatant& target) const {
	int damage = Combatant::calculateDamage(target);
	return stinging ? damage + StingBonusDamage : damage;
}
