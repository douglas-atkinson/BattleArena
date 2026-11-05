#include "combatant.h"
#include "hero.h"
#include "attackresult.h"
#include "herotitle.h"
#include "position.h"
#include "movementresult.h"
#include <cctype>
#include <string>

Hero::Hero(const std::string& name, HeroTitle title, int hitPoints, int strength, int accuracy, 
           int agility, int defense, int speed, const Position& position,
           const std::array<std::string, 6>& portrait)
    : Combatant(name, hitPoints, strength, accuracy, agility, defense, speed, position, portrait),
    title(title), level(1), victories(0) {}

MovementResult Hero::moveByPlayerCommand(const std::string& commands, const WorldState& world) {
    if (commands.empty()) {
        return MovementResult::InvalidCommand;
    }

    if (commands.size() > static_cast<std::size_t>(getSpeed())) {
        return MovementResult::InvalidCommand;
    }

    std::string normalized;
    normalized.reserve(commands.size());

    for (char ch : commands) {
        normalized.push_back(
            static_cast<char>(std::tolower(static_cast<unsigned char>(ch)))
        );
    }

    if (normalized == "r") {
        return MovementResult::Success;
    }

    if (normalized.find('r') != std::string::npos) {
        return MovementResult::InvalidCommand;
    }

	const std::string validCommands = "wasd";
    for (char command : normalized) {
        if (validCommands.find(command) == std::string::npos) {
            return MovementResult::InvalidCommand;
        }
	}

    // Walk the whole path on a local copy so a blocked step leaves the hero where it started.
    Position pathPos = position;

    for (char command : normalized) {
        switch (command) {
        case 'w': pathPos.row -= 1; break;
        case 'a': pathPos.col -= 1; break;
        case 's': pathPos.row += 1; break;
        case 'd': pathPos.col += 1; break;
        default:
			return MovementResult::InvalidCommand; // don't really need this since we validated above, but just in case
        }

        if (!world.isInBounds(pathPos)) {
            return MovementResult::Blocked;
        }

        // world.heroPosition is the hero's own starting square, which the path may revisit.
        if (pathPos != position && world.isOccupied(pathPos)) {
            return MovementResult::Blocked;
        }
    }

    position = pathPos;
    return MovementResult::Success;
}

AttackResult Hero::attack(Combatant& target) {
    return performBasicAttack(target);
}

std::string Hero::speak() const {
    std::string words = getTitleString() + " " + name + ", onward to victory!";
    return words;
}

HeroTitle Hero::getTitle() const {
    return title;
}

std::string Hero::getTitleString() const {
    return heroTitleToString(title);
}

int Hero::getLevel() const {
    return level;
}

int Hero::getVictories() const {
    return victories;
}

void Hero::recordVictory() {
    ++victories;
    if (victories % 3 == 0) {
        ++level;
        if (level % 2 == 0) {
            // This is intentional
            // maxHitPoints will increase about 20% give or take a truncated fraction
            // hitPoints is not updated until a new Battle Arena round starts
            maxHitPoints = maxHitPoints * 6 / 5;
        }
    }
}
