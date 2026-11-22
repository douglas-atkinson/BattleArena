#include <array>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "battlefactory.h"
#include "hero.h"
#include "goblin.h"
#include "ogre.h"
#include "skeleton.h"
#include "bat.h"
#include "herotitle.h"

Arena createDefaultArena() {
    std::array<std::string, 6> portrait;

    Hero hero("Bob", HeroTitle::Knight, 20, 18, 16, 14, 18, 2, Position{ 2, 2 }, portrait);

    std::vector<std::unique_ptr<Monster>> monsters;

    monsters.push_back(std::make_unique<Goblin>("Grub", 24, 4, 2, 1, 4, 2, Position{ 5, 2 }, portrait));

    monsters.push_back(std::make_unique<Ogre>("Brak", 40, 20, 8, 2, 8, 1, Position{ 7, 4 }, portrait));

    monsters.push_back(std::make_unique<Skeleton>("Bones", 16, 17, 6, 4, 4, 1, Position{ 8, 8 }, portrait));

    monsters.push_back(std::make_unique<Bat>("Squeak", 6, 6, 6, 20, 2, 2, Position{ 1, 8 }, portrait));

    return Arena(std::move(hero), std::move(monsters), 10, 10);
}
