#include <iostream>
#include <memory>
#include <vector>

#include "arena.h"
#include "hero.h"
//#include "goblin.h"
//#include "ogre.h"
#include "dummymonster.h"

int main() {
    std::array<std::string, 6> portrait;

    Hero hero("Bob", HeroTitle::Knight, 20, 18, 16, 14, 18, Position{ 2, 2 }, portrait);

    std::vector<std::unique_ptr<Monster>> monsters;

    monsters.push_back(std::make_unique<DummyMonster>("Grub", 8, 4, 2, 1, 2, Position{ 5, 2 }, portrait));

    monsters.push_back(std::make_unique<DummyMonster>("Brak", 14, 6, 2, 3, 4, Position{ 7, 4 }, portrait));

    Arena arena(std::move(hero), std::move(monsters), 10, 10);
    arena.runBattle();

    return 0;
}

