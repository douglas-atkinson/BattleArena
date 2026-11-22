#pragma once

// Original pixel art for the BattleArena mockup, authored as text grids in code.
// Each character in a row maps to a palette colour; '.' is transparent.
// Rows may be written at full width, or as a left half that is mirrored
// (handy for symmetric faces and sprites).

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Image.hpp>

#include <map>
#include <string>
#include <vector>

namespace art
{
enum class Kind { Knight = 0, Goblin, Ogre, Skeleton, Bat };
inline constexpr int kindCount = 5;

struct PixelDef
{
    unsigned width = 0;
    unsigned height = 0;
    std::map<char, sf::Color> palette;
    std::vector<std::string> rows;
};

sf::Image buildImage(const PixelDef& def, const char* debugName);
sf::Image toGrayscale(const sf::Image& src, float brightness);

sf::Image buildSprite(Kind kind);      // 15x15 battlefield sprite
sf::Image buildPortrait(Kind kind);    // 28x28 portrait with dithered backdrop
sf::Image buildBonePile();             // 15x15 remains of a defeated skeleton
sf::Image buildSkull();                // 11x9 "defeated" overlay
sf::Image buildPanelNoise(unsigned size, sf::Color base, unsigned seed);

struct BattlefieldSpec
{
    sf::Vector2u panelSize{550, 260};  // art pixels (drawn at 2x => 1100x520)
    sf::Vector2u gridOrigin{50, 10};
    unsigned cols = 30;
    unsigned rows = 16;
    unsigned tile = 15;
    unsigned frame = 6;
    unsigned seed = 1992;
};

sf::Image buildBattlefield(const BattlefieldSpec& spec);
} // namespace art
