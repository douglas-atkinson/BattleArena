#pragma once

#include <memory>
#include <vector>
#include "hero.h"
#include "monster.h"
#include "worldstate.h"

class Arena {
private:
    Hero hero;
    std::vector<std::unique_ptr<Monster>> monsters;
    int rows;
    int cols;

public:
    Arena(Hero hero, std::vector<std::unique_ptr<Monster>> monsters,
        int rows, int cols);

    void runBattle();

private:
    WorldState buildWorldStateForHero() const;
    WorldState buildWorldStateForMonster(const Monster* activeMonster) const;

    bool isBattleOver() const;
    bool heroWon() const;

    bool processHeroTurn();
    void processMonsterTurn(Monster& monster);

    Monster* findAdjacentMonsterToHero();

    void removeDeadMonsters();
    void printBattleState() const;

    void assignSymbols();

    void validateStartState() const;
    void validateArenaDimensions() const;
    void validateHeroPosition() const;
    void validateNoNullMonsters() const;
    void validateMonsterPositions() const;
    void validateNoOverlappingCombatants() const;
    bool inBounds(int row, int col) const;

	void displayBattleField() const;
	void displayBattleFieldLegend() const;
};
