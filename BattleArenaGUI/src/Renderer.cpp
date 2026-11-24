#include "Renderer.hpp"

#include "DefeatCutscene.hpp"
#include "VictoryCutscene.hpp"
#include "Layout.hpp"
#include "LogScroll.hpp"
#include "PixelFont.hpp"

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Text.hpp>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

namespace
{
using C = sf::Color;
using V = sf::Vector2f;
using R = sf::FloatRect;

// Palette from the design specification (section 7), plus a few derived shades.
namespace pal
{
const C background(27, 27, 28);
const C ink(10, 9, 8);
const C wood(84, 67, 51);
const C woodLight(122, 98, 70);
const C woodDark(50, 39, 29);
const C text(226, 208, 168);
const C textDim(150, 138, 112);
const C textFaint(104, 96, 80);
const C gold(196, 150, 80);
const C goldBright(236, 200, 120);
const C red(182, 66, 55);
const C green(108, 157, 98);
const C yellow(218, 179, 61);
const C steel(106, 158, 192);
const C special(222, 128, 60);
} // namespace pal

C lighten(C c, int d)
{
    auto f = [d](std::uint8_t v) { return static_cast<std::uint8_t>(std::clamp(v + d, 0, 255)); };
    return C(f(c.r), f(c.g), f(c.b), c.a);
}

C withAlpha(C c, std::uint8_t a)
{
    c.a = a;
    return c;
}

R shrink(R r, float by)
{
    return {{r.position.x + by, r.position.y + by}, {r.size.x - 2.f * by, r.size.y - 2.f * by}};
}

void fillRect(sf::RenderTarget& t, R r, C c)
{
    sf::RectangleShape s(r.size);
    s.setPosition(r.position);
    s.setFillColor(c);
    t.draw(s);
}

void outlineRect(sf::RenderTarget& t, R r, C c, float thickness)
{
    sf::RectangleShape s({r.size.x - 2.f * thickness, r.size.y - 2.f * thickness});
    s.setPosition({r.position.x + thickness, r.position.y + thickness});
    s.setFillColor(C::Transparent);
    s.setOutlineColor(c);
    s.setOutlineThickness(thickness);
    t.draw(s);
}

void drawTiled(sf::RenderTarget& t, const sf::Texture& tex, R r, float scale)
{
    sf::Sprite s(tex);
    s.setTextureRect(sf::IntRect({0, 0}, {static_cast<int>(std::ceil(r.size.x / scale)), static_cast<int>(std::ceil(r.size.y / scale))}));
    s.setScale({scale, scale});
    s.setPosition(r.position);
    t.draw(s);
}

void drawImage(sf::RenderTarget& t, const sf::Texture& tex, V pos, float scale, C tint = C::White)
{
    sf::Sprite s(tex);
    s.setScale({scale, scale});
    s.setPosition({std::round(pos.x), std::round(pos.y)});
    s.setColor(tint);
    t.draw(s);
}

float textWidth(const sf::Font& font, const std::string& str, unsigned size)
{
    sf::Text text(font, str, size);
    return text.getLocalBounds().size.x;
}

// Readable body text (Tuffy) with a 1px drop shadow. Returns the advance width.
float drawText(sf::RenderTarget& t, const sf::Font& font, const std::string& str, V pos, unsigned size, C color,
               bool shadow = true)
{
    sf::Text text(font, str, size);
    if (shadow)
    {
        text.setFillColor(C(0, 0, 0, 200));
        text.setPosition({std::round(pos.x) + 1.f, std::round(pos.y) + 1.f});
        t.draw(text);
    }
    text.setFillColor(color);
    text.setPosition({std::round(pos.x), std::round(pos.y)});
    t.draw(text);
    const sf::FloatRect bounds = text.getLocalBounds();
    return bounds.position.x + bounds.size.x;
}

pixelfont::Style heading(float scale = 2.f)
{
    return {scale, pal::goldBright, pal::gold, true};
}

pixelfont::Style parchment(float scale = 2.f)
{
    return {scale, C(240, 226, 192), pal::text, true};
}

// Wood-and-iron framed panel with a textured stone interior.
void drawFrame(sf::RenderTarget& t, const Assets& a, R r, bool fillInterior = true)
{
    fillRect(t, r, pal::ink);                                 // 2px outer black
    const R wood = shrink(r, 2.f);
    fillRect(t, wood, pal::wood);                             // 5px wood band
    fillRect(t, {wood.position, {wood.size.x, 1.f}}, pal::woodLight);
    fillRect(t, {wood.position, {1.f, wood.size.y}}, pal::woodLight);
    fillRect(t, {{wood.position.x, wood.position.y + wood.size.y - 1.f}, {wood.size.x, 1.f}}, pal::woodDark);
    fillRect(t, {{wood.position.x + wood.size.x - 1.f, wood.position.y}, {1.f, wood.size.y}}, pal::woodDark);
    const R goldLine = shrink(r, 7.f);
    outlineRect(t, goldLine, C(150, 112, 58), 1.f);           // tarnished gold inlay
    outlineRect(t, shrink(r, 8.f), pal::ink, 1.f);
    if (fillInterior)
        drawTiled(t, a.panelNoise, shrink(r, 9.f), 2.f);

    // Iron rivets in the corners of the wood band.
    const float rx[2] = {r.position.x + 3.f, r.position.x + r.size.x - 6.f};
    const float ry[2] = {r.position.y + 3.f, r.position.y + r.size.y - 6.f};
    for (float x : rx)
        for (float y : ry)
        {
            fillRect(t, {{x, y}, {3.f, 3.f}}, C(40, 36, 32));
            fillRect(t, {{x, y}, {2.f, 2.f}}, C(170, 160, 140));
            fillRect(t, {{x, y}, {1.f, 1.f}}, C(230, 222, 200));
        }
}

// Recessed well (dark with a light lower-right lip).
void drawInset(sf::RenderTarget& t, R r, C fill = C(18, 17, 16))
{
    fillRect(t, r, C(70, 64, 54));
    fillRect(t, {r.position, {r.size.x - 1.f, r.size.y - 1.f}}, C(6, 6, 6));
    fillRect(t, {{r.position.x + 1.f, r.position.y + 1.f}, {r.size.x - 2.f, r.size.y - 2.f}}, fill);
}

void drawBar(sf::RenderTarget& t, R r, float frac, C fill)
{
    drawInset(t, r, C(30, 22, 20));
    const R inner = shrink(r, 2.f);
    const float w = std::round(inner.size.x * std::clamp(frac, 0.f, 1.f));
    if (w > 0.f)
    {
        fillRect(t, {inner.position, {w, inner.size.y}}, fill);
        fillRect(t, {inner.position, {w, std::max(1.f, std::floor(inner.size.y / 4.f))}}, lighten(fill, 48));
        fillRect(t, {{inner.position.x, inner.position.y + inner.size.y - 2.f}, {w, 2.f}}, lighten(fill, -50));
    }
    for (float x = inner.position.x + 8.f; x < inner.position.x + inner.size.x; x += 8.f)
        fillRect(t, {{x, inner.position.y}, {1.f, inner.size.y}}, C(0, 0, 0, 80));
}

// One color policy for every health bar. Values outside the expected range are
// clamped by drawBar; invalid maximum HP is treated as an empty bar.
void drawHealthBar(sf::RenderTarget& t, R r, int hp, int maxHp)
{
    const float fraction = maxHp > 0 ? static_cast<float>(hp) / static_cast<float>(maxHp) : 0.f;
    const C fill = fraction <= 0.25f ? pal::red : (fraction <= 0.75f ? pal::yellow : pal::green);
    drawBar(t, r, fraction, fill);
}

void drawDiamond(sf::RenderTarget& t, V center, float px, C color)
{
    // 5x5 pixel diamond made of rows 1,3,5,3,1.
    const int widths[5] = {1, 3, 5, 3, 1};
    for (int i = 0; i < 5; ++i)
    {
        const float w = static_cast<float>(widths[i]) * px;
        fillRect(t, {{center.x - w / 2.f, center.y + (static_cast<float>(i) - 2.5f) * px}, {w, px}}, color);
    }
}

// A muted key cap marks a command that is currently unavailable (same size, darker and faded).
void drawKeyCap(sf::RenderTarget& t, const sf::Font& font, const std::string& label, V pos, float& outWidth,
                bool muted = false)
{
    const float w = std::max(24.f, textWidth(font, label, 15) + 14.f);
    const R r{pos, {w, 24.f}};
    fillRect(t, r, C(12, 11, 10));
    fillRect(t, shrink(r, 1.f), muted ? C(38, 35, 31) : C(68, 60, 50));
    fillRect(t, {{r.position.x + 1.f, r.position.y + 1.f}, {w - 2.f, 2.f}}, muted ? C(58, 53, 46) : C(118, 104, 84));
    fillRect(t, {{r.position.x + 1.f, r.position.y + r.size.y - 4.f}, {w - 2.f, 3.f}}, muted ? C(24, 22, 19) : C(36, 32, 26));
    drawText(t, font, label, {pos.x + (w - textWidth(font, label, 15)) / 2.f, pos.y + 2.f}, 15,
             muted ? pal::textFaint : pal::text);
    outWidth = w;
}

const sf::Texture& spriteFor(const Assets& a, const Character& c)
{
    return a.sprites[static_cast<std::size_t>(c.kind)];
}

// ---------------------------------------------------------------------------
// Regions
// ---------------------------------------------------------------------------

void drawTitleBar(sf::RenderTarget& t, const Assets& a, const ViewState& s)
{
    const R r = layout::titleBar;
    drawFrame(t, a, r);
    const float cy = r.position.y + r.size.y / 2.f;
    drawDiamond(t, {r.position.x + 28.f, cy}, 2.f, pal::gold);
    pixelfont::draw(t, "BATTLE ARENA", {r.position.x + 44.f, cy - 14.f}, heading(4.f));
    const float subX = r.position.x + 44.f + pixelfont::measure("BATTLE ARENA", 4.f) + 26.f;
    drawText(t, a.font, "The Proving Grounds", {subX, cy - 10.f}, 17, pal::textDim);

    // Right side: ROUND n (the model has no arena levels)
    const std::string round = std::to_string(s.round);
    const pixelfont::Style label{2.f, pal::textDim, lighten(pal::textDim, -16), true};
    const pixelfont::Style value = heading(3.f);
    float x = r.position.x + r.size.x - 28.f;
    x -= pixelfont::measure(round, 3.f);
    pixelfont::draw(t, round, {x, cy - 10.f}, value);
    x -= pixelfont::measure("ROUND", 2.f) + 10.f;
    pixelfont::draw(t, "ROUND", {x, cy - 6.f}, label);
}

void drawBattlefield(sf::RenderTarget& t, const Assets& a, const ViewState& s, const UiState& ui)
{
    const R r = layout::battlefield;
    const layout::Grid& grid = s.grid;
    assert(a.battlefieldGrid == grid && "call Assets::prepareBattlefield() after changing the grid");
    drawFrame(t, a, r, false);
    // The battlefield art fills the frame interior (art pixel 5 == screen pixel 10).
    sf::Sprite inner(a.battlefield);
    inner.setTextureRect(sf::IntRect({5, 5}, {540, 250}));
    inner.setScale({layout::artScale, layout::artScale});
    inner.setPosition({r.position.x + 10.f, r.position.y + 10.f});
    t.draw(inner);
    outlineRect(t, shrink(r, 8.f), pal::ink, 2.f);

    // Hovered tile highlight.
    if (ui.hoveredTile)
    {
        const R tile = grid.tileRect(*ui.hoveredTile);
        fillRect(t, tile, C(255, 236, 190, 28));
        outlineRect(t, tile, withAlpha(pal::text, 150), 1.f);
    }

    // Characters, back to front.
    std::vector<int> order(s.characters.size());
    for (std::size_t i = 0; i < order.size(); ++i)
        order[i] = static_cast<int>(i);
    std::sort(order.begin(), order.end(), [&](int x, int y) { return s.characters[x].tile.y < s.characters[y].tile.y; });

    for (int i : order)
    {
        const Character& c = s.characters[static_cast<std::size_t>(i)];
        if (!c.onBattlefield)
            continue;  // e.g. a corpse that has dissolved: still listed in the roster and HUD
        const R tile = grid.tileRect(c.tile);
        if (!c.alive)
        {
            if (i == 0)
                defeatcutscene::drawMarker(t, tile.position, layout::artScale);
            else
                drawImage(t, a.bonePile, tile.position, layout::artScale, C(200, 196, 186));
            continue;
        }
        const bool flying = c.kind == art::Kind::Bat;
        sf::CircleShape shadow(10.f, 16);
        shadow.setScale({1.f, 0.4f});
        shadow.setFillColor(C(0, 0, 0, flying ? 60 : 110));
        shadow.setPosition({tile.position.x + 5.f, tile.position.y + 21.f});
        t.draw(shadow);
        // Accent-coloured base ring ties each sprite to its sidebar/portrait colour.
        sf::CircleShape ring(13.f, 20);
        ring.setScale({1.f, 0.42f});
        ring.setFillColor(C::Transparent);
        ring.setOutlineThickness(2.f);
        ring.setOutlineColor(withAlpha(c.accent, flying ? 110 : 190));
        ring.setPosition({tile.position.x + 2.f, tile.position.y + 19.f});
        t.draw(ring);
        drawImage(t, spriteFor(a, c), tile.position, layout::artScale);
    }

    // Translucent hero at the previewed destination: a dynamic overlay, never selectable, drawn
    // under the selection brackets and tooltip so it hides neither.
    if (s.preview.ghostTile)
    {
        const R tile = grid.tileRect(*s.preview.ghostTile);
        const Character& hero = s.characters.front();
        fillRect(t, shrink(tile, 1.f), withAlpha(hero.accent, 40));
        outlineRect(t, shrink(tile, 1.f), withAlpha(hero.accent, 170), 1.f);
        drawImage(t, spriteFor(a, hero), tile.position, layout::artScale, C(200, 225, 255, 130));
    }

    // Selection brackets (gold corners) around the selected character's tile.
    if (ui.selected >= 0 && s.characters[static_cast<std::size_t>(ui.selected)].onBattlefield)
    {
        const Character& c = s.characters[static_cast<std::size_t>(ui.selected)];
        const R tile = shrink(grid.tileRect(c.tile), -3.f);
        const float L = 8.f, th = 2.f;
        const C g = pal::goldBright;
        const float x0 = tile.position.x, y0 = tile.position.y;
        const float x1 = x0 + tile.size.x, y1 = y0 + tile.size.y;
        fillRect(t, {{x0, y0}, {L, th}}, g);
        fillRect(t, {{x0, y0}, {th, L}}, g);
        fillRect(t, {{x1 - L, y0}, {L, th}}, g);
        fillRect(t, {{x1 - th, y0}, {th, L}}, g);
        fillRect(t, {{x0, y1 - th}, {L, th}}, g);
        fillRect(t, {{x0, y1 - L}, {th, L}}, g);
        fillRect(t, {{x1 - L, y1 - th}, {L, th}}, g);
        fillRect(t, {{x1 - th, y1 - L}, {th, L}}, g);
    }

    // Tooltip for a hovered character on the battlefield.
    if (ui.hovered >= 0 && r.contains(ui.mouse) && s.characters[static_cast<std::size_t>(ui.hovered)].onBattlefield)
    {
        const Character& c = s.characters[static_cast<std::size_t>(ui.hovered)];
        const std::string line1 = c.name + "  -  " + c.typeName;
        const std::string line2 = c.alive ? ("HP " + std::to_string(c.hp) + " / " + std::to_string(c.maxHp)) : "Defeated";
        const float w = std::max(textWidth(a.font, line1, 16), textWidth(a.font, line2, 15)) + 20.f;
        V p{ui.mouse.x + 16.f, ui.mouse.y + 14.f};
        p.x = std::min(p.x, r.position.x + r.size.x - w - 12.f);
        p.y = std::min(p.y, r.position.y + r.size.y - 60.f);
        const R box{p, {w, 46.f}};
        fillRect(t, box, pal::ink);
        fillRect(t, shrink(box, 1.f), C(54, 44, 34));
        fillRect(t, shrink(box, 2.f), C(26, 23, 20));
        fillRect(t, {{box.position.x + 2.f, box.position.y + 2.f}, {3.f, box.size.y - 4.f}}, c.accent);
        drawText(t, a.font, line1, {p.x + 11.f, p.y + 4.f}, 16, pal::text);
        drawText(t, a.font, line2, {p.x + 11.f, p.y + 24.f}, 15, c.alive ? pal::textDim : pal::red);
    }
}

// Outcome plaque: visual feedback only. The Arena decides the result, and no
// portrait or hero-specific artwork is required.
void drawOutcomePlaque(sf::RenderTarget& t, const Assets& a, const ViewState& s, float elapsedSeconds)
{
    if (!s.battleOver)
        return;

    // Smoothstep easing prevents the fade from starting or stopping abruptly.
    const auto fadeProgress = [elapsedSeconds](float start, float duration)
    {
        const float progress = std::clamp((elapsedSeconds - start) / duration, 0.f, 1.f);
        return progress * progress * (3.f - 2.f * progress);
    }; 
    constexpr float heroFadeSeconds = 1.4f;
    constexpr float punchlineDelaySeconds = 1.6f;
    constexpr float punchlineFadeSeconds = 0.8f;
    const float entranceOpacity = fadeProgress(0.f, heroFadeSeconds);
    const float punchlineOpacity = fadeProgress(punchlineDelaySeconds, punchlineFadeSeconds);
    const auto faded = [](C color, float opacity)
    {
        color.a = static_cast<std::uint8_t>(std::lround(static_cast<float>(color.a) * opacity));
        return color;
    };

    const bool victory = s.heroWon;
    const std::string heroName = s.characters.empty() ? "The hero" : s.characters.front().name;
    const std::string title = victory ? "VICTORY!" : "DEFEAT!";
    const std::string declaration = victory ? heroName + " stands triumphant."
                                            : heroName + " has fallen. His courage will be remembered.";
    const std::string quip = victory ? "\"Now where is that damn gold you promised me!?\""
                                     : "\"So... I suppose I'm not getting paid?\"";
    const std::string attribution = victory ? "- " + heroName : "- " + heroName + "'s final words";

    // Keep the same plaque geometry and theatrical timing for both outcomes.
    const C plaqueFill = victory ? C(54, 32, 27, 248) : C(43, 31, 36, 248);
    const C outerTrim = victory ? pal::goldBright : C(218, 146, 142);
    const C innerTrim = victory ? C(127, 88, 46) : C(119, 73, 78);
    const C heraldry = victory ? pal::gold : C(183, 121, 121);
    const C titleTop = victory ? C(255, 232, 155) : C(245, 191, 187);
    const C titleBottom = victory ? pal::goldBright : C(219, 133, 129);
    const C topRule = victory ? C(149, 106, 57) : C(153, 92, 97);
    const C lowerRule = victory ? C(109, 77, 46) : C(122, 78, 83);
    const C seriousColor = victory ? pal::text : C(231, 211, 205);
    const C jokeColor = victory ? C(244, 194, 147) : C(229, 181, 170);

    const R field = layout::battlefield;
    fillRect(t, shrink(field, 10.f), faded(C(7, 5, 4, 125), entranceOpacity));

    const R banner{{field.position.x + (field.size.x - 872.f) / 2.f,
                    field.position.y + (field.size.y - 238.f) / 2.f},
                   {872.f, 238.f}};
    const float cx = banner.position.x + banner.size.x / 2.f;

    // The two outcomes share the wood-and-iron geometry with different accents.
    fillRect(t, {{banner.position.x + 7.f, banner.position.y + 8.f}, banner.size}, faded(C(0, 0, 0, 160), entranceOpacity));
    fillRect(t, banner, faded(C(10, 8, 7, 245), entranceOpacity));
    fillRect(t, shrink(banner, 4.f), faded(plaqueFill, entranceOpacity));
    outlineRect(t, shrink(banner, 8.f), faded(outerTrim, entranceOpacity), 2.f);
    outlineRect(t, shrink(banner, 14.f), faded(innerTrim, entranceOpacity), 1.f);

    // Heraldic accents deliberately don't depict a particular hero.
    drawDiamond(t, {banner.position.x + 84.f, banner.position.y + 63.f}, 3.f, faded(heraldry, entranceOpacity));
    drawDiamond(t, {banner.position.x + banner.size.x - 84.f, banner.position.y + 63.f}, 3.f, faded(heraldry, entranceOpacity));
    pixelfont::draw(t, title,
                    {cx - pixelfont::measure(title, 9.f) / 2.f, banner.position.y + 24.f},
                    {9.f, titleTop, titleBottom, true}, entranceOpacity);

    fillRect(t, {{banner.position.x + 176.f, banner.position.y + 103.f},
                 {banner.size.x - 352.f, 2.f}}, faded(topRule, entranceOpacity));

    auto centeredText = [&](const std::string& message, float y, unsigned size, C color, float opacity, bool italic = false)
    {
        sf::Text text(a.font, message, size);
        if (italic)
            text.setStyle(sf::Text::Italic);
        const R bounds = text.getLocalBounds();
        text.setPosition({std::round(cx - bounds.position.x - bounds.size.x / 2.f), std::round(y)});
        sf::Text shadow = text;
        shadow.move({1.f, 1.f});
        shadow.setFillColor(faded(C(0, 0, 0, 210), opacity));
        t.draw(shadow);
        text.setFillColor(faded(color, opacity));
        t.draw(text);
    };

    centeredText(declaration, banner.position.y + 115.f, victory ? 27u : 24u, seriousColor, entranceOpacity);
    fillRect(t, {{banner.position.x + 248.f, banner.position.y + 150.f},
                 {banner.size.x - 496.f, 1.f}}, faded(lowerRule, punchlineOpacity));
    centeredText(quip, banner.position.y + 159.f, 23, jokeColor, punchlineOpacity, true);
    centeredText(attribution, banner.position.y + 195.f, 16, pal::textDim, punchlineOpacity);
}

void drawHeroPanel(sf::RenderTarget& t, const Assets& a, const ViewState& s, const UiState& ui)
{
    const R r = layout::heroPanel;
    const Character& hero = s.characters.front();
    drawFrame(t, a, r);
    if (ui.selected == 0)
        outlineRect(t, shrink(r, 7.f), pal::goldBright, 2.f);

    const float x = r.position.x + 18.f;
    pixelfont::draw(t, "HERO STATUS", {x, r.position.y + 16.f}, heading());

    // Level badge, top right.
    const std::string lv = "LV " + std::to_string(s.heroLevel);
    const float bw = pixelfont::measure(lv, 2.f) + 16.f;
    const R badge{{r.position.x + r.size.x - 18.f - bw, r.position.y + 12.f}, {bw, 22.f}};
    fillRect(t, badge, pal::ink);
    fillRect(t, shrink(badge, 1.f), C(40, 62, 84));
    outlineRect(t, shrink(badge, 1.f), pal::steel, 1.f);
    pixelfont::draw(t, lv, {badge.position.x + 8.f, badge.position.y + 4.f}, parchment());

    drawText(t, a.font, hero.name + " the " + s.heroTitle, {x, r.position.y + 36.f}, 22, C(244, 232, 204));

    const R bar{{x, r.position.y + 70.f}, {266.f, 18.f}};
    drawHealthBar(t, bar, hero.hp, hero.maxHp);
    drawText(t, a.font, std::to_string(hero.hp) + " / " + std::to_string(hero.maxHp) + " HP",
             {bar.position.x + bar.size.x + 12.f, bar.position.y - 3.f}, 19, pal::text);

    float lx = x;
    const float ly = r.position.y + 95.f;
    const std::string monsters = std::to_string(s.characters.size() - 1);
    lx += drawText(t, a.font, "Level " + std::to_string(s.heroLevel), {lx, ly}, 16, pal::textDim) + 22.f;
    drawText(t, a.font, "Defeated " + std::to_string(s.monstersDefeated) + " / " + monsters, {lx, ly}, 16, pal::textDim);
}

void drawRosterPanel(sf::RenderTarget& t, const Assets& a, const ViewState& s, const UiState& ui)
{
    const R r = layout::rosterPanel;
    drawFrame(t, a, r);
    pixelfont::draw(t, "MONSTERS", {r.position.x + 18.f, r.position.y + 16.f}, heading());
    const int alive = static_cast<int>(std::count_if(s.characters.begin() + 1, s.characters.end(),
                                                     [](const Character& c) { return c.alive; }));
    const std::string remaining = std::to_string(alive) + " REMAIN";
    pixelfont::draw(t, remaining, {r.position.x + r.size.x - 18.f - pixelfont::measure(remaining, 2.f), r.position.y + 16.f},
                    {2.f, pal::textDim, lighten(pal::textDim, -16), true});

    for (int i = 1; i < static_cast<int>(s.characters.size()); ++i)
    {
        const Character& c = s.characters[static_cast<std::size_t>(i)];
        const R row = layout::rosterRow(i - 1);
        const bool dead = !c.alive;
        const bool sel = ui.selected == i, hov = ui.hovered == i;

        fillRect(t, row, sel ? C(62, 52, 36, 220) : (hov ? C(48, 44, 38, 200) : C(20, 19, 18, 150)));
        fillRect(t, {row.position, {4.f, row.size.y}}, dead ? C(70, 66, 60) : c.accent);
        if (sel)
            outlineRect(t, row, pal::goldBright, 1.f);

        const sf::Texture& icon = dead ? a.spritesGray[static_cast<std::size_t>(c.kind)] : spriteFor(a, c);
        drawImage(t, icon, {row.position.x + 10.f, row.position.y}, 2.f, dead ? C(150, 150, 150) : C::White);

        const C nameColor = dead ? pal::textFaint : C(244, 232, 204);
        const float nw = drawText(t, a.font, c.name, {row.position.x + 50.f, row.position.y + 4.f}, 18, nameColor);
        if (dead)
            fillRect(t, {{row.position.x + 49.f, row.position.y + 16.f}, {nw + 2.f, 2.f}}, C(140, 50, 44));
        drawText(t, a.font, c.typeName, {row.position.x + 122.f, row.position.y + 6.f}, 16,
                 dead ? pal::textFaint : lighten(c.accent, 10));

        const R bar{{row.position.x + 224.f, row.position.y + 10.f}, {114.f, 12.f}};
        drawHealthBar(t, bar, c.hp, c.maxHp);
        if (dead)
            pixelfont::draw(t, "SLAIN", {row.position.x + 350.f, row.position.y + 9.f},
                            {2.f, C(214, 90, 74), pal::red, true});
        else
            drawText(t, a.font, std::to_string(c.hp) + "/" + std::to_string(c.maxHp),
                     {row.position.x + 350.f, row.position.y + 5.f}, 17, pal::text);
    }
}

C logColor(LogKind k)
{
    switch (k)
    {
    case LogKind::Info: return pal::text;
    case LogKind::Damage: return C(226, 104, 88);
    case LogKind::Heal: return C(136, 196, 120);
    case LogKind::Special: return pal::special;
    case LogKind::Victory: return pal::goldBright;
    case LogKind::System: return pal::textDim;
    }
    return pal::text;
}

// Restrict drawing to a rectangle (in view coordinates) using the view's scissor,
// mapped through the current viewport so it stays correct when letterboxed.
class ScopedClip
{
public:
    ScopedClip(sf::RenderTarget& t, R clip) : target(t), saved(t.getView())
    {
        sf::View clipped = saved;
        const R vp = saved.getViewport();
        const V viewTopLeft = saved.getCenter() - saved.getSize() / 2.f;
        const float sx = vp.size.x / saved.getSize().x, sy = vp.size.y / saved.getSize().y;
        clipped.setScissor({{vp.position.x + (clip.position.x - viewTopLeft.x) * sx, vp.position.y + (clip.position.y - viewTopLeft.y) * sy},
                            {clip.size.x * sx, clip.size.y * sy}});
        target.setView(clipped);
    }
    ~ScopedClip() { target.setView(saved); }
    ScopedClip(const ScopedClip&) = delete;
    ScopedClip& operator=(const ScopedClip&) = delete;

private:
    sf::RenderTarget& target;
    sf::View saved;
};

void drawLogPanel(sf::RenderTarget& t, const Assets& a, const ViewState& s, const UiState& ui)
{
    const R r = layout::logPanel;
    const int total = static_cast<int>(ui.logRows.size());
    const int scroll = logscroll::clampScroll(ui.logScroll, total);
    drawFrame(t, a, r);
    pixelfont::draw(t, "COMBAT LOG", {r.position.x + 18.f, r.position.y + 16.f}, heading());
    const std::string hint = scroll > 0 ? "SCROLLED" : "LATEST";
    pixelfont::draw(t, hint, {r.position.x + r.size.x - 18.f - pixelfont::measure(hint, 2.f), r.position.y + 16.f},
                    {2.f, pal::textDim, lighten(pal::textDim, -16), true});

    const R area = layout::logArea;
    drawInset(t, area, C(16, 15, 14));

    const int lines = logscroll::visibleLines();
    const int last = total - 1 - scroll;  // newest visible text row
    const int first = std::max(0, last - lines + 1);
    float y = area.position.y + 4.f;
    {
        const ScopedClip clip(t, shrink(area, 1.f));  // no text escapes the log well
        for (int i = first; i <= last; ++i)
        {
            const loglayout::Row& row = ui.logRows[static_cast<std::size_t>(i)];
            const LogEntry& entry = s.log[row.entryIndex];
            const bool newest = row.entryIndex == s.log.size() - 1;
            if (entry.round == s.log.back().round)
                fillRect(t, {{area.position.x + 2.f, y - 1.f}, {area.size.x - 18.f, layout::logLineHeight - 1.f}},
                         C(255, 220, 160, newest ? 18 : 9));
            if (row.firstOfEntry)
                drawText(t, a.font, "R" + std::to_string(entry.round),
                         {area.position.x + 8.f, y}, 14, pal::textFaint, false);
            drawText(t, a.font, row.text, {area.position.x + 42.f, y - 1.f}, 16, logColor(entry.kind));
            y += layout::logLineHeight;
        }
    } // clip ends

    // Scrollbar: the log scrolls internally; the window never scrolls.
    const logscroll::Geometry sb = logscroll::geometry(total, scroll);
    fillRect(t, sb.track, C(8, 8, 8));
    if (!sb.enabled())
    {
        // Everything fits: show a flat, inert thumb so the bar reads as disabled.
        fillRect(t, shrink(sb.thumb, 1.f), C(34, 31, 28));
        return;
    }
    const R thumb{{sb.thumb.position.x + 1.f, sb.thumb.position.y}, {sb.thumb.size.x - 2.f, sb.thumb.size.y}};
    fillRect(t, thumb, ui.draggingLogThumb ? lighten(pal::wood, 22) : pal::wood);
    fillRect(t, {thumb.position, {thumb.size.x, 1.f}}, pal::woodLight);
    fillRect(t, {thumb.position, {1.f, thumb.size.y}}, pal::woodLight);
    fillRect(t, {{thumb.position.x, thumb.position.y + thumb.size.y - 1.f}, {thumb.size.x, 1.f}}, pal::woodDark);
}

void drawCommandBar(sf::RenderTarget& t, const Assets& a, const ViewState& s, const UiState& ui)
{
    const R r = layout::commandBar;
    drawFrame(t, a, r);
    const float cy = r.position.y + r.size.y / 2.f;
    pixelfont::draw(t, "COMMAND", {r.position.x + 22.f, cy - 7.f}, heading());

    const R box{{r.position.x + 128.f, cy - 15.f}, {230.f, 30.f}};
    drawInset(t, box, C(12, 12, 11));
    float px = box.position.x + 10.f;
    for (char ch : ui.pending)
    {
        const std::string one(1, ch);
        pixelfont::draw(t, one, {px, cy - 10.f}, {3.f, C(255, 240, 200), pal::goldBright, true});
        px += pixelfont::measure(one, 3.f) + 9.f;
    }
    fillRect(t, {{px, cy - 10.f}, {12.f, 21.f}}, withAlpha(pal::goldBright, 170));  // block cursor

    // Every hint is always listed, so nothing shifts; unavailable ones are muted. Fight follows
    // the Arena's canFight, Submit follows the Arena's preview of the pending command, and
    // after the battle (or in a visual test, for Submit) only Esc remains.
    const bool playing = !s.battleOver;
    const bool canSubmit = playing && s.visualTest.empty() && s.preview.tone == FeedbackTone::Valid;
    struct Hint { std::vector<std::string> keys; std::string label; bool enabled = true; };
    const std::vector<Hint> hints{{{"W", "A", "S", "D"}, "Move", playing}, {{"Enter"}, "Submit", canSubmit},
                                  {{"Backspace"}, "Edit", playing}, {{"R"}, "Rest", playing},
                                  {{"F"}, "Fight", playing && s.canFight}, {{"Esc"}, "Exit"}};
    // Measure, then right-align.
    float total = 0.f;
    for (const auto& h : hints)
    {
        for (const auto& k : h.keys)
            total += std::max(24.f, textWidth(a.font, k, 15) + 14.f) + 4.f;
        total += textWidth(a.font, h.label, 17) + 30.f;
    }
    float x = r.position.x + r.size.x - 20.f - total;

    // Feedback on the pending command, from the Arena's preview. Long messages shrink a
    // little, then are shortened, so they never run into the hints.
    std::string feedback = "Enter a command";
    C color = pal::textFaint;
    if (s.battleOver)
    {
        feedback = s.heroWon ? "Victory! The battle is won." : "Defeat. The battle is lost.";
        color = s.heroWon ? pal::goldBright : lighten(pal::red, 30);
    }
    else if (!s.preview.feedback.empty())
    {
        feedback = s.preview.feedback;
        color = s.preview.tone == FeedbackTone::Valid     ? lighten(pal::green, 20)
                : s.preview.tone == FeedbackTone::Invalid ? lighten(pal::red, 30)
                                                          : pal::textDim;
    }
    const float fx = box.position.x + box.size.x + 10.f;
    const float room = x - 16.f - fx;
    unsigned size = 15;
    while (size > 13 && textWidth(a.font, feedback, size) > room)
        --size;
    if (textWidth(a.font, feedback, size) > room)
    {
        while (!feedback.empty() && textWidth(a.font, feedback + "...", size) > room)
            feedback.pop_back();
        feedback += "...";
    }
    drawText(t, a.font, feedback, {fx, cy - 10.f + (15.f - static_cast<float>(size)) / 2.f}, size, color);

    for (const auto& h : hints)
    {
        for (const auto& k : h.keys)
        {
            float w = 0.f;
            drawKeyCap(t, a.font, k, {x, cy - 12.f}, w, !h.enabled);
            x += w + 4.f;
        }
        x += 4.f;
        x += drawText(t, a.font, h.label, {x, cy - 11.f}, 17, h.enabled ? pal::text : pal::textFaint) + 26.f;
    }
}

void drawCard(sf::RenderTarget& t, const Assets& a, const ViewState& s, const UiState& ui, int i)
{
    const Character& c = s.characters[static_cast<std::size_t>(i)];
    const bool isHero = i == 0;
    const bool dead = !c.alive;
    const R r = layout::card(i);

    // Card body.
    fillRect(t, r, pal::ink);
    drawTiled(t, a.cardNoise, shrink(r, 1.f), 2.f);
    fillRect(t, {{r.position.x + 1.f, r.position.y + 1.f}, {r.size.x - 2.f, 1.f}}, C(70, 62, 50));
    fillRect(t, {{r.position.x + 1.f, r.position.y + 2.f}, {r.size.x - 2.f, 4.f}}, dead ? C(64, 60, 56) : c.accent);
    fillRect(t, {{r.position.x + 1.f, r.position.y + 6.f}, {r.size.x - 2.f, 1.f}}, C(0, 0, 0, 160));

    // Portrait well.
    const R well{{r.position.x + 12.f, r.position.y + 20.f}, {120.f, 120.f}};
    fillRect(t, well, C(110, 100, 84));
    fillRect(t, {well.position, {well.size.x - 1.f, well.size.y - 1.f}}, pal::ink);
    fillRect(t, shrink(well, 1.f), C(36, 32, 28));
    const V portraitPos{well.position.x + 4.f, well.position.y + 4.f};
    const std::size_t k = static_cast<std::size_t>(c.kind);
    drawImage(t, dead ? a.portraitsGray[k] : a.portraits[k], portraitPos, 4.f);

    if (dead)
    {
        fillRect(t, {portraitPos, {112.f, 112.f}}, C(10, 6, 6, 70));
        // Skull badge in the corner so the (greyed) face stays recognisable.
        const R badge{{portraitPos.x + 112.f - 43.f, portraitPos.y + 4.f}, {39.f, 33.f}};
        fillRect(t, badge, pal::ink);
        fillRect(t, shrink(badge, 1.f), C(70, 22, 20));
        drawImage(t, a.skull, {badge.position.x + 3.f, badge.position.y + 3.f}, 3.f);
        const R banner{{portraitPos.x - 2.f, portraitPos.y + 80.f}, {116.f, 22.f}};
        fillRect(t, banner, pal::ink);
        fillRect(t, shrink(banner, 1.f), C(118, 30, 26));
        fillRect(t, {{banner.position.x + 1.f, banner.position.y + 1.f}, {banner.size.x - 2.f, 1.f}}, C(176, 62, 50));
        pixelfont::draw(t, "DEFEATED", {banner.position.x + (banner.size.x - pixelfont::measure("DEFEATED", 2.f)) / 2.f, banner.position.y + 4.f},
                        {2.f, C(255, 226, 200), C(240, 196, 170), true});
    }

    // Text column.
    const float x = r.position.x + 146.f;
    const pixelfont::Style nameStyle = dead ? pixelfont::Style{3.f, C(140, 132, 116), C(112, 104, 90), true}
                                            : (isHero ? pixelfont::Style{3.f, C(200, 226, 244), pal::steel, true} : parchment(3.f));
    pixelfont::draw(t, c.name, {x, r.position.y + 22.f}, nameStyle);
    const std::string type = isHero ? (s.heroTitle + "  |  Level " + std::to_string(s.heroLevel)) : c.typeName;
    drawText(t, a.font, type, {x, r.position.y + 50.f}, 17, dead ? pal::textFaint : lighten(c.accent, 20));

    drawText(t, a.font, "HP", {x, r.position.y + 78.f}, 14, pal::textDim);
    const R bar{{x + 26.f, r.position.y + 80.f}, {r.position.x + r.size.x - 14.f - (x + 26.f), 14.f}};
    drawHealthBar(t, bar, c.hp, c.maxHp);
    drawText(t, a.font, std::to_string(c.hp) + " / " + std::to_string(c.maxHp), {x, r.position.y + 98.f}, 19,
             dead ? pal::textFaint : pal::text);

    // Status tag.
    const std::string tag = isHero ? "HERO" : (dead ? "DEFEATED" : "ALIVE");
    const C tagColor = isHero ? pal::steel : (dead ? pal::red : pal::green);
    const float tw = pixelfont::measure(tag, 2.f);
    const R tagBox{{x, r.position.y + 126.f}, {tw + 12.f, 18.f}};
    fillRect(t, tagBox, pal::ink);
    fillRect(t, shrink(tagBox, 1.f), lighten(tagColor, -70));
    outlineRect(t, shrink(tagBox, 1.f), tagColor, 1.f);
    pixelfont::draw(t, tag, {tagBox.position.x + 6.f, tagBox.position.y + 2.f}, {2.f, lighten(tagColor, 60), lighten(tagColor, 30), false});

    // Hover / selection outline.
    if (ui.selected == i)
        outlineRect(t, shrink(r, -3.f), pal::goldBright, 2.f);
    else if (ui.hovered == i)
        outlineRect(t, shrink(r, -2.f), withAlpha(pal::text, 160), 1.f);
}

void drawPortraitHud(sf::RenderTarget& t, const Assets& a, const ViewState& s, const UiState& ui)
{
    drawFrame(t, a, layout::portraitHud);
    for (int i = 0; i < static_cast<int>(s.characters.size()) && i < 5; ++i)
        drawCard(t, a, s, ui, i);
}

void drawStatusLine(sf::RenderTarget& t, const Assets& a, const ViewState& s, const UiState& ui)
{
    if (!ui.logExportNotice.empty())
        drawText(t, a.font, ui.logExportNotice, {24.f, layout::statusLineY}, 14,
                 ui.logExportSucceeded ? lighten(pal::green, 25) : lighten(pal::red, 30), false);
    else if (s.battleOver)
        drawText(t, a.font, s.heroWon ? "VICTORY  -  every monster is defeated" : "DEFEAT  -  the hero has fallen",
                 {24.f, layout::statusLineY}, 14, s.heroWon ? pal::goldBright : lighten(pal::red, 30), false);
    else if (s.visualTest.empty())
        drawText(t, a.font, "Live Arena  -  type a command and press Enter to play a round",
                 {24.f, layout::statusLineY}, 14, pal::textFaint, false);
    else
        drawText(t, a.font, "VISUAL TEST  -  " + s.visualTest + "  -  not the live Arena",
                 {24.f, layout::statusLineY}, 14, pal::gold, false);
    const std::string right = "F8 Save log  |  Hover to highlight  |  Click to inspect  |  Wheel scrolls log  |  W A S D R F  Enter  Backspace  Esc";
    drawText(t, a.font, right, {1576.f - textWidth(a.font, right, 14), layout::statusLineY}, 14, pal::textFaint, false);
}
} // namespace

bool Assets::prepareBattlefield(const layout::Grid& grid)
{
    if (battlefieldReady && grid == battlefieldGrid)
        return true;
    art::BattlefieldSpec spec;  // fixed visual seed: the same grid always looks the same
    spec.cols = static_cast<unsigned>(grid.cols);
    spec.rows = static_cast<unsigned>(grid.rows);
    spec.gridOrigin = grid.artOrigin();
    if (!battlefield.loadFromImage(art::buildBattlefield(spec)))
        return false;
    battlefield.setSmooth(false);
    battlefieldGrid = grid;
    battlefieldReady = true;
    return true;
}

bool Assets::load(const std::filesystem::path& exeDir, const layout::Grid& grid)
{
    std::filesystem::path dir;
    for (const auto& candidate : {exeDir / "assets", std::filesystem::current_path() / "assets"})
        if (std::filesystem::exists(candidate / "fonts" / "tuffy.ttf"))
        {
            dir = candidate;
            break;
        }
    if (dir.empty() || !font.openFromFile(dir / "fonts" / "tuffy.ttf"))
    {
        std::cerr << "Could not find assets/fonts/tuffy.ttf next to the executable or in the working directory.\n";
        return false;
    }

    auto make = [](sf::Texture& tex, const sf::Image& img) {
        if (!tex.loadFromImage(img))
            return false;
        tex.setSmooth(false);  // nearest-neighbour: pixel art stays crisp when enlarged
        return true;
    };

    bool ok = prepareBattlefield(grid);
    ok = ok && make(panelNoise, art::buildPanelNoise(128, sf::Color(40, 38, 35), 11));
    ok = ok && make(cardNoise, art::buildPanelNoise(128, sf::Color(28, 26, 24), 23));
    panelNoise.setRepeated(true);
    cardNoise.setRepeated(true);
    for (int i = 0; i < art::kindCount && ok; ++i)
    {
        const auto kind = static_cast<art::Kind>(i);
        const sf::Image sprite = art::buildSprite(kind);
        const sf::Image portrait = art::buildPortrait(kind);
        const auto idx = static_cast<std::size_t>(i);
        ok = make(sprites[idx], sprite) && make(spritesGray[idx], art::toGrayscale(sprite, 0.7f)) &&
             make(portraits[idx], portrait) && make(portraitsGray[idx], art::toGrayscale(portrait, 0.62f));
    }
    ok = ok && make(bonePile, art::buildBonePile()) && make(skull, art::buildSkull());
    if (!ok)
        std::cerr << "Failed to create textures.\n";
    return ok;
}

void drawScene(sf::RenderTarget& target, const Assets& assets, const ViewState& state, const UiState& ui)
{
    target.clear(pal::background);
    drawTitleBar(target, assets, state);
    drawBattlefield(target, assets, state, ui);
    if (state.battleOver && state.heroWon)
    {
        if (ui.outcomeElapsedSeconds < victorycutscene::duration)
            victorycutscene::draw(target, layout::battlefield, ui.outcomeElapsedSeconds);
        else
            drawOutcomePlaque(target, assets, state, ui.outcomeElapsedSeconds - victorycutscene::duration);
    }
    else if (state.battleOver && !state.heroWon)
    {
        if (ui.outcomeElapsedSeconds < defeatcutscene::duration)
            defeatcutscene::draw(target, layout::battlefield, ui.outcomeElapsedSeconds);
        else
            drawOutcomePlaque(target, assets, state, ui.outcomeElapsedSeconds - defeatcutscene::duration);
    }
    else
        drawOutcomePlaque(target, assets, state, ui.outcomeElapsedSeconds);
    drawHeroPanel(target, assets, state, ui);
    drawRosterPanel(target, assets, state, ui);
    drawLogPanel(target, assets, state, ui);
    drawCommandBar(target, assets, state, ui);
    drawPortraitHud(target, assets, state, ui);
    drawStatusLine(target, assets, state, ui);
}

std::optional<sf::Vector2i> tileAt(sf::Vector2f p, const ViewState& state)
{
    return state.grid.tileAt(p);
}

int characterAt(sf::Vector2f p, const ViewState& state)
{
    const int n = static_cast<int>(state.characters.size());
    if (const auto tile = state.grid.tileAt(p))
        for (int i = 0; i < n; ++i)
            if (state.characters[static_cast<std::size_t>(i)].onBattlefield &&
                state.characters[static_cast<std::size_t>(i)].tile == *tile)
                return i;
    for (int i = 0; i < n && i < 5; ++i)
        if (layout::card(i).contains(p))
            return i;
    for (int i = 1; i < n; ++i)
        if (layout::rosterRow(i - 1).contains(p))
            return i;
    if (layout::heroPanel.contains(p))
        return 0;
    return -1;
}

bool isOverLog(sf::Vector2f p)
{
    return layout::logPanel.contains(p);
}
