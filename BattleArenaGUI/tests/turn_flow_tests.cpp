// Turn-flow tests: the real Arena plays rounds and the GUI's ViewState is refreshed from
// each TurnResult. Combat is random, so results are checked against the snapshot the
// Arena returns rather than against particular hits or damage.

#include "ViewState.hpp"

#include "bat.h"
#include "battlefactory.h"
#include "goblin.h"
#include "herotitle.h"
#include "ogre.h"
#include "skeleton.h"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

namespace
{
int failures = 0;

#define CHECK(cond)                                                                 \
    do                                                                              \
    {                                                                               \
        if (!(cond))                                                                \
        {                                                                           \
            ++failures;                                                             \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);             \
        }                                                                           \
    } while (false)

const std::array<std::string, 6> noPortrait{};

// Everything a rejected command must leave untouched.
std::string fingerprint(const ArenaSnapshot& s)
{
    std::string k = std::to_string(s.roundsCompleted) + "|" + std::to_string(s.heroLevel) + "|" +
                    std::to_string(s.monstersDefeated) + "|" + std::to_string(s.battleOver);
    auto add = [&](const CombatantSnapshot& c) {
        k += "|" + c.name + "@" + std::to_string(c.position.row) + "," + std::to_string(c.position.col) + ":" +
             std::to_string(c.hitPoints) + "/" + std::to_string(c.maxHitPoints) + (c.alive ? "a" : "d") +
             (c.onBattlefield ? "b" : "-");
    };
    add(s.hero);
    for (const CombatantSnapshot& m : s.monsters)
        add(m);
    return k;
}

// The view must show exactly what the snapshot says.
void checkViewMatches(const ViewState& v, const ArenaSnapshot& s)
{
    CHECK(v.grid.cols == s.cols && v.grid.rows == s.rows);
    CHECK(v.roundsCompleted == s.roundsCompleted);
    CHECK(v.round == s.roundsCompleted + (s.battleOver ? 0 : 1));
    CHECK(v.heroLevel == s.heroLevel && v.heroTitle == s.heroTitle);
    CHECK(v.monstersDefeated == s.monstersDefeated && v.canFight == s.canFight);
    CHECK(v.battleOver == s.battleOver && v.heroWon == s.heroWon);
    CHECK(v.characters.size() == s.monsters.size() + 1);
    for (std::size_t i = 0; i < v.characters.size(); ++i)
    {
        const CombatantSnapshot& c = i == 0 ? s.hero : s.monsters[i - 1];
        const Character& shown = v.characters[i];
        CHECK(shown.id == c.id && shown.name == c.name);
        CHECK(shown.tile == sf::Vector2i(c.position.col, c.position.row));
        CHECK(shown.hp == c.hitPoints && shown.maxHp == c.maxHitPoints);
        CHECK(shown.alive == c.alive && shown.onBattlefield == c.onBattlefield);
    }
}

// Submits like the GUI does and checks the bookkeeping. Returns the TurnResult.
TurnResult play(Arena& arena, ViewState& v, const std::string& command)
{
    const ArenaSnapshot before = arena.getSnapshot();
    const std::vector<LogEntry> oldLog = v.log;
    const TurnResult result = arena.submitCommand(command);
    if (!result.accepted)
    {
        CHECK(fingerprint(arena.getSnapshot()) == fingerprint(before));  // nothing happened
        CHECK(result.messages.size() == 1);
        return result;
    }
    const int added = applyTurnResult(v, result);
    CHECK(added == static_cast<int>(result.messages.size()));
    CHECK(result.state.roundsCompleted == before.roundsCompleted + 1);
    checkViewMatches(v, result.state);
    CHECK(fingerprint(result.state) == fingerprint(arena.getSnapshot()));
    // Old entries kept as they were; new ones appended in order, tagged with the finished round.
    CHECK(v.log.size() == oldLog.size() + result.messages.size());
    for (std::size_t i = 0; i < oldLog.size() && i < v.log.size(); ++i)
        CHECK(v.log[i].text == oldLog[i].text && v.log[i].round == oldLog[i].round);
    for (std::size_t i = 0; i < result.messages.size(); ++i)
    {
        const LogEntry& e = v.log[oldLog.size() + i];
        CHECK(e.text == result.messages[i] && e.round == result.state.roundsCompleted);
    }
    return result;
}

ViewState freshView(const Arena& arena)
{
    ViewState v = makeViewState(arena.getSnapshot());
    v.log.push_back({v.round, "The battle begins!", LogKind::System});
    return v;
}

void testFirstTurn()
{
    Arena arena = createDefaultArena();
    ViewState v = freshView(arena);
    CHECK(v.round == 1 && v.characters[0].tile == sf::Vector2i(2, 2));
    CHECK(makeCommandPreviewView(arena.previewCommand("W"), v.characters[0].tile).ghostTile == sf::Vector2i(2, 1));

    const TurnResult r = play(arena, v, "W");
    CHECK(r.accepted);
    CHECK(r.state.hero.position == (Position{1, 2}));
    CHECK(!r.messages.empty() && r.messages.front() == "Bob moves to (1, 2).");
    CHECK(v.log.front().text == "The battle begins!" && v.log[1].round == 1);
    CHECK(v.round == (r.state.battleOver ? 1 : 2));
    // The next preview is computed from the new position.
    CHECK(makeCommandPreviewView(arena.previewCommand("S"), v.characters[0].tile).ghostTile == sf::Vector2i(2, 2));

    const TurnResult second = play(arena, v, "R");
    CHECK(second.accepted && second.messages.front() == "Bob rests.");
    CHECK(second.state.hero.position == r.state.hero.position);  // resting never moves the hero
    CHECK(v.log.back().round == 2);
}

void testRejectedCommands()
{
    Arena arena = createDefaultArena();
    ViewState v = freshView(arena);
    for (const char* bad : {"", "WWW", "F", "XYZ", "RW"})
    {
        const std::size_t logSize = v.log.size();
        const TurnResult r = play(arena, v, bad);
        CHECK(!r.accepted);
        CHECK(v.log.size() == logSize && v.round == 1);
    }
    // A valid move that ends where it started still plays a round.
    const TurnResult ws = play(arena, v, "WS");
    CHECK(ws.accepted && ws.state.roundsCompleted == 1 && ws.state.hero.position == (Position{2, 2}));

    // Walls and occupied squares, on a small dedicated arena (the default battle is untouched).
    std::vector<std::unique_ptr<Monster>> ms;
    ms.push_back(std::make_unique<Skeleton>("Bones", 16, 17, 6, 4, 4, 1, Position{1, 2}, noPortrait));
    ms.push_back(std::make_unique<Goblin>("Grub", 24, 4, 2, 1, 4, 2, Position{4, 4}, noPortrait));
    Arena edge(Hero("Bob", HeroTitle::Knight, 20, 18, 16, 14, 18, 2, Position{0, 2}, noPortrait), std::move(ms), 5, 5);
    ViewState ev = freshView(edge);
    CHECK(ev.canFight);
    for (const char* bad : {"W", "S", "SS"})  // off the board, onto Bones, through Bones
    {
        const TurnResult r = play(edge, ev, bad);
        CHECK(!r.accepted && r.messages.front() == "Movement blocked.");
        CHECK(ev.round == 1);
    }
    const TurnResult fight = play(edge, ev, "F");
    CHECK(fight.accepted && fight.messages.front() == "Bob holds position and fights.");
    CHECK(fight.state.hero.position == (Position{0, 2}));
}

// Plays the default battle to the end with a simple strategy, checking every round.
void testFullBattle()
{
    Arena arena = createDefaultArena();
    ViewState v = freshView(arena);
    int damage = 0, heals = 0, defeats = 0, levels = 0, specials = 0, monsterMoves = 0;
    for (int turn = 0; turn < 300 && !v.battleOver; ++turn)
    {
        std::string command = "R";
        if (v.canFight)
            command = "F";
        else
        {
            // Step toward the nearest living monster, using only moves the Arena accepts.
            int best = 1 << 30;
            for (const char* c : {"W", "A", "S", "D", "WW", "AA", "SS", "DD", "WA", "WD", "SA", "SD"})
            {
                const CommandPreview p = arena.previewCommand(c);
                if (!p.canSubmit)
                    continue;
                for (std::size_t i = 1; i < v.characters.size(); ++i)
                    if (v.characters[i].alive)
                    {
                        const int d = std::abs(v.characters[i].tile.x - p.destination.col) +
                                      std::abs(v.characters[i].tile.y - p.destination.row);
                        if (d < best)
                        {
                            best = d;
                            command = c;
                        }
                    }
            }
        }
        const ArenaSnapshot before = arena.getSnapshot();
        const TurnResult r = play(arena, v, command);
        CHECK(r.accepted);
        for (std::size_t i = 0; i < before.monsters.size(); ++i)
            monsterMoves += before.monsters[i].position != r.state.monsters[i].position;
        for (std::size_t i = v.log.size() - r.messages.size(); i < v.log.size(); ++i)
        {
            damage += v.log[i].kind == LogKind::Damage;
            heals += v.log[i].kind == LogKind::Heal;
            specials += v.log[i].kind == LogKind::Special;
            levels += v.log[i].text.find(" reaches level ") != std::string::npos;
            defeats += v.log[i].text.find(" is defeated!") != std::string::npos;
        }
        // A defeated monster stays listed, and its corpse stays on the battlefield.
        for (std::size_t i = 1; i < v.characters.size(); ++i)
            if (!v.characters[i].alive)
                CHECK(v.characters[i].onBattlefield && v.characters[i].hp <= 0);
    }
    CHECK(v.battleOver);
    CHECK(v.round == v.roundsCompleted);
    CHECK(v.log.back().kind == (v.heroWon ? LogKind::Victory : LogKind::Damage));
    std::printf("full battle: %s after %d rounds; monster moves %d, damage lines %d, heals %d, specials %d, "
                "defeats %d, level-ups %d, hero level %d\n",
                v.heroWon ? "victory" : "defeat", v.roundsCompleted, monsterMoves, damage, heals, specials, defeats,
                levels, v.heroLevel);
}

void checkEndedBattle(Arena& arena, ViewState& v, bool expectWin)
{
    CHECK(v.battleOver && v.heroWon == expectWin);
    CHECK(v.round == v.roundsCompleted);  // the final round, not a next one
    CHECK(!v.canFight);
    CHECK(v.log.back().kind == (expectWin ? LogKind::Victory : LogKind::Damage));
    for (const char* c : {"F", "R", "W", "D"})  // nothing more can happen
    {
        const std::size_t logSize = v.log.size();
        const TurnResult r = play(arena, v, c);
        CHECK(!r.accepted && r.messages.front() == "Battle has ended.");
        CHECK(v.log.size() == logSize);
    }
}

void testVictory()
{
    std::vector<std::unique_ptr<Monster>> ms;
    ms.push_back(std::make_unique<Goblin>("Grub", 1, 1, 1, 1, 1, 1, Position{1, 2}, noPortrait));
    Arena arena(Hero("Bob", HeroTitle::Knight, 20, 99, 99, 14, 18, 2, Position{0, 2}, noPortrait), std::move(ms), 5, 5);
    ViewState v = freshView(arena);
    for (int i = 0; i < 50 && !v.battleOver; ++i)
        CHECK(play(arena, v, "F").accepted);
    checkEndedBattle(arena, v, true);
    CHECK(v.monstersDefeated == 1 && !v.characters[1].alive && v.characters[1].onBattlefield);
    CHECK(v.log.back().text == "All monsters are defeated. Bob is victorious!");
}

void testDefeat()
{
    std::vector<std::unique_ptr<Monster>> ms;
    ms.push_back(std::make_unique<Ogre>("Brak", 40, 99, 99, 2, 8, 1, Position{1, 2}, noPortrait));
    Arena arena(Hero("Bob", HeroTitle::Knight, 1, 1, 1, 1, 0, 2, Position{0, 2}, noPortrait), std::move(ms), 5, 5);
    ViewState v = freshView(arena);
    for (int i = 0; i < 50 && !v.battleOver; ++i)
        CHECK(play(arena, v, "R").accepted);
    checkEndedBattle(arena, v, false);
    CHECK(!v.characters[0].alive);
    CHECK(v.log.back().text == "Bob has fallen. The battle is lost.");
}

void testClassification()
{
    CHECK(classifyLogMessage("Bob moves to (1, 2).", "Bob") == LogKind::Info);
    CHECK(classifyLogMessage("Grub attacks Bob.", "Bob") == LogKind::Info);
    CHECK(classifyLogMessage("Hit! Bob takes 3 damage (HP 17 / 20).", "Bob") == LogKind::Damage);
    CHECK(classifyLogMessage("Bob recovers 2 HP.", "Bob") == LogKind::Heal);
    CHECK(classifyLogMessage("Brak winds up a crushing smash...", "Bob") == LogKind::Special);
    CHECK(classifyLogMessage("The sting stabs home!", "Bob") == LogKind::Special);
    CHECK(classifyLogMessage("Bones is defeated!", "Bob") == LogKind::Victory);
    CHECK(classifyLogMessage("Bob is defeated!", "Bob") == LogKind::Damage);
    CHECK(classifyLogMessage("Bob reaches level 2!", "Bob") == LogKind::Victory);
    CHECK(classifyLogMessage("Something new happens.", "Bob") == LogKind::Info);
}
} // namespace

int main()
{
    testClassification();
    testFirstTurn();
    testRejectedCommands();
    testFullBattle();
    testVictory();
    testDefeat();
    std::printf("%s (%d failures)\n", failures == 0 ? "PASS" : "FAIL", failures);
    return failures == 0 ? 0 : 1;
}
