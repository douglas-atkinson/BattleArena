#include "hero.h"
#include "dummymonster.h"
#include "position.h"
#include "worldstate.h"
#include "attackresult.h"
#include "combatant.h"
#include <iostream>

char getHeroMoveDirection(Hero theHero, Monster& monster);
void displayAttackResult(AttackResult result, Combatant& combatant);

int main() {
    Hero hero("Bob", HeroTitle::Knight, 18, 16, 12, 14, 16, Position(2, 3), {}, '@');
    DummyMonster dummy("Dummy", 10, 14, 10, 11, 12, Position(8, 12), {}, '&');
    WorldState ws;
    ws.rows = 20;
    ws.cols = 20;
    ws.heroPosition = hero.getPosition();
    ws.monsterPositions.push_back(dummy.getPosition());

    std::cout << hero.speak() << std::endl;

    while (hero.getHitPoints() > 0 && dummy.getHitPoints() > 0) {
        char moveDirection = getHeroMoveDirection(hero, dummy);
        std::cout << std::endl;
        if (moveDirection != 'r' && moveDirection != 'R') {
            if (hero.moveByPlayerCommand(moveDirection, ws)) {
                ws.heroPosition = hero.getPosition();
            }
        }
        displayAttackResult(hero.attack(dummy), hero);
        std::cout << std::endl;

        dummy.move(ws);
        displayAttackResult(dummy.attack(hero), dummy);
        std::cout << std::endl;
    }

    std::cout << "Match over!\n";

	return 0;
}

char getHeroMoveDirection(Hero theHero, Monster& monster) {
    Position heroPos = theHero.getPosition();
    Position monsterPos = monster.getPosition();

    std::cout << "You are at (" << heroPos.col << ", " << heroPos.row << ")\n";
    std::cout << monster.getName() << " is at (" << monsterPos.col << ", " << monsterPos.row << ")\n";
    std::cout << "Enter direction to move:\n";
    std::cout << "W -> Go North\n"
              << "A -> Go West\n"
              << "S -> Go South\n"
              << "D -> Go East\n"
              << "R -> To not move\n";
    std::cout << "Enter choice: ";
    char choice;
    std::cin >> choice;
    std::cin.ignore();
    return choice;
}

void displayAttackResult(AttackResult result, Combatant& combatant) {
    std::cout << combatant.getName() << "'s Attack Result\n";
    std::cout << "Hit: " << (result.hit ? "Yes" : "No") << std::endl;
    std::cout << "Damage done: " << result.damage << std::endl;
    std::cout << "Opponent defeated: " << (result.targetDefeated ? "Yes" : "No") << std::endl;
}
