#include <iostream>
#include <memory>
#include <vector>
#include <array>

#include "arena.h"
#include "hero.h"
#include "goblin.h"
#include "ogre.h"
#include "herotitle.h"

int main() {
    std::array<std::string, 6> portrait;

    Hero hero("Bob", HeroTitle::Knight, 20, 18, 16, 14, 18, 2, Position{ 2, 2 }, portrait);

    std::vector<std::unique_ptr<Monster>> monsters;

    monsters.push_back(std::make_unique<Goblin>("Grub", 24, 4, 2, 1, 4, 2, Position{ 5, 2 }, portrait));

    monsters.push_back(std::make_unique<Ogre>("Brak", 40, 20, 8, 2, 8, 1, Position{ 7, 4 }, portrait));

    Arena arena(std::move(hero), std::move(monsters), 10, 10);
    arena.runBattle();

    return 0;
}

