#pragma once

#include <string>
#include <vector>
#include "position.h"

// Read-only, value-based copies of the Arena state for a presentation layer.
// They hold no pointers or references to the live Hero or Monster objects.

struct CombatantSnapshot {
    int id;                 // hero is 0; monsters are 1..N in their original order
    std::string name;
    std::string type;       // "Hero", or the monster's type name
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
};
