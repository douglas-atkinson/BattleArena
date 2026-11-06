#include "ogre.h"
#include "random.h"
#include "worldstate.h"

Ogre::Ogre(const std::string& name, int hitPoints, int strength, int accuracy, int agility,
	int defense, int speed, const Position& position, const std::array<std::string, 6>& portrait)
	: Monster(name, hitPoints, strength, accuracy, agility, defense, speed, position, portrait) {}

// A crushing smash reuses performBasicAttack(). That function calls the virtual
// calculateHitChance() and calculateDamage(), so the overrides below apply the
// smash's penalty and bonus while the smashing flag is set.
AttackResult Ogre::attack(Combatant& target) {
	smashing = Random::getUniformInt(1, 100) <= SmashChancePercent;
	AttackResult result = performBasicAttack(target);
	bool wasSmash = smashing;
	smashing = false;

	if (!wasSmash) {
		result.messages.push_back(name + " swings its club.");
	}
	else if (result.hit) {
		result.messages.push_back(name + " winds up a crushing smash...");
		result.messages.push_back("The smash lands with bone-rattling force!");
	}
	else {
		result.messages.push_back(name + " winds up a crushing smash...");
		result.messages.push_back("The smash misses and slams into the ground.");
	}

	return result;
}

// Takes up to speed steps toward the hero, stopping early when blocked or adjacent.
// Returns true if the ogre moved at least one square.
bool Ogre::move(const WorldState& world) {
	bool moved = false;

	for (int step = 0; step < getSpeed(); ++step) {
		if (!stepTowardHero(world)) {
			break;
		}
		moved = true;
	}

	return moved;
}

int Ogre::calculateHitChance(const Combatant& target) const {
	int chance = Combatant::calculateHitChance(target);
	return smashing ? chance - SmashAccuracyPenalty : chance;
}

int Ogre::calculateDamage(const Combatant& target) const {
	int damage = Combatant::calculateDamage(target);
	return smashing ? damage + damage / 2 : damage;
}
