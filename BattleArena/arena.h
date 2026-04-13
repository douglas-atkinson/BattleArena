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

    void processHeroTurn();
    void processMonsterTurn(Monster& monster);

    Monster* findAdjacentMonsterToHero();
    bool areOrthogonallyAdjacent(const Combatant& a, const Combatant& b) const;

    void removeDeadMonsters();
    void printBattleState() const;

    void assignSymbols();

    void validateStartState() const;
    void validateArenaDimensions() const;
    void validateHeroPosition() const;
    void validateMonsterPositions() const;
    void validateNoOverlappingCombatants() const;
};
