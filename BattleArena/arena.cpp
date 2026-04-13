#include <vector>
#include <memory>
#include <cmath>
#include <iostream>
#include "arena.h"
#include "hero.h"
#include "monster.h"

Arena::Arena(Hero hero, std::vector<std::unique_ptr<Monster>> monsters, int rows, int cols)
    : hero(std::move(hero)), monsters(std::move(monsters)), rows(rows), cols(cols) {
    assignSymbols();
}

void Arena::runBattle() {
    while (!isBattleOver()) {
        printBattleState();

        if (hero.isAlive()) {
            processHeroTurn();
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

void Arena::processHeroTurn() {
    WorldState world = buildWorldStateForHero();

    char command;
    std::cout << "Enter movement command (W/A/S/D/R): ";
    std::cin >> command;

    hero.moveByPlayerCommand(command, world);

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
}

void Arena::processMonsterTurn(Monster& monster) {
    if (!monster.isAlive() || !hero.isAlive()) return;

    WorldState world = buildWorldStateForMonster(&monster);
    monster.move(world);

    if (hero.isAlive() && areOrthogonallyAdjacent(monster, hero)) {
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
            areOrthogonallyAdjacent(hero, *monsterPtr)) {
            return monsterPtr.get();
        }
    }

    return nullptr;
}

bool Arena::areOrthogonallyAdjacent(const Combatant& a, const Combatant& b) const {
    int dr = std::abs(a.getPosition().row - b.getPosition().row);
    int dc = std::abs(a.getPosition().col - b.getPosition().col);
    return (dr + dc == 1);
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

    int index = 0;

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
