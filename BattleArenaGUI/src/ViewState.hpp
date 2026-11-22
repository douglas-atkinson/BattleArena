#pragma once

// The renderer's copy of the game state. makeViewState() converts an ArenaSnapshot into
// this presentation-friendly form; the Arena remains the only authority over game state.

#include "Art.hpp"
#include "Layout.hpp"

#include "arenatypes.h"

#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Vector2.hpp>

#include <optional>
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

// What the command bar and battlefield show for the pending (not yet submitted) command.
// Built from Arena::previewCommand() by makeCommandPreviewView(); the GUI never judges
// commands itself.
enum class FeedbackTone { Neutral, Valid, Invalid };

struct CommandPreviewView
{
    std::string feedback;                  // empty: show the default instruction
    FeedbackTone tone = FeedbackTone::Neutral;
    std::optional<sf::Vector2i> ghostTile;  // where a translucent hero is drawn, if anywhere
};

struct ViewState
{
    int round = 1;                      // the round shown: the upcoming one while the battle continues
    int roundsCompleted = 0;
    int heroLevel = 1;
    std::string heroTitle;
    int monstersDefeated = 0;
    bool canFight = false;
    bool battleOver = false;            // straight from ArenaSnapshot: the GUI never decides the outcome
    bool heroWon = false;
    layout::Grid grid;
    std::vector<Character> characters;  // index 0 is the hero, then monsters in ID order
    std::vector<LogEntry> log;          // oldest first
    std::string visualTest;             // non-empty only in a visual test mode (see main.cpp)
    CommandPreviewView preview;         // feedback for the pending command
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
    v.battleOver = snapshot.battleOver;
    v.heroWon = snapshot.heroWon;
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

// Turns the Arena's verdict on a pending command into what the GUI shows. Validity, the
// destination, and every explanation come from the CommandPreview; this only words the
// successful cases and decides whether a ghost is worth drawing.
inline CommandPreviewView makeCommandPreviewView(const CommandPreview& preview, sf::Vector2i heroTile)
{
    CommandPreviewView view;
    if (!preview.canSubmit)
    {
        view.feedback = preview.message;  // e.g. "Movement blocked." or the speed limit
        view.tone = FeedbackTone::Invalid;
        return view;
    }
    view.tone = FeedbackTone::Valid;
    switch (preview.action)
    {
    case PlayerAction::Rest: view.feedback = "Rest available"; break;
    case PlayerAction::Fight: view.feedback = "Fight available"; break;
    case PlayerAction::Move:
        if (toTile(preview.destination) == heroTile)
            view.feedback = "Move available: ends on the starting square";
        else
        {
            view.feedback = "Move available";
            view.ghostTile = toTile(preview.destination);
        }
        break;
    }
    return view;
}

// Colour category for a message from the Arena. Purely cosmetic: it reads the model's
// wording but never changes it, and anything unrecognised is shown as ordinary Info.
inline LogKind classifyLogMessage(const std::string& text, const std::string& heroName)
{
    auto has = [&](const char* part) { return text.find(part) != std::string::npos; };
    auto endsWith = [&](const std::string& tail) {
        return text.size() >= tail.size() && text.compare(text.size() - tail.size(), tail.size(), tail) == 0;
    };
    if (has(" is victorious!") || has(" reaches level ") || has("'s maximum HP rises"))
        return LogKind::Victory;
    if (has(" has fallen.") || text == heroName + " is defeated!")
        return LogKind::Damage;
    if (endsWith(" is defeated!"))
        return LogKind::Victory;
    if (text.rfind("Hit! ", 0) == 0)
        return LogKind::Damage;
    if (has(" recovers ") && endsWith(" HP."))
        return LogKind::Heal;
    if (endsWith("...") || text.rfind("The smash ", 0) == 0 || text.rfind("The sting ", 0) == 0)
        return LogKind::Special;  // special-attack wind-ups and their outcomes
    return LogKind::Info;
}

// Applies an accepted turn: the returned snapshot replaces every displayed game value,
// the existing log is kept, and the round's messages are appended in order, tagged with
// the round that just finished. Returns how many log lines were added.
inline int applyTurnResult(ViewState& v, const TurnResult& result)
{
    ViewState updated = makeViewState(result.state);
    updated.log = std::move(v.log);
    updated.visualTest = v.visualTest;
    const int round = result.state.roundsCompleted;
    for (const std::string& message : result.messages)
        updated.log.push_back({round, message, classifyLogMessage(message, result.state.hero.name)});
    v = std::move(updated);
    return static_cast<int>(result.messages.size());
}
