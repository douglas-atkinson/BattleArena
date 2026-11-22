#pragma once

#include <memory>
#include <string>
#include <vector>
#include "hero.h"
#include "monster.h"
#include "worldstate.h"
#include "arenatypes.h"

// The game controller: accepts player commands, resolves complete rounds, and reports
// state through value snapshots and messages. It performs no console input or output.
class Arena {
private:
    Hero hero;
    std::vector<std::unique_ptr<Monster>> monsters;
    int rows;
    int cols;
    int roundsCompleted = 0;

public:
    Arena(Hero hero, std::vector<std::unique_ptr<Monster>> monsters,
        int rows, int cols);

    // A value-based copy of the current state. Monster IDs are 1..N in insertion order.
    ArenaSnapshot getSnapshot() const;

    // Checks a player command against the current state without executing it.
    // Changes nothing: no movement, turn, attack, healing, or random numbers.
    CommandPreview previewCommand(const std::string& command) const;

    // Validates the command and, if it is legal, resolves one complete round.
    // A rejected command changes nothing and returns an explanation.
    TurnResult submitCommand(const std::string& command);

private:
    WorldState buildWorldStateForHero() const;
    WorldState buildWorldStateForMonster(const Monster* activeMonster) const;

    bool isBattleOver() const;
    bool heroWon() const;
    bool hasLivingMonsters() const;
    bool canHeroFight() const;

    std::string describeValidation(const MovementValidation& validation,
        const std::string& command) const;

    // The healing opportunity the hero's action earned, if the round stays peaceful.
    enum class RecoveryChance { None, Moved, Rested };

    bool processHeroAttack(std::vector<std::string>& messages);
    bool processMonsterTurn(Monster& monster, std::vector<std::string>& messages);
    void tryHeroRecovery(RecoveryChance recovery, std::vector<std::string>& messages);

    Monster* findAdjacentMonsterToHero() const;

    void assignSymbols();

    void validateStartState() const;
    void validateArenaDimensions() const;
    void validateHeroPosition() const;
    void validateNoNullMonsters() const;
    void validateMonsterPositions() const;
    void validateNoOverlappingCombatants() const;
    bool inBounds(int row, int col) const;
};
