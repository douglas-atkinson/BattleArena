#pragma once

#include <memory>
#include <string>
#include <vector>
#include "hero.h"
#include "monster.h"
#include "worldstate.h"
#include "arenatypes.h"

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

    // A value-based copy of the current state. Monster IDs are 1..N in insertion order.
    ArenaSnapshot getSnapshot() const;

    // Checks a player command against the current state without executing it.
    // Changes nothing: no movement, turn, attack, healing, or random numbers.
    CommandPreview previewCommand(const std::string& command) const;

private:
    WorldState buildWorldStateForHero() const;
    WorldState buildWorldStateForMonster(const Monster* activeMonster) const;

    bool isBattleOver() const;
    bool heroWon() const;
    bool hasLivingMonsters() const;

    // The healing opportunity the hero's turn earned, if the round stays peaceful.
    enum class RecoveryChance { None, Moved, Rested };

    bool processHeroTurn(RecoveryChance& recovery);
    bool processMonsterTurn(Monster& monster);
    void tryHeroRecovery(RecoveryChance recovery);

    Monster* findAdjacentMonsterToHero();

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
