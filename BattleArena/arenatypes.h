#pragma once

#include <string>
#include <vector>
#include "position.h"
#include "movementresult.h"

// Read-only, value-based copies of the Arena state for a presentation layer.
// They hold no pointers or references to the live Hero or Monster objects.

struct CombatantSnapshot {
    int id;                 // hero is 0; monsters are 1..N in their original order
    std::string name;
    std::string type;       // "Hero", or the monster's type name
    char symbol;            // Arena-assigned display symbol ('@', 'A', 'B', ...)
    Position position;
    int hitPoints;
    int maxHitPoints;
    int strength;
    int accuracy;
    int agility;
    int defense;
    int speed;
    bool alive;
    bool onBattlefield;     // true while the combatant (or its corpse) occupies its square
};

struct ArenaSnapshot {
    int rows;
    int cols;
    CombatantSnapshot hero;
    std::vector<CombatantSnapshot> monsters;
    int heroLevel;
    int monstersDefeated;
    bool battleOver;
    bool heroWon;
    int roundsCompleted;    // accepted, completed player rounds
    std::string heroTitle;
    bool canFight;          // battle active and a living monster is orthogonally adjacent to the hero
};

// What a player command asks the hero to do.
enum class PlayerAction { Move, Rest, Fight };

// The result of checking a player command without executing it.
struct CommandPreview {
    MovementResult status;
    Position destination;   // proposed final square if valid; otherwise the hero's current square
    std::string message;    // brief feedback for the player
    bool canSubmit;         // valid command and the battle is still active
    PlayerAction action;    // Fight for F, Rest for a valid R, otherwise Move
};

// The outcome of submitting a player command.
struct TurnResult {
    bool accepted;                      // true only if the command was valid and a round ran
    ArenaSnapshot state;                // the state after the attempt
    std::vector<std::string> messages;  // new messages from this attempt, in order
};
