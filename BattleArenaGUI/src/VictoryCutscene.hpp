#pragma once

// A small scripted victory vignette. Bob rides through the dungeon on a clearly horse-like
// steed and gives a discreet little wave before the usual victory plaque appears.

#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/VertexArray.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace victorycutscene
{
inline constexpr float duration = 3.5f;

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
    float angle = 0.f;
};

inline float clamp01(float t) { return std::clamp(t, 0.f, 1.f); }
inline float ease(float t)
{
    t = clamp01(t);
    return t * t * (3.f - 2.f * t);
}

inline V transformed(V point, Pose pose, float scale)
{
    const float cs = std::cos(pose.angle), sn = std::sin(pose.angle);
    return {pose.position.x + scale * (point.x * cs - point.y * sn),
            pose.position.y + scale * (point.x * sn + point.y * cs)};
}

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

// Chunky horse silhouette: intentionally simple but unmistakably equine.
inline const std::array horseBody{
    Block{-30, -14, 60, 22, C(91, 66, 42)}, Block{-28, -12, 56, 18, C(125, 92, 57)},
    Block{24, -28, 20, 16, C(91, 66, 42)}, Block{26, -26, 18, 14, C(129, 96, 62)},
    Block{38, -22, 10, 8, C(91, 66, 42)}, Block{44, -20, 5, 5, C(18, 18, 22)},
    Block{46, -30, 3, 9, C(68, 49, 31)}, Block{45, -34, 4, 6, C(68, 49, 31)},
    Block{-36, -20, 10, 12, C(68, 49, 31)}, Block{-43, -28, 12, 4, C(68, 49, 31)},
    Block{-24, 4, 8, 7, C(78, 56, 35)}, Block{-8, 4, 8, 7, C(78, 56, 35)},
    Block{12, 4, 8, 7, C(78, 56, 35)}, Block{28, 4, 8, 7, C(78, 56, 35)},
};

inline const std::array horseLeg{
    Block{-3, 0, 6, 24, C(68, 49, 31)}, Block{-2, 0, 3, 20, C(100, 71, 44)},
    Block{-4, 22, 8, 4, C(31, 28, 30)},
};

inline const std::array saddle{
    Block{-6, -18, 23, 9, C(28, 36, 55)}, Block{-4, -16, 20, 6, C(61, 93, 136)},
    Block{-3, -9, 16, 5, C(215, 172, 83)}, Block{9, -9, 4, 13, C(215, 172, 83)},
};

inline const std::array riderBody{
    Block{-8, -30, 16, 20, C(17, 23, 38)}, Block{-7, -29, 14, 18, C(150, 169, 188)},
    Block{-6, -27, 12, 14, C(59, 100, 156)}, Block{-4, -25, 8, 10, C(76, 123, 181)},
    Block{-1, -25, 2, 10, C(216, 172, 87)}, Block{-4, -41, 8, 10, C(22, 24, 33)},
    Block{-3, -40, 6, 8, C(193, 157, 121)}, Block{-3, -38, 6, 2, C(75, 68, 69)},
    Block{-6, -46, 12, 7, C(20, 26, 38)}, Block{-5, -45, 10, 6, C(178, 193, 206)},
    Block{-4, -44, 8, 2, C(224, 231, 232)}, Block{-2, -51, 3, 6, C(68, 109, 172)},
    Block{-4, -53, 4, 4, C(46, 76, 127)},
};

inline const std::array riderLegs{
    Block{-7, -10, 5, 18, C(22, 29, 43)}, Block{-6, -9, 4, 14, C(104, 133, 160)},
    Block{2, -10, 5, 18, C(22, 29, 43)}, Block{2, -9, 4, 14, C(104, 133, 160)},
    Block{-8, 6, 7, 4, C(171, 187, 202)}, Block{2, 6, 7, 4, C(171, 187, 202)},
};

inline const std::array wavingArm{
    Block{-2, 0, 4, 13, C(17, 23, 38)}, Block{-1, 1, 2, 10, C(165, 182, 197)},
    Block{-2, 11, 4, 4, C(193, 157, 121)},
};

inline const std::array reinsArm{
    Block{-2, -1, 10, 4, C(17, 23, 38)}, Block{0, 0, 6, 2, C(165, 182, 197)},
    Block{7, -1, 3, 4, C(193, 157, 121)},
};

} // namespace detail

inline void draw(sf::RenderTarget& target, sf::FloatRect battlefield, float seconds)
{
    using namespace detail;
    const float t = std::clamp(seconds, 0.f, duration);
    const float progress = ease(t / duration);
    const float fade = 1.f - ease((t - (duration - 0.35f)) / 0.35f);

    sf::RectangleShape veil({battlefield.size.x - 20.f, battlefield.size.y - 20.f});
    veil.setPosition({battlefield.position.x + 10.f, battlefield.position.y + 10.f});
    veil.setFillColor(C(6, 6, 13, static_cast<std::uint8_t>(std::lround(90.f * fade))));
    target.draw(veil);

    // Keep the entire horse-and-rider silhouette inside the dungeon panel.
    const float left = battlefield.position.x + 200.f;
    const float right = battlefield.position.x + battlefield.size.x - 215.f;
    const float y = battlefield.position.y + battlefield.size.y - 175.f;
    const float x = left + (right - left) * progress;
    const float bob = 5.f * std::sin(t * 5.4f);
    const float horseScale = 3.8f;
    const float riderScale = 3.0f;

    // Small track shadow.
    sf::RectangleShape ground({battlefield.size.x - 80.f, 12.f});
    ground.setPosition({battlefield.position.x + 40.f, battlefield.position.y + battlefield.size.y - 54.f});
    ground.setFillColor(C(0, 0, 0, static_cast<std::uint8_t>(std::lround(65.f * fade))));
    target.draw(ground);

    // A simple alternating trot, not a physics simulation.
    constexpr std::array<float, 4> hips{-22.f, -5.f, 15.f, 31.f};
    for (std::size_t i = 0; i < hips.size(); ++i)
    {
        const float phase = (i == 0 || i == 3) ? 0.f : 3.14159265f;
        const float swing = 0.19f * std::sin(t * 11.f + phase);
        drawBlocks(target, horseLeg, {{x + hips[i] * horseScale, y + 10.f * horseScale + bob}, swing},
                   horseScale, fade);
    }
    const Pose horsePose{{x, y + bob}, 0.f};
    drawBlocks(target, horseBody, horsePose, horseScale, fade);
    drawBlocks(target, saddle, {{x - 6.f, y - 33.f + bob}, 0.f}, horseScale, fade);

    const Pose riderPose{{x - 2.f, y - 62.f + bob}, 0.f};
    drawBlocks(target, riderLegs, {{riderPose.position.x, riderPose.position.y + 26.f}, 0.f}, riderScale, fade);
    drawBlocks(target, riderBody, riderPose, riderScale, fade);

    // Both arms pivot at the shoulders. Their offsets are art pixels, so scale
    // them before adding them to the rider's screen-space position.
    drawBlocks(target, reinsArm,
               {{riderPose.position.x + 9.f * riderScale, riderPose.position.y - 24.f * riderScale}, 0.22f},
               riderScale, fade);
    const float waveLift = ease((t - 1.05f) / 0.40f) * (1.f - ease((t - 2.70f) / 0.35f));
    const float waveAngle = 0.15f + (2.5f + 0.18f * std::sin(t * 12.f)) * waveLift;
    drawBlocks(target, wavingArm,
               {{riderPose.position.x - 10.f * riderScale, riderPose.position.y - 26.f * riderScale}, waveAngle},
               riderScale, fade);
}
} // namespace victorycutscene
