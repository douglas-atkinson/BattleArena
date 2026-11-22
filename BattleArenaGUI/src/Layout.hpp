#pragma once

// Screen layout from the design specification (section 2). All values are in
// "design pixels" for a 1600x900 view; the window may be scaled or letterboxed.

#include <SFML/Graphics/Rect.hpp>

#include <optional>

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
inline constexpr int tileArt = 15;       // tile size in art pixels
inline constexpr float tileSize = 30.f;  // tile size on screen (tileArt * artScale)
// The battlefield art covers the whole panel: 550x260 art pixels, drawn at 2x.
inline constexpr int battlefieldArtW = 550;
inline constexpr int battlefieldArtH = 260;

// Geometry of a battlefield grid with the given number of columns and rows. The grid is
// centred in the battlefield panel (rounded down to whole art pixels), so the original
// 30x16 grid keeps its original origin. Rendering and hit testing both use this, via
//     x = origin().x + col * tileSize,  y = origin().y + row * tileSize
// Tiles are (col, row) == (x, y). The game model's Position is {row, col}: convert explicitly.
struct Grid
{
    int cols = 30;
    int rows = 16;

    // The largest grid is the original 30x16 design: 16 rows already fill the panel's
    // height, and wider grids would crowd the wall torches beside the frame.
    static constexpr int minCols = 3, maxCols = 30;
    static constexpr int minRows = 3, maxRows = 16;

    static constexpr bool supports(int c, int r)
    {
        return c >= minCols && c <= maxCols && r >= minRows && r <= maxRows;
    }

    // Top-left of the playable grid inside the battlefield art, in art pixels.
    sf::Vector2u artOrigin() const
    {
        return {static_cast<unsigned>((battlefieldArtW - cols * tileArt) / 2),
                static_cast<unsigned>((battlefieldArtH - rows * tileArt) / 2)};
    }

    // Top-left of the playable grid on screen.
    sf::Vector2f origin() const
    {
        const sf::Vector2u a = artOrigin();
        return {battlefield.position.x + static_cast<float>(a.x) * artScale,
                battlefield.position.y + static_cast<float>(a.y) * artScale};
    }

    sf::FloatRect bounds() const
    {
        return {origin(), {static_cast<float>(cols) * tileSize, static_cast<float>(rows) * tileSize}};
    }

    bool contains(sf::Vector2i tile) const
    {
        return tile.x >= 0 && tile.y >= 0 && tile.x < cols && tile.y < rows;
    }

    sf::FloatRect tileRect(sf::Vector2i tile) const
    {
        const sf::Vector2f o = origin();
        return {{o.x + static_cast<float>(tile.x) * tileSize, o.y + static_cast<float>(tile.y) * tileSize},
                {tileSize, tileSize}};
    }

    // The tile under a screen point, if the point is inside the grid.
    std::optional<sf::Vector2i> tileAt(sf::Vector2f point) const
    {
        const sf::Vector2f local = point - origin();
        if (local.x < 0.f || local.y < 0.f)
            return std::nullopt;
        const sf::Vector2i tile{static_cast<int>(local.x / tileSize), static_cast<int>(local.y / tileSize)};
        if (!contains(tile))
            return std::nullopt;
        return tile;
    }

    bool operator==(const Grid&) const = default;
};

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
