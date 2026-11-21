#pragma once

// Screen layout from the design specification (section 2). All values are in
// "design pixels" for a 1600x900 view; the window may be scaled or letterboxed.

#include <SFML/Graphics/Rect.hpp>

namespace layout
{
inline constexpr float viewW = 1600.f;
inline constexpr float viewH = 900.f;

inline const sf::FloatRect titleBar{{20.f, 16.f}, {1560.f, 52.f}};
inline const sf::FloatRect battlefield{{20.f, 80.f}, {1100.f, 520.f}};
inline const sf::FloatRect heroPanel{{1132.f, 80.f}, {448.f, 124.f}};
inline const sf::FloatRect rosterPanel{{1132.f, 216.f}, {448.f, 174.f}};
inline const sf::FloatRect logPanel{{1132.f, 402.f}, {448.f, 198.f}};
inline const sf::FloatRect commandBar{{20.f, 612.f}, {1560.f, 52.f}};
inline const sf::FloatRect portraitHud{{20.f, 676.f}, {1560.f, 188.f}};
inline constexpr float statusLineY = 870.f;

// Battlefield art is authored at half resolution and drawn at 2x (chunky pixels).
inline constexpr float artScale = 2.f;
inline constexpr int gridCols = 30;
inline constexpr int gridRows = 16;
inline constexpr unsigned tileArt = 15;  // tile size in art pixels
inline constexpr float tileSize = 30.f;  // tile size on screen
// Grid is centred in the battlefield panel: (1100-900)/2, (520-480)/2.
inline const sf::Vector2f gridOrigin{battlefield.position.x + 100.f, battlefield.position.y + 20.f};

// Portrait HUD cards.
inline constexpr float cardW = 296.f;
inline constexpr float cardH = 156.f;
inline constexpr float cardGap = 12.f;
inline constexpr float cardMargin = 16.f;

inline sf::FloatRect card(int index)
{
    return {{portraitHud.position.x + cardMargin + static_cast<float>(index) * (cardW + cardGap),
             portraitHud.position.y + cardMargin},
            {cardW, cardH}};
}

inline sf::FloatRect tileRect(int col, int row)
{
    return {{gridOrigin.x + static_cast<float>(col) * tileSize, gridOrigin.y + static_cast<float>(row) * tileSize},
            {tileSize, tileSize}};
}

// Monster roster rows inside the roster panel.
inline sf::FloatRect rosterRow(int index)
{
    return {{rosterPanel.position.x + 14.f, rosterPanel.position.y + 40.f + static_cast<float>(index) * 32.f},
            {rosterPanel.size.x - 28.f, 30.f}};
}

// Scrollable text area of the combat log.
inline const sf::FloatRect logArea{{logPanel.position.x + 14.f, logPanel.position.y + 38.f},
                                   {logPanel.size.x - 28.f, 146.f}};
inline constexpr float logLineHeight = 20.f;
} // namespace layout
