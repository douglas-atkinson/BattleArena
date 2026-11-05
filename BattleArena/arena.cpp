#include <vector>
#include <memory>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <set>
#include <utility>
#include "arena.h"
#include "hero.h"
#include "monster.h"
#include "position.h"

Arena::Arena(Hero hero, std::vector<std::unique_ptr<Monster>> monsters, int rows, int cols)
    : hero(std::move(hero)), monsters(std::move(monsters)), rows(rows), cols(cols) {
    validateStartState();
    assignSymbols();
}

void Arena::runBattle() {
    while (!isBattleOver()) {
        printBattleState();

        if (hero.isAlive()) {
			displayBattleFieldLegend();
			displayBattleField();
            if (!processHeroTurn()) {
                std::cout << "\nInput ended. Battle abandoned.\n";
                return;
            }
			displayBattleFieldLegend();
			displayBattleField();
        }

        removeDeadMonsters();

        if (isBattleOver()) break;

        for (auto& monsterPtr : monsters) {
            if (!monsterPtr->isAlive()) continue;

            processMonsterTurn(*monsterPtr);

            if (!hero.isAlive()) break;
        }

        removeDeadMonsters();
    }

    printBattleState();

    if (heroWon()) {
        std::cout << "Victory!\n";
    }
    else {
        std::cout << "Game Over.\n";
    }
}

WorldState Arena::buildWorldStateForHero() const {
    std::vector<Position> monsterPositions;

    for (const auto& monsterPtr : monsters) {
        if (monsterPtr->isAlive()) {
            monsterPositions.push_back(monsterPtr->getPosition());
        }
    }

    return WorldState{ rows, cols, hero.getPosition(), monsterPositions };
}

WorldState Arena::buildWorldStateForMonster(const Monster* activeMonster) const {
    std::vector<Position> monsterPositions;

    for (const auto& monsterPtr : monsters) {
        if (monsterPtr.get() != activeMonster && monsterPtr->isAlive()) {
            monsterPositions.push_back(monsterPtr->getPosition());
        }
    }

    return WorldState{ rows, cols, hero.getPosition(), monsterPositions };
}

bool Arena::isBattleOver() const {
    return !hero.isAlive() || monsters.empty();
}

bool Arena::heroWon() const {
    return hero.isAlive() && monsters.empty();
}

// Returns false if input ended before the hero could take a turn.
bool Arena::processHeroTurn() {
    WorldState world = buildWorldStateForHero();

    std::string command;
    MovementResult result = MovementResult::InvalidCommand;

    while (result == MovementResult::InvalidCommand) {
        std::cout << "Enter movement command (W/A/S/D/R): ";
        if (!std::getline(std::cin, command)) {
            return false;
        }

        result = hero.moveByPlayerCommand(command, world);
        if (result == MovementResult::InvalidCommand) {
            std::cout << "Invalid input. Enter R to rest, or W/A/S/D moves (maximum "
                << hero.getSpeed() << ").\n";
        }
    }

    if (result == MovementResult::Blocked) {
        std::cout << "You cannot move there.\n";
    }

    Monster* target = findAdjacentMonsterToHero();
    if (target != nullptr && hero.isAlive() && target->isAlive()) {
        AttackResult result = hero.attack(*target);

        std::cout << "\nHero Attack Result\n";
        std::cout << "Hit: " << (result.hit ? "Yes" : "No") << '\n';
        std::cout << "Damage done: " << result.damage << '\n';
        std::cout << "Opponent defeated: "
            << (result.targetDefeated ? "Yes" : "No") << '\n';

        if (result.targetDefeated) {
            hero.recordVictory();
        }
    }

    return true;
}

void Arena::processMonsterTurn(Monster& monster) {
    if (!monster.isAlive() || !hero.isAlive()) return;

    WorldState world = buildWorldStateForMonster(&monster);
    monster.move(world);

    if (hero.isAlive() && areOrthogonallyAdjacent(monster.getPosition(), hero.getPosition())) {
        AttackResult result = monster.attack(hero);

        std::cout << "\n" << monster.getName() << "'s Attack Result\n";
        std::cout << "Hit: " << (result.hit ? "Yes" : "No") << '\n';
        std::cout << "Damage done: " << result.damage << '\n';
        std::cout << "Opponent defeated: "
            << (result.targetDefeated ? "Yes" : "No") << '\n';
    }
}

Monster* Arena::findAdjacentMonsterToHero() {
    for (auto& monsterPtr : monsters) {
        if (monsterPtr->isAlive() &&
            areOrthogonallyAdjacent(hero.getPosition(), monsterPtr->getPosition())) {
            return monsterPtr.get();
        }
    }

    return nullptr;
}

void Arena::removeDeadMonsters() {
    std::erase_if(monsters, [](const auto& m) { return !m->isAlive(); });

}

void Arena::printBattleState() const {
    std::cout << "\n========================================\n";
    std::cout << "Current Battle State\n";
    std::cout << "========================================\n";

    std::cout << "Hero: " << hero.getName() << '\n';
    std::cout << "Position: (" << hero.getPosition().row
        << ", " << hero.getPosition().col << ")\n";
    std::cout << "HP: " << hero.getHitPoints()
        << " / " << hero.getMaxHitPoints() << '\n';
    std::cout << "Level: " << hero.getLevel() << '\n';
    std::cout << "Victories: " << hero.getVictories() << '\n';
    std::cout << "Status: " << (hero.isAlive() ? "Alive" : "Dead") << "\n\n";

    std::cout << "Monsters:\n";

    if (monsters.empty()) {
        std::cout << "None\n";
    }
    else {
        for (size_t i = 0; i < monsters.size(); ++i) {
            const Monster& monster = *monsters[i];

            std::cout << i + 1 << ". " << monster.getName() << '\n';
            std::cout << "   Position: (" << monster.getPosition().row
                << ", " << monster.getPosition().col << ")\n";
            std::cout << "   HP: " << monster.getHitPoints()
                << " / " << monster.getMaxHitPoints() << '\n';
            std::cout << "   Status: "
                << (monster.isAlive() ? "Alive" : "Dead") << "\n\n";
        }
    }
}

void Arena::assignSymbols() {
    std::vector<char> pool = {
        '@', 'A', 'B', 'C', 'D', 'E', 'F',
        'G', 'H', 'I', 'J', 'K', 'L'
    };

    std::size_t index = 0;

    hero.setSymbol(pool[index++]);

    for (auto& monster : monsters) {
        if (index < pool.size()) {
            monster->setSymbol(pool[index++]);
        }
        else {
            monster->setSymbol('?'); // fallback
        }
    }
}

void Arena::validateStartState() const {
    validateArenaDimensions();
    validateHeroPosition();
    validateNoNullMonsters();
    validateMonsterPositions();
    validateNoOverlappingCombatants();
}

void Arena::validateArenaDimensions() const {
    if (rows <= 0 || cols <= 0) {
        throw std::invalid_argument("Arena dimensions must be greater than 0.");
    }
}

void Arena::validateHeroPosition() const {
    const Position pos = hero.getPosition();
    if (!inBounds(pos.row, pos.col)) {
        throw std::invalid_argument("Hero's position must be within Arena boundaries");
    }
}

void Arena::validateNoNullMonsters() const {
    for (const auto& monster : monsters) {
        if (monster == nullptr) {
            throw std::invalid_argument("Pointer to Monster cannot be null");
        }
    }
}

void Arena::validateMonsterPositions() const {
    for (const auto& monster : monsters) {
        const Position pos = monster->getPosition();
        if (!inBounds(pos.row, pos.col)) {
            throw std::invalid_argument("Monster's position must be within Arena boundaries");
        }
    }

}

void Arena::validateNoOverlappingCombatants() const {
    std::set<std::pair<int, int>> occupied;

    const Position heroPos = hero.getPosition();
    occupied.insert({ heroPos.row, heroPos.col });

    for (const auto& monster : monsters) {
        const Position pos = monster->getPosition();

        if (!occupied.insert({ pos.row, pos.col }).second) {
            throw std::invalid_argument("Two combatants cannot occupy the same starting position.");
        }
    }
}

bool Arena::inBounds(int row, int col) const {
    return (row >= 0 && row < rows && col >= 0 && col < cols);
}

void Arena::displayBattleField() const {
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {

			Position current{ row, col };
            char symbol = '.';

            if (hero.getPosition() == current) {
                symbol = hero.getSymbol();
            }
            else {
                // Check monsters
                for (const auto& monster : monsters) {
                    if (monster->isAlive() && monster->getPosition() == current) {
                        symbol = monster->getSymbol();
                        break;
					}
                }
            }
            std::cout << symbol << ' ';
        }
        std::cout << '\n';
	}
	std::cout << '\n';
}

void Arena::displayBattleFieldLegend() const {
    std::cout << hero.getSymbol() << " = Hero\n";
    for (const auto& monster : monsters) {
        std::cout << monster->getSymbol() << " = Monster\n";
    }
    std::cout << '\n';
}
