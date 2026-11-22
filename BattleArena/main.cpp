#include <iostream>
#include <vector>
#include <string>
#include <cctype>

#include "arena.h"
#include "arenatypes.h"
#include "battlefactory.h"

// A thin console presentation. It talks to the game only through Arena's public
// controller API: getSnapshot() and submitCommand(). It never inspects Hero or Monster.
namespace {
    // A corpse shows its symbol in lowercase.
    char displaySymbol(const CombatantSnapshot& combatant) {
        if (combatant.alive) {
            return combatant.symbol;
        }
        return static_cast<char>(std::tolower(static_cast<unsigned char>(combatant.symbol)));
    }

    void printBattlefield(const ArenaSnapshot& state) {
        std::vector<std::string> grid(state.rows, std::string(state.cols, '.'));

        for (const CombatantSnapshot& monster : state.monsters) {
            if (monster.onBattlefield) {
                grid[monster.position.row][monster.position.col] = displaySymbol(monster);
            }
        }
        grid[state.hero.position.row][state.hero.position.col] = displaySymbol(state.hero);

        for (const std::string& row : grid) {
            for (char square : row) {
                std::cout << square << ' ';
            }
            std::cout << '\n';
        }
        std::cout << '\n';
    }

    void printStatus(const ArenaSnapshot& state) {
        const CombatantSnapshot& hero = state.hero;

        std::cout << "\n========================================\n";
        std::cout << "Round " << state.roundsCompleted + (state.battleOver ? 0 : 1)
            << "   Monsters defeated: " << state.monstersDefeated << " / " << state.monsters.size() << '\n';
        std::cout << "========================================\n";
        std::cout << hero.symbol << "  " << state.heroTitle << ' ' << hero.name
            << "   HP " << hero.hitPoints << " / " << hero.maxHitPoints
            << "   Level " << state.heroLevel
            << "   (" << hero.position.row << ", " << hero.position.col << ")"
            << (hero.alive ? "" : "   DEAD") << '\n';

        for (const CombatantSnapshot& monster : state.monsters) {
            std::cout << displaySymbol(monster) << "  " << monster.name << " (" << monster.type << ")";
            if (monster.alive) {
                std::cout << "   HP " << monster.hitPoints << " / " << monster.maxHitPoints;
            }
            else {
                std::cout << "   defeated";
            }
            std::cout << "   (" << monster.position.row << ", " << monster.position.col << ")\n";
        }
        std::cout << '\n';

        printBattlefield(state);
    }

    void printMessages(const std::vector<std::string>& messages) {
        for (const std::string& message : messages) {
            std::cout << "  " << message << '\n';
        }
    }

    void runConsoleBattle(Arena& arena) {
        ArenaSnapshot state = arena.getSnapshot();
        printStatus(state);

        while (!state.battleOver) {
            std::cout << "Enter command (W/A/S/D, up to " << state.hero.speed << " moves; R to rest"
                << (state.canFight ? "; F to fight" : "") << "): ";

            std::string command;
            if (!std::getline(std::cin, command)) {
                std::cout << "\nInput ended. Battle abandoned.\n";
                return;
            }

            TurnResult result = arena.submitCommand(command);
            printMessages(result.messages);

            // A rejected command leaves the round unchanged; just prompt again.
            if (result.accepted) {
                state = result.state;
                printStatus(state);
            }
        }

        std::cout << (state.heroWon ? "Victory!\n" : "Game Over.\n");
    }
}

int main() {
    Arena arena = createDefaultArena();
    runConsoleBattle(arena);

    return 0;
}
