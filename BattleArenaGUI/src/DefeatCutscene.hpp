#pragma once

// A short, scripted defeat vignette. Art uses solid pixel blocks (no filtering or
// external assets); only drawing is affected. This is intentionally not an animation engine.

#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/VertexArray.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace defeatcutscene
{
    inline constexpr float duration = 3.5f;
    inline constexpr float authoredDuration = 1.8f;

namespace detail
{
using C = sf::Color;
using V = sf::Vector2f;

struct Block
{
    float x, y, w, h;
    C color;
};
struct Pose
{
    V position;
    float angle = 0.f;  // radians, clockwise in screen coordinates
};

inline float ease(float t)
{
    t = std::clamp(t, 0.f, 1.f);
    return t * t * (3.f - 2.f * t);
}
inline float tween(float a, float b, float t) { return a + (b - a) * ease(t); }
inline V lerp(V a, V b, float t) { return {tween(a.x, b.x, t), tween(a.y, b.y, t)}; }
inline Pose blend(Pose a, Pose b, float t)
{
    return {lerp(a.position, b.position, t), tween(a.angle, b.angle, t)};
}
inline V transformed(V point, Pose pose, float scale)
{
    const float cs = std::cos(pose.angle), sn = std::sin(pose.angle);
    return {pose.position.x + scale * (point.x * cs - point.y * sn),
            pose.position.y + scale * (point.x * sn + point.y * cs)};
}

// Each block is a solid rectangle in low-resolution 'art pixels'. Rotating a set
// of these yields a deliberately chunky, old-school silhouette rather than a
// smooth vector illustration.
template <std::size_t N>
inline void drawBlocks(sf::RenderTarget& target, const std::array<Block, N>& blocks,
                       Pose pose, float scale, float opacity = 1.f)
{
    if (opacity <= 0.f)
        return;
    sf::VertexArray triangles(sf::PrimitiveType::Triangles);
    for (const Block& b : blocks)
    {
        C color = b.color;
        color.a = static_cast<std::uint8_t>(std::lround(static_cast<float>(color.a) * opacity));
        const V tl = transformed({b.x, b.y}, pose, scale);
        const V tr = transformed({b.x + b.w, b.y}, pose, scale);
        const V bl = transformed({b.x, b.y + b.h}, pose, scale);
        const V br = transformed({b.x + b.w, b.y + b.h}, pose, scale);
        for (const V p : {tl, tr, bl, tr, br, bl})
            triangles.append({p, color});
    }
    target.draw(triangles);
}

// Bob's body, without helmet, sword or shield. All pieces have the same
// original blue-and-steel palette as the small battlefield knight.
inline const std::array body{
    Block{-7, 9, 6, 20, C(22, 29, 43)}, Block{1, 9, 6, 20, C(22, 29, 43)},
    Block{-6, 10, 5, 16, C(104, 133, 160)}, Block{1, 10, 5, 16, C(104, 133, 160)},
    Block{-9, 27, 9, 5, C(22, 29, 43)}, Block{1, 27, 9, 5, C(22, 29, 43)},
    Block{-8, 27, 8, 3, C(171, 187, 202)}, Block{1, 27, 8, 3, C(171, 187, 202)},
    Block{-10, -14, 20, 27, C(17, 23, 38)}, Block{-9, -13, 18, 24, C(150, 169, 188)},
    Block{-8, -11, 16, 20, C(59, 100, 156)}, Block{-6, -9, 12, 16, C(76, 123, 181)},
    Block{-1, -9, 2, 16, C(216, 172, 87)}, Block{-6, -2, 12, 2, C(216, 172, 87)},
    Block{-11, 8, 22, 4, C(32, 43, 64)}, Block{-9, 9, 18, 2, C(180, 188, 202)},
    Block{-17, -11, 7, 20, C(17, 23, 38)}, Block{-16, -10, 5, 16, C(165, 182, 197)},
    Block{-16, 6, 5, 6, C(35, 46, 64)}, Block{-15, 7, 3, 3, C(209, 215, 218)},
    Block{10, -11, 7, 20, C(17, 23, 38)}, Block{11, -10, 5, 16, C(165, 182, 197)},
    Block{11, 6, 5, 6, C(35, 46, 64)}, Block{12, 7, 3, 3, C(209, 215, 218)},
    Block{-6, -25, 12, 13, C(22, 24, 33)}, Block{-5, -24, 10, 11, C(193, 157, 121)},
    Block{-5, -21, 10, 3, C(75, 68, 69)}, Block{-3, -21, 2, 2, C(15, 18, 26)},
    Block{2, -21, 2, 2, C(15, 18, 26)},
};
inline const std::array helmet{
    Block{-8, -4, 16, 11, C(20, 26, 38)}, Block{-7, -5, 14, 9, C(178, 193, 206)},
    Block{-6, -4, 12, 4, C(224, 231, 232)}, Block{-5, 3, 10, 2, C(55, 71, 90)},
    Block{-3, 5, 6, 3, C(121, 140, 160)}, Block{-1, -10, 3, 6, C(68, 109, 172)},
    Block{-3, -12, 4, 4, C(46, 76, 127)},
};
inline const std::array sword{
    Block{-2, -30, 4, 25, C(22, 29, 39)}, Block{-1, -29, 2, 23, C(221, 233, 237)},
    Block{-1, -32, 2, 4, C(238, 242, 241)}, Block{-6, -6, 12, 3, C(36, 41, 52)},
    Block{-5, -5, 10, 2, C(207, 159, 74)}, Block{-2, -3, 4, 10, C(30, 30, 43)},
    Block{-1, -2, 2, 8, C(130, 86, 53)}, Block{-4, 6, 8, 3, C(211, 172, 91)},
};
inline const std::array shield{
    Block{-9, -12, 18, 20, C(22, 27, 40)}, Block{-8, -11, 16, 17, C(145, 161, 179)},
    Block{-7, -10, 14, 15, C(62, 104, 159)}, Block{-5, -8, 10, 11, C(74, 118, 174)},
    Block{-1, -8, 2, 13, C(222, 175, 83)}, Block{-5, -2, 10, 2, C(222, 175, 83)},
    Block{-7, 7, 14, 3, C(26, 35, 53)}, Block{-4, 10, 8, 3, C(26, 35, 53)},
};

// Pixel-sized battlefield memorial, retained behind the plaque after the cutscene.
inline const std::array remains{
    Block{0, 10, 13, 2, C(25, 25, 30)}, Block{2, 7, 10, 3, C(83, 99, 120)},
    Block{3, 6, 8, 2, C(166, 179, 190)}, Block{5, 3, 3, 4, C(199, 211, 218)},
    Block{6, 1, 2, 3, C(224, 229, 226)}, Block{1, 8, 5, 2, C(65, 103, 159)},
    Block{4, 6, 2, 4, C(210, 166, 78)}, Block{11, 2, 1, 10, C(215, 226, 231)},
    Block{10, 10, 4, 1, C(189, 151, 76)},
};
} // namespace detail

inline void drawMarker(sf::RenderTarget& target, sf::Vector2f tileTopLeft, float scale)
{
    detail::drawBlocks(target, detail::remains,
                       {{tileTopLeft.x + scale, tileTopLeft.y + scale}, 0.f}, scale);
}

inline void draw(sf::RenderTarget& target, sf::FloatRect battlefield, float seconds)
{
    using namespace detail;
    const float t = std::clamp(seconds, 0.f, duration)
        * (authoredDuration / duration);
    const float fade = 1.f - ease((t - 1.55f) / 0.25f);
    const float cx = battlefield.position.x + battlefield.size.x / 2.f;
    const float top = battlefield.position.y;
    constexpr float scale = 5.f; // 65 art-pixel figure => about 325 screen pixels tall

    // The original dungeon remains the backdrop. Only a translucent vignette
    // and a large foreground Bob are drawn on top of the existing battlefield.
    sf::RectangleShape veil({battlefield.size.x - 20.f, battlefield.size.y - 20.f});
    veil.setPosition({battlefield.position.x + 10.f, top + 10.f});
    veil.setFillColor(C(4, 5, 11, static_cast<std::uint8_t>(std::lround(145.f * fade))));
    target.draw(veil);

    Pose bodyPose{{cx, top + 290.f}, 0.f};
    if (t < 0.30f)
        bodyPose = blend(bodyPose, {{cx - 14.f, top + 290.f}, -0.15f}, t / 0.30f);
    else if (t < 0.70f)
        bodyPose = blend({{cx - 14.f, top + 290.f}, -0.15f}, {{cx + 5.f, top + 326.f}, 0.25f}, (t - 0.30f) / 0.40f);
    else if (t < 1.20f)
        bodyPose = blend({{cx + 5.f, top + 326.f}, 0.25f}, {{cx + 50.f, top + 384.f}, 1.15f}, (t - 0.70f) / 0.50f);
    else
        bodyPose = blend({{cx + 50.f, top + 384.f}, 1.15f}, {{cx + 66.f, top + 398.f}, 1.52f}, (t - 1.20f) / 0.30f);

    // Ground shadow, with deliberately simple 2D perspective.
    sf::RectangleShape ground({260.f, 13.f});
    ground.setPosition({cx - 102.f, top + 440.f});
    ground.setFillColor(C(0, 0, 0, static_cast<std::uint8_t>(std::lround(110.f * fade))));
    target.draw(ground);

    drawBlocks(target, body, bodyPose, scale, fade);

    // Equipment is attached at first, then tumbles independently. Smoothstep
    // interpolation gives movement without accumulating any simulation state.
    const V head = transformed({0.f, -29.f}, bodyPose, scale);
    Pose helmPose{head, bodyPose.angle};
    if (t >= 0.67f)
    {
        const float u = ease((t - 0.67f) / 0.81f);
        const V start{cx + 38.f, top + 199.f};
        helmPose.position = {start.x + (cx + 360.f - start.x) * u,
                             start.y + (top + 426.f - start.y) * u - 32.f * std::sin(3.14159265f * u)};
        helmPose.angle = 0.25f + 1.55f * u;
    }
    drawBlocks(target, helmet, helmPose, scale, fade);

    const V swordStart = transformed({18.f, 2.f}, bodyPose, scale);
    Pose swordPose{swordStart, bodyPose.angle};
    if (t >= 0.34f)
    {
        const float u = ease((t - 0.34f) / 1.08f);
        swordPose.position = {cx + 90.f + 150.f * u, top + 290.f + 162.f * u - 38.f * std::sin(3.14159265f * u)};
        swordPose.angle = 0.15f + 1.42f * u;
    }
    drawBlocks(target, sword, swordPose, scale, fade);

    const V shieldStart = transformed({-19.f, 1.f}, bodyPose, scale);
    Pose shieldPose{shieldStart, bodyPose.angle};
    if (t >= 0.38f)
    {
        const float u = ease((t - 0.38f) / 0.93f);
        shieldPose.position = {cx - 95.f - 118.f * u, top + 295.f + 148.f * u - 22.f * std::sin(3.14159265f * u)};
        shieldPose.angle = -0.15f - 0.65f * u;
    }
    drawBlocks(target, shield, shieldPose, scale, fade);
}
} // namespace defeatcutscene
