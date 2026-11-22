#pragma once

// Combat-log scrollbar geometry and scroll math, shared by the renderer (drawing)
// and main.cpp (hit testing / dragging) so both always agree on where the thumb is.
//
// Scroll position is measured in visual text rows, not source log entries:
// 0 shows the latest wrapped rows (thumb at the bottom), maxScroll shows the oldest
// (thumb at the top).

#include "Layout.hpp"

#include <algorithm>
#include <cmath>

namespace logscroll
{
inline constexpr float minThumbHeight = 18.f;

struct Geometry
{
    sf::FloatRect track;
    sf::FloatRect thumb;
    int visibleLines = 0;
    int totalLines = 0;
    int maxScroll = 0;

    bool enabled() const { return maxScroll > 0; }
};

inline int visibleLines()
{
    return static_cast<int>(layout::logArea.size.y / layout::logLineHeight);
}

inline int maxScroll(int totalLines)
{
    return std::max(0, totalLines - visibleLines());
}

inline int clampScroll(int scroll, int totalLines)
{
    return std::clamp(scroll, 0, maxScroll(totalLines));
}

inline sf::FloatRect trackRect()
{
    const sf::FloatRect& a = layout::logArea;
    return {{a.position.x + a.size.x - 14.f, a.position.y + 3.f}, {10.f, a.size.y - 6.f}};
}

inline Geometry geometry(int totalLines, int scroll)
{
    Geometry g;
    g.track = trackRect();
    g.visibleLines = visibleLines();
    g.totalLines = totalLines;
    g.maxScroll = maxScroll(totalLines);
    if (!g.enabled())
    {
        g.thumb = g.track;  // everything fits: thumb fills the track (drawn as disabled)
        return g;
    }
    const float visibleFrac = static_cast<float>(g.visibleLines) / static_cast<float>(totalLines);
    const float thumbH = std::clamp(g.track.size.y * visibleFrac, minThumbHeight, g.track.size.y);
    const float travel = g.track.size.y - thumbH;
    const float fromTop = 1.f - static_cast<float>(clampScroll(scroll, totalLines)) / static_cast<float>(g.maxScroll);
    g.thumb = {{g.track.position.x, g.track.position.y + travel * fromTop}, {g.track.size.x, thumbH}};
    return g;
}

// Scroll position for a desired thumb top edge (while dragging). Clamped to valid limits.
inline int scrollForThumbTop(const Geometry& g, float thumbTop)
{
    if (!g.enabled())
        return 0;
    const float travel = g.track.size.y - g.thumb.size.y;
    if (travel <= 0.f)
        return 0;
    const float fromTop = std::clamp((thumbTop - g.track.position.y) / travel, 0.f, 1.f);
    return static_cast<int>(std::lround((1.f - fromTop) * static_cast<float>(g.maxScroll)));
}

// Clicking the track above the thumb pages toward older entries; below pages toward newer ones.
inline int pageScroll(const Geometry& g, int scroll, float clickY)
{
    if (!g.enabled())
        return 0;
    const int page = g.visibleLines;
    if (clickY < g.thumb.position.y)
        scroll += page;
    else if (clickY >= g.thumb.position.y + g.thumb.size.y)
        scroll -= page;
    return std::clamp(scroll, 0, g.maxScroll);
}
} // namespace logscroll
