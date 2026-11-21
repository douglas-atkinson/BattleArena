#pragma once

// Hardcoded sample state for the static mockup. Nothing here is connected to the
// real BattleArena model; values are representative only (stats loosely follow
// BattleArenaMain/main.cpp so the numbers feel plausible).

#include "Art.hpp"

#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Vector2.hpp>

#include <string>
#include <vector>

struct Character
{
    std::string name;
    std::string typeName;   // later: Monster::getTypeName()
    art::Kind kind;
    int hp;
    int maxHp;
    sf::Vector2i tile;      // column, row on the 30x16 grid
    sf::Color accent;

    bool alive() const { return hp > 0; }
};

enum class LogKind { Info, Damage, Heal, Special, Victory, System };

struct LogEntry
{
    int round;
    std::string text;
    LogKind kind;
};

struct HeroInfo
{
    std::string title = "Knight";
    int level = 2;
    int kills = 1;
    int lastDamage = 4;  // most recent change, shown in hero status
    int lastHeal = 2;
};

struct MockState
{
    int arenaLevel = 1;
    int round = 12;
    std::vector<Character> characters;  // index 0 is always the hero
    HeroInfo hero;
    std::vector<LogEntry> log;          // oldest first
};

inline MockState makeSampleState()
{
    MockState s;
    s.characters = {
        {"Bob", "Knight", art::Kind::Knight, 17, 24, {11, 8}, sf::Color(106, 158, 192)},
        {"Grub", "Goblin", art::Kind::Goblin, 15, 24, {15, 6}, sf::Color(108, 170, 88)},
        {"Brak", "Ogre", art::Kind::Ogre, 31, 40, {12, 10}, sf::Color(204, 140, 82)},
        {"Bones", "Skeleton", art::Kind::Skeleton, 0, 16, {6, 12}, sf::Color(214, 204, 176)},
        {"Squeak", "Bat", art::Kind::Bat, 4, 6, {23, 3}, sf::Color(170, 112, 178)},
    };
    s.log = {
        {7, "The battle begins!", LogKind::System},
        {7, "Bones rattles toward Bob.", LogKind::Info},
        {8, "Bob strikes Bones for 5 damage.", LogKind::Info},
        {8, "Bones slashes Bob for 3 damage.", LogKind::Damage},
        {9, "Squeak stings Bob for 2 damage!", LogKind::Special},
        {10, "Bob strikes Bones for 6 damage.", LogKind::Info},
        {10, "Bones has been defeated!", LogKind::Victory},
        {11, "Grub retreats from Bob!", LogKind::Info},
        {11, "Bob recovers 2 HP.", LogKind::Heal},
        {12, "Brak unleashes a crushing smash!", LogKind::Special},
        {12, "Bob takes 4 damage.", LogKind::Damage},
        {12, "Squeak flutters out of reach.", LogKind::Info},
    };
    return s;
}
