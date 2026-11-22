// Unit tests for the combat-log scrollbar math (src/LogScroll.hpp).
// Run: ctest --test-dir build -C Release   (or run LogScrollTests.exe directly)

#include "LogScroll.hpp"

#include <cmath>
#include <iostream>

namespace
{
int failures = 0;
int checks = 0;

void check(bool ok, const char* what, int line)
{
    ++checks;
    if (!ok)
    {
        ++failures;
        std::cerr << "FAIL (line " << line << "): " << what << "\n";
    }
}
#define CHECK(expr) check((expr), #expr, __LINE__)

bool near(float a, float b) { return std::fabs(a - b) < 0.01f; }

float top(const sf::FloatRect& r) { return r.position.y; }
float bottom(const sf::FloatRect& r) { return r.position.y + r.size.y; }
} // namespace

int main()
{
    using namespace logscroll;
    const int vis = visibleLines();
    CHECK(vis == 7);
    const sf::FloatRect track = trackRect();

    // --- Whole log fits: scrolling disabled -------------------------------------------------
    for (int total : {0, 1, 3, vis})
    {
        const Geometry g = geometry(total, 0);
        CHECK(!g.enabled());
        CHECK(g.maxScroll == 0);
        CHECK(clampScroll(5, total) == 0);
        CHECK(clampScroll(-3, total) == 0);
        CHECK(scrollForThumbTop(g, top(track) - 100.f) == 0);
        CHECK(pageScroll(g, 0, top(track) + 1.f) == 0);
    }

    // --- One line more than fits ---------------------------------------------------------------
    {
        const Geometry g = geometry(vis + 1, 0);
        CHECK(g.enabled());
        CHECK(g.maxScroll == 1);
        CHECK(scrollForThumbTop(g, -1000.f) == 1);
        CHECK(scrollForThumbTop(g, 1000.f) == 0);
    }

    // --- Sample log: 12 entries, 7 visible -> max scroll 5 ---------------------------------------
    {
        const int total = 12;
        const Geometry newest = geometry(total, 0);
        const Geometry oldest = geometry(total, 5);
        CHECK(newest.maxScroll == 5);
        // Proportional thumb: 7/12 of the track.
        CHECK(near(newest.thumb.size.y, track.size.y * 7.f / 12.f));
        // Scroll 0 => thumb at the bottom; max scroll => thumb at the top.
        CHECK(near(bottom(newest.thumb), bottom(track)));
        CHECK(near(top(oldest.thumb), top(track)));
        // Thumb always stays inside the track, even for out-of-range scroll values.
        for (int s : {-10, 0, 1, 2, 3, 4, 5, 99})
        {
            const Geometry g = geometry(total, s);
            CHECK(top(g.thumb) >= top(track) - 0.01f);
            CHECK(bottom(g.thumb) <= bottom(track) + 0.01f);
        }
        // Dragging: thumb position round-trips to the same scroll value.
        for (int s = 0; s <= 5; ++s)
            CHECK(scrollForThumbTop(newest, top(geometry(total, s).thumb)) == s);
        // Dragging past either end clamps.
        CHECK(scrollForThumbTop(newest, top(track) - 500.f) == 5);
        CHECK(scrollForThumbTop(newest, bottom(track) + 500.f) == 0);
        // Small drag movements snap to the nearest line.
        const float step = (track.size.y - newest.thumb.size.y) / 5.f;
        CHECK(scrollForThumbTop(newest, top(newest.thumb) - step * 0.4f) == 0);
        CHECK(scrollForThumbTop(newest, top(newest.thumb) - step * 0.6f) == 1);

        // Track paging: above the thumb pages older, below pages newer, clamped.
        CHECK(pageScroll(newest, 0, top(track) + 1.f) == 5);        // 0 + 7 clamps to 5
        CHECK(pageScroll(oldest, 5, bottom(track) - 1.f) == 0);     // 5 - 7 clamps to 0
        CHECK(pageScroll(newest, 0, bottom(track) - 1.f) == 0);     // already newest: no-op
        CHECK(pageScroll(oldest, 5, top(oldest.thumb) + 2.f) == 5); // on thumb: no paging
    }

    // --- Long log: paging moves a full page, thumb respects minimum height ------------------------
    {
        const int total = 500;
        const Geometry g = geometry(total, 0);
        CHECK(g.maxScroll == 493);
        CHECK(near(g.thumb.size.y, minThumbHeight));
        CHECK(pageScroll(g, 0, top(track) + 1.f) == vis);
        const Geometry mid = geometry(total, 100);
        CHECK(pageScroll(mid, 100, top(track) + 1.f) == 107);
        CHECK(pageScroll(mid, 100, bottom(track) - 1.f) == 93);
        for (int s = 0; s <= g.maxScroll; s += 17)
            CHECK(scrollForThumbTop(g, top(geometry(total, s).thumb)) == s);
        CHECK(scrollForThumbTop(g, top(geometry(total, g.maxScroll).thumb)) == g.maxScroll);
    }

    std::cout << (checks - failures) << "/" << checks << " checks passed\n";
    return failures == 0 ? 0 : 1;
}
