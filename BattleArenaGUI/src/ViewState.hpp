#pragma once

// The renderer's copy of the game state. makeViewState() converts an ArenaSnapshot into
// this presentation-friendly form; the Arena remains the only authority over game state.

#include "Art.hpp"
#include "Layout.hpp"

#include "arenatypes.h"

#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Vector2.hpp>

#include <string>
#include <vector>

struct Character
{
    int id;                 // CombatantSnapshot::id: hero 0, monsters 1..N
    std::string name;
    std::string typeName;   // monster type ("Goblin", ...), or the hero's title
    art::Kind kind;
    int hp;
    int maxHp;
    sf::Vector2i tile;      // (column, row) on the battlefield grid
    sf::Color accent;
    bool alive;
    bool onBattlefield;     // a defeated monster may still leave a corpse on its tile
};

enum class LogKind { Info, Damage, Heal, Special, Victory, System };

struct LogEntry
{
    int round;
    std::string text;
    LogKind kind;
};

struct ViewState
{
    int round = 1;                      // the round shown: the upcoming one while the battle continues
    int roundsCompleted = 0;
    int heroLevel = 1;
    std::string heroTitle;
    int monstersDefeated = 0;
    bool canFight = false;
    layout::Grid grid;
    std::vector<Character> characters;  // index 0 is the hero, then monsters in ID order
    std::vector<LogEntry> log;          // oldest first
    std::string visualTest;             // non-empty only in a visual test mode (see main.cpp)
};

// The one place where a model Position {row, col} becomes a screen tile (col, row).
inline sf::Vector2i toTile(const Position& position)
{
    return sf::Vector2i{position.col, position.row};
}

// Artwork and accent colour by combatant type. The sample battle's hero is a Knight.
inline art::Kind artFor(const std::string& type)
{
    if (type == "Goblin")
        return art::Kind::Goblin;
    if (type == "Ogre")
        return art::Kind::Ogre;
    if (type == "Skeleton")
        return art::Kind::Skeleton;
    if (type == "Bat")
        return art::Kind::Bat;
    return art::Kind::Knight;  // the hero (no other artwork exists yet)
}

inline sf::Color accentFor(art::Kind kind)
{
    switch (kind)
    {
    case art::Kind::Knight: return sf::Color(106, 158, 192);
    case art::Kind::Goblin: return sf::Color(108, 170, 88);
    case art::Kind::Ogre: return sf::Color(204, 140, 82);
    case art::Kind::Skeleton: return sf::Color(214, 204, 176);
    case art::Kind::Bat: return sf::Color(170, 112, 178);
    }
    return sf::Color::White;
}

inline Character toCharacter(const CombatantSnapshot& c, const std::string& typeName, art::Kind kind)
{
    return {c.id, c.name, typeName, kind, c.hitPoints, c.maxHitPoints, toTile(c.position), accentFor(kind),
            c.alive, c.onBattlefield};
}

inline ViewState makeViewState(const ArenaSnapshot& snapshot)
{
    ViewState v;
    v.roundsCompleted = snapshot.roundsCompleted;
    v.round = snapshot.roundsCompleted + (snapshot.battleOver ? 0 : 1);  // same as the console
    v.heroLevel = snapshot.heroLevel;
    v.heroTitle = snapshot.heroTitle;
    v.monstersDefeated = snapshot.monstersDefeated;
    v.canFight = snapshot.canFight;
    v.grid = layout::Grid{snapshot.cols, snapshot.rows};
    v.characters.push_back(toCharacter(snapshot.hero, snapshot.heroTitle, art::Kind::Knight));
    for (const CombatantSnapshot& monster : snapshot.monsters)
        v.characters.push_back(toCharacter(monster, monster.type, artFor(monster.type)));
    return v;
}

// Visual test mode only: shows the same characters on a different grid size, scaling each
// position proportionally; a character that would share a tile takes the next free tile
// in row-major order. This is for checking layouts, never for the live game.
inline void applyTestGrid(ViewState& v, const layout::Grid& grid)
{
    const layout::Grid from = v.grid;
    const int count = grid.cols * grid.rows;
    std::vector<sf::Vector2i> placed;
    for (Character& c : v.characters)
    {
        int index = (c.tile.y * grid.rows / from.rows) * grid.cols + c.tile.x * grid.cols / from.cols;
        for (int tries = 0; tries < count; ++tries, index = (index + 1) % count)
        {
            const sf::Vector2i tile{index % grid.cols, index / grid.cols};
            bool occupied = false;
            for (const sf::Vector2i& p : placed)
                occupied = occupied || p == tile;
            if (!occupied)
            {
                c.tile = tile;
                break;
            }
        }
        placed.push_back(c.tile);
    }
    v.grid = grid;
}
