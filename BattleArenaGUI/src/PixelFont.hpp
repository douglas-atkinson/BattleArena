#pragma once

// A small original 5x7 bitmap font for headings, drawn as solid pixel quads so it
// stays perfectly crisp at any integer scale.

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/System/Vector2.hpp>

#include <string_view>

namespace pixelfont
{
struct Style
{
    float scale = 2.f;
    sf::Color top{232, 196, 120};     // rows 0-2
    sf::Color bottom{180, 132, 62};   // rows 3-6
    bool shadow = true;
};

float measure(std::string_view text, float scale);
inline float height(float scale) { return 7.f * scale; }

// Returns the drawn width.
float draw(sf::RenderTarget& target, std::string_view text, sf::Vector2f position, const Style& style);
} // namespace pixelfont
