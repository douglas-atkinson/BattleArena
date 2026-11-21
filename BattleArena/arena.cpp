#include <vector>
#include <memory>
#include <cmath>
#include <stdexcept>
#include <set>
#include <utility>
#include <string>
#include "arena.h"
#include "hero.h"
#include "monster.h"
#include "position.h"
#include "random.h"

namespace {
    const int MoveRecoveryChancePercent = 25;   // successful move: 1 HP
    const int RestRecoveryChancePercent = 65;   // rest: 1-3 HP
    const int MinRestRecovery = 1;
    const int MaxRestRecovery = 3;

    // Explains an InvalidCommand result. Hero::validateMoveCommand() checks length
    // before characters, so an over-long command was rejected for exceeding speed.
    std::string describeInvalidCommand(const std::string& command, int speed) {
        if (command.empty()) {
            return "Enter a command: W/A/S/D to move, R to rest.";
        }
        if (command.size() > static_cast<std::size_t>(speed)) {
            return "Movement command exceeds hero speed (maximum " + std::to_string(speed) + ").";
        }
        return "Invalid command. Use W/A/S/D moves, R to rest, or F to fight.";
    }

    bool isFightCommand(const std::string& command) {
        return command == "f" || command == "F";
    }

    std::string formatPosition(const Position& pos) {
        return "(" + std::to_string(pos.row) + ", " + std::to_string(pos.col) + ")";
    }

    // Turns one attack into log lines: who attacked, any special-attack messages,
    // the hit or miss with damage and remaining HP, and a defeat.
    void appendAttackMessages(const Combatant& attacker, const Combatant& target,
        const AttackResult& result, std::vector<std::string>& messages) {
        messages.push_back(attacker.getName() + " attacks " + target.getName() + ".");
        for (const std::string& message : result.messages) {
            if (!message.empty()) {
                messages.push_back(message);
            }
        }
        if (result.hit) {
            messages.push_back("Hit! " + target.getName() + " takes " + std::to_string(result.damage)
                + " damage (HP " + std::to_string(target.getHitPoints()) + " / "
                + std::to_string(target.getMaxHitPoints()) + ").");
        }
        else {
            messages.push_back(attacker.getName() + " misses.");
        }
        if (result.targetDefeated) {
            messages.push_back(target.getName() + " is defeated!");
        }
    }

    CombatantSnapshot makeSnapshot(const Combatant& combatant, int id,
        const std::string& type, bool onBattlefield) {
        return CombatantSnapshot{
            id,
            combatant.getName(),
            type,
            combatant.getSymbol(),
            combatant.getPosition(),
            combatant.getHitPoints(),
            combatant.getMaxHitPoints(),
            combatant.getStrength(),
            combatant.getAccuracy(),
            combatant.getAgility(),
            combatant.getDefense(),
            combatant.getSpeed(),
            combatant.isAlive(),
            onBattlefield
        };
    }
}

Arena::Arena(Hero hero, std::vector<std::unique_ptr<Monster>> monsters, int rows, int cols)
    : hero(std::move(hero)), monsters(std::move(monsters)), rows(rows), cols(cols) {
    validateStartState();
    assignSymbols();
}

WorldState Arena::buildWorldStateForHero() const {
    std::vector<Position> monsterPositions;

    // Defeated monsters stay on the battlefield as corpses and still block their squares.
    for (const auto& monsterPtr : monsters) {
        if (monsterPtr->isOnBattlefield()) {
            monsterPositions.push_back(monsterPtr->getPosition());
        }
    }

    return WorldState{ rows, cols, hero.getPosition(), monsterPositions };
}

WorldState Arena::buildWorldStateForMonster(const Monster* activeMonster) const {
    std::vector<Position> monsterPositions;

    for (const auto& monsterPtr : monsters) {
        if (monsterPtr.get() != activeMonster && monsterPtr->isOnBattlefield()) {
            monsterPositions.push_back(monsterPtr->getPosition());
        }
    }

    return WorldState{ rows, cols, hero.getPosition(), monsterPositions };
}

bool Arena::isBattleOver() const {
    return !hero.isAlive() || !hasLivingMonsters();
}

bool Arena::heroWon() const {
    return hero.isAlive() && !hasLivingMonsters();
}

// Defeated monsters stay in the collection, so victory depends on living monsters only.
bool Arena::hasLivingMonsters() const {
    for (const auto& monsterPtr : monsters) {
        if (monsterPtr->isAlive()) {
            return true;
        }
    }
    return false;
}

// The single Fight rule, shared by snapshots, previews, and submissions.
bool Arena::canHeroFight() const {
    return !isBattleOver() && findAdjacentMonsterToHero() != nullptr;
}

ArenaSnapshot Arena::getSnapshot() const {
    ArenaSnapshot snapshot{};
    snapshot.rows = rows;
    snapshot.cols = cols;
    snapshot.hero = makeSnapshot(hero, 0, "Hero", true);
    snapshot.heroLevel = hero.getLevel();
    snapshot.monstersDefeated = 0;

    // Monsters are never removed or reordered, so index + 1 is a stable ID.
    for (std::size_t i = 0; i < monsters.size(); ++i) {
        const Monster& monster = *monsters[i];
        snapshot.monsters.push_back(makeSnapshot(monster, static_cast<int>(i) + 1,
            monster.getTypeName(), monster.isOnBattlefield()));
        if (!monster.isAlive()) {
            ++snapshot.monstersDefeated;
        }
    }

    snapshot.battleOver = isBattleOver();
    snapshot.heroWon = heroWon();
    snapshot.roundsCompleted = roundsCompleted;
    snapshot.heroTitle = hero.getTitleString();
    snapshot.canFight = canHeroFight();
    return snapshot;
}

std::string Arena::describeValidation(const MovementValidation& validation,
    const std::string& command) const {
    switch (validation.status) {
    case MovementResult::Success:
        return validation.isRest ? "Rest." : "Move available.";
    case MovementResult::Blocked:
        return "Movement blocked.";
    case MovementResult::InvalidCommand:
        break;
    }
    return describeInvalidCommand(command, hero.getSpeed());
}

// Fight is Arena's rule (it needs an adjacent living monster); moves and rest use
// Hero's shared movement validator. submitCommand() relies on this same check.
CommandPreview Arena::previewCommand(const std::string& command) const {
    if (isBattleOver()) {
        return CommandPreview{ MovementResult::InvalidCommand, hero.getPosition(),
            "Battle has ended.", false, PlayerAction::Move };
    }

    if (isFightCommand(command)) {
        if (canHeroFight()) {
            return CommandPreview{ MovementResult::Success, hero.getPosition(),
                "Fight.", true, PlayerAction::Fight };
        }
        return CommandPreview{ MovementResult::InvalidCommand, hero.getPosition(),
            "No living monster is adjacent to fight.", false, PlayerAction::Fight };
    }

    MovementValidation validation = hero.validateMoveCommand(command, buildWorldStateForHero());

    return CommandPreview{ validation.status, validation.destination,
        describeValidation(validation, command),
        validation.status == MovementResult::Success,
        validation.isRest ? PlayerAction::Rest : PlayerAction::Move };
}

// One complete round: the hero's action (a move with its automatic attack, a rest,
// or a fight), then (if the battle continues) each living monster's turn, then
// peaceful-round healing. Rejected commands change nothing.
TurnResult Arena::submitCommand(const std::string& command) {
    // Validate independently of any earlier preview, with the same rules preview uses.
    CommandPreview check = previewCommand(command);
    if (!check.canSubmit) {
        return TurnResult{ false, getSnapshot(), { check.message } };
    }

    std::vector<std::string> messages;
    RecoveryChance recovery = RecoveryChance::None;

    switch (check.action) {
    case PlayerAction::Move:
        // Just validated against the current state, so this always succeeds.
        hero.moveByPlayerCommand(command, buildWorldStateForHero());
        messages.push_back(hero.getName() + " moves to " + formatPosition(hero.getPosition()) + ".");
        recovery = RecoveryChance::Moved;
        if (processHeroAttack(messages)) {
            recovery = RecoveryChance::None;  // attacking ends any chance to heal this round
        }
        break;
    case PlayerAction::Rest:
        messages.push_back(hero.getName() + " rests.");  // resting never attacks
        recovery = RecoveryChance::Rested;
        break;
    case PlayerAction::Fight:
        messages.push_back(hero.getName() + " holds position and fights.");
        processHeroAttack(messages);  // fighting never earns a healing chance
        break;
    }

    // A hero victory ends the battle at once: no monster turns, no healing.
    if (!isBattleOver()) {
        bool heroWasAttacked = false;

        for (auto& monsterPtr : monsters) {
            if (!monsterPtr->isAlive()) continue;

            if (processMonsterTurn(*monsterPtr, messages)) {
                heroWasAttacked = true;
            }

            if (!hero.isAlive()) break;
        }

        // Healing happens only in a peaceful round: no monster attacked the hero (hit
        // or miss), and the chance was already cleared above if the hero attacked.
        if (!heroWasAttacked) {
            tryHeroRecovery(recovery, messages);
        }
    }

    ++roundsCompleted;

    if (heroWon()) {
        messages.push_back("All monsters are defeated. " + hero.getName() + " is victorious!");
    }
    else if (!hero.isAlive()) {
        messages.push_back(hero.getName() + " has fallen. The battle is lost.");
    }

    return TurnResult{ true, getSnapshot(), messages };
}

// Resolves the hero's automatic attack on an adjacent living monster, if any.
// Returns true if the hero attacked (hit or miss).
bool Arena::processHeroAttack(std::vector<std::string>& messages) {
    Monster* target = findAdjacentMonsterToHero();
    if (target == nullptr || !hero.isAlive() || !target->isAlive()) {
        return false;
    }

    AttackResult result = hero.attack(*target);
    appendAttackMessages(hero, *target, result, messages);

    if (result.targetDefeated) {
        int levelBefore = hero.getLevel();
        int maxHitPointsBefore = hero.getMaxHitPoints();
        hero.recordVictory();

        if (hero.getLevel() > levelBefore) {
            messages.push_back(hero.getName() + " reaches level " + std::to_string(hero.getLevel()) + "!");
        }
        if (hero.getMaxHitPoints() > maxHitPointsBefore) {
            messages.push_back(hero.getName() + "'s maximum HP rises to "
                + std::to_string(hero.getMaxHitPoints()) + ".");
        }
    }

    return true;
}

// Moves the monster, then attacks if it ends adjacent to the hero.
// Returns true if the monster attacked the hero (hit or miss).
bool Arena::processMonsterTurn(Monster& monster, std::vector<std::string>& messages) {
    if (!monster.isAlive() || !hero.isAlive()) return false;

    WorldState world = buildWorldStateForMonster(&monster);
    monster.move(world);

    if (hero.isAlive() && areOrthogonallyAdjacent(monster.getPosition(), hero.getPosition())) {
        AttackResult result = monster.attack(hero);
        appendAttackMessages(monster, hero, result, messages);
        return true;
    }

    return false;
}

void Arena::tryHeroRecovery(RecoveryChance recovery, std::vector<std::string>& messages) {
    int amount = 0;

    switch (recovery) {
    case RecoveryChance::Moved:
        if (Random::getUniformInt(1, 100) <= MoveRecoveryChancePercent) {
            amount = 1;
        }
        break;
    case RecoveryChance::Rested:
        if (Random::getUniformInt(1, 100) <= RestRecoveryChancePercent) {
            amount = Random::getUniformInt(MinRestRecovery, MaxRestRecovery);
        }
        break;
    case RecoveryChance::None:
        break;
    }

    int restored = hero.recoverHitPoints(amount);
    if (restored > 0) {
        messages.push_back(hero.getName() + " recovers " + std::to_string(restored) + " HP.");
    }
}

Monster* Arena::findAdjacentMonsterToHero() const {
    for (const auto& monsterPtr : monsters) {
        if (monsterPtr->isAlive() &&
            areOrthogonallyAdjacent(hero.getPosition(), monsterPtr->getPosition())) {
            return monsterPtr.get();
        }
    }

    return nullptr;
}

void Arena::assignSymbols() {
    std::vector<char> pool = {
        '@', 'A', 'B', 'C', 'D', 'E', 'F',
        'G', 'H', 'I', 'J', 'K', 'L'
    };

    std::size_t index = 0;

    hero.setSymbol(pool[index++]);

    for (auto& monster : monsters) {
        if (index < pool.size()) {
            monster->setSymbol(pool[index++]);
        }
        else {
            monster->setSymbol('?'); // fallback
        }
    }
}

void Arena::validateStartState() const {
    validateArenaDimensions();
    validateHeroPosition();
    validateNoNullMonsters();
    validateMonsterPositions();
    validateNoOverlappingCombatants();
}

void Arena::validateArenaDimensions() const {
    if (rows <= 0 || cols <= 0) {
        throw std::invalid_argument("Arena dimensions must be greater than 0.");
    }
}

void Arena::validateHeroPosition() const {
    const Position pos = hero.getPosition();
    if (!inBounds(pos.row, pos.col)) {
        throw std::invalid_argument("Hero's position must be within Arena boundaries");
    }
}

void Arena::validateNoNullMonsters() const {
    for (const auto& monster : monsters) {
        if (monster == nullptr) {
            throw std::invalid_argument("Pointer to Monster cannot be null");
        }
    }
}

void Arena::validateMonsterPositions() const {
    for (const auto& monster : monsters) {
        const Position pos = monster->getPosition();
        if (!inBounds(pos.row, pos.col)) {
            throw std::invalid_argument("Monster's position must be within Arena boundaries");
        }
    }

}

void Arena::validateNoOverlappingCombatants() const {
    std::set<std::pair<int, int>> occupied;

    const Position heroPos = hero.getPosition();
    occupied.insert({ heroPos.row, heroPos.col });

    for (const auto& monster : monsters) {
        const Position pos = monster->getPosition();

        if (!occupied.insert({ pos.row, pos.col }).second) {
            throw std::invalid_argument("Two combatants cannot occupy the same starting position.");
        }
    }
}

bool Arena::inBounds(int row, int col) const {
    return (row >= 0 && row < rows && col >= 0 && col < cols);
}
