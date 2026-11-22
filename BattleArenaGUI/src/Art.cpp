#include "Art.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>

namespace art
{
namespace
{
using C = sf::Color;

std::uint32_t hash3(std::uint32_t x, std::uint32_t y, std::uint32_t s)
{
    std::uint32_t h = x * 374761393u + y * 668265263u + s * 2246822519u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

std::uint8_t clampByte(int v)
{
    return static_cast<std::uint8_t>(std::clamp(v, 0, 255));
}

C shade(C c, int d)
{
    return C(clampByte(c.r + d), clampByte(c.g + d), clampByte(c.b + d), c.a);
}

C mix(C a, C b, float t)
{
    auto l = [t](std::uint8_t x, std::uint8_t y) { return clampByte(static_cast<int>(std::lround(x + (y - x) * t))); };
    return C(l(a.r, b.r), l(a.g, b.g), l(a.b, b.b), 255);
}

std::string expandRow(const std::string& row, unsigned width, const char* name)
{
    if (row.size() == width)
        return row;
    const std::size_t half = (width + 1) / 2;
    if (row.size() == half)
    {
        std::string out = row;
        const int start = (width % 2 == 1) ? static_cast<int>(half) - 2 : static_cast<int>(half) - 1;
        for (int i = start; i >= 0; --i)
            out += row[static_cast<std::size_t>(i)];
        return out;
    }
    std::cerr << "[art] " << name << ": row \"" << row << "\" has length " << row.size() << ", expected " << width
              << " or " << half << "\n";
    std::string out = row;
    out.resize(width, '.');
    return out;
}

// ---------------------------------------------------------------------------
// Battlefield sprites (15x15, drawn at 2x into 30x30 tiles)
// ---------------------------------------------------------------------------

PixelDef knightSprite()
{
    return {15, 15,
            {{'k', C(18, 18, 26)}, {'W', C(226, 234, 240)}, {'L', C(168, 188, 204)}, {'M', C(112, 134, 154)},
             {'D', C(64, 76, 92)}, {'v', C(8, 8, 14)}, {'B', C(74, 120, 184)}, {'b', C(42, 66, 120)},
             {'G', C(214, 168, 84)}, {'s', C(200, 206, 214)}},
            {
                ".....kkkkk.....",
                "....kLWLLMk....",
                "....kLLLLMk..k.",
                "....kvvvvvk.ksk",
                "....kMLLLDk.ksk",
                "..kkkkDDDkkkksk",
                ".kLLkBBGBBkMksk",
                "kLWLkBGGGBkMkGk",
                "kLBLkBBGBBkkGGG",
                "kLLLkBBBBBk.kDk",
                ".kLkkbbbbbk..k.",
                "..k.kDDkDDk....",
                "....kMDkMDk....",
                "....kMMkMMk....",
                "....kkk.kkk....",
            }};
}

PixelDef goblinSprite()
{
    return {15, 15,
            {{'k', C(16, 22, 10)}, {'L', C(150, 198, 96)}, {'M', C(96, 150, 64)}, {'D', C(56, 96, 40)},
             {'Y', C(250, 220, 70)}, {'w', C(236, 230, 200)}, {'H', C(118, 82, 50)}, {'h', C(78, 54, 32)}},
            {
                "........",
                "........",
                "....kkkk",
                "k..kMMMM",
                "kLkkMLLL",
                ".kMMkYkL",
                "..kMLLLL",
                "..kMkwkw",
                "...kMMMM",
                "..kkHHhH",
                ".kMkHhHH",
                ".kMkHHHH",
                "..kkhHHH",
                "...kDk..",
                "...kk...",
            }};
}

PixelDef ogreSprite()
{
    return {15, 15,
            {{'k', C(24, 16, 12)}, {'L', C(196, 156, 110)}, {'M', C(154, 114, 80)}, {'D', C(106, 76, 54)},
             {'R', C(230, 60, 40)}, {'w', C(236, 226, 190)}, {'F', C(96, 74, 52)}, {'f', C(64, 48, 34)}},
            {
                "...kkkkk",
                "..kMLLLL",
                ".kMLLLLL",
                ".kMDkkDL",
                ".kMkRkMM",
                ".kMMMMDk",
                ".kMwkkkk",
                "kkkDwMMM",
                "kMkkDDDD",
                "kMLkFFfF",
                "kMLkFfFF",
                "kMkkFFFF",
                ".kkFFfFF",
                "..kDDDk.",
                "..kkkkk.",
            }};
}

PixelDef skeletonSprite()
{
    return {15, 15,
            {{'k', C(20, 18, 16)}, {'L', C(226, 218, 196)}, {'M', C(176, 166, 144)}, {'v', C(10, 8, 8)},
             {'R', C(170, 70, 50)}},
            {
                "........",
                ".....kkk",
                "....kLLL",
                "....kvvL",
                "....kLLv",
                ".....kLk",
                "..kkkkML",
                ".kLkLLLL",
                ".kMkkkkL",
                ".kLkLLLL",
                "..kkkkkL",
                "....kLLL",
                "....kLk.",
                "....kMk.",
                "....kk..",
            }};
}

PixelDef batSprite()
{
    return {15, 15,
            {{'k', C(16, 10, 18)}, {'M', C(112, 80, 108)}, {'D', C(70, 48, 70)}, {'P', C(96, 62, 92)},
             {'p', C(60, 38, 60)}, {'R', C(240, 70, 60)}, {'w', C(240, 236, 220)}},
            {
                "........",
                "........",
                "........",
                "k....k..",
                "kPk..kMM",
                "kPPkkMMM",
                "kpPPkRMM",
                "kpPPkMwM",
                "kppPPkMM",
                "kpkpPPkD",
                "kk.kpPkk",
                "k...kpk.",
                ".....k..",
                "........",
                "........",
            }};
}

PixelDef bonePile()
{
    return {15, 15,
            {{'k', C(20, 18, 16)}, {'L', C(214, 206, 184)}, {'M', C(160, 150, 130)}, {'v', C(10, 8, 8)}},
            {
                "...............",
                "...............",
                "...............",
                "...............",
                "...............",
                "...............",
                "...............",
                "......kkk......",
                "..kk.kLLLk.....",
                ".kLLkkvLvk.kk..",
                "..kkkLLLLkkLLk.",
                ".kLLLLkMkLLkk..",
                "..kkkkLLLLLLk..",
                "......kkkkkk...",
                "...............",
            }};
}

// ---------------------------------------------------------------------------
// Portraits (28x28, drawn at 4x => 112x112)
// ---------------------------------------------------------------------------

PixelDef knightPortrait()
{
    return {28, 28,
            {{'k', C(16, 16, 24)}, {'W', C(228, 236, 242)}, {'L', C(164, 186, 202)}, {'M', C(108, 132, 152)},
             {'D', C(62, 74, 90)}, {'v', C(6, 6, 10)}, {'B', C(74, 118, 180)}, {'b', C(40, 62, 112)},
             {'G', C(214, 166, 82)}, {'g', C(136, 98, 46)}},
            {
                "............bB",
                "...........bBB",
                "...........bBB",
                "..........kbBB",
                ".......kkkkkGG",
                ".....kkLLLLLGG",
                "....kLLWWLLLGG",
                "....kLWWLLLLGG",
                "...kMLWLLLLLGG",
                "...kMLLLLLLLGG",
                "...kMLLLLLLLGG",
                "...kGGGGGGGGGG",
                "...kggggggggGG",
                "...kMLLLLLLLGG",
                "...kMvvvvvvvGG",
                "...kMvvvvvvvGG",
                "...kMLLLLLLMGG",
                "...kMLLkLkLMGG",
                "...kMLLLkLkMGG",
                "...kDMLLLLLMGG",
                "....kDMLLLLMGg",
                "....kDMMMMMMGg",
                ".....kDDDDDDDD",
                "..kkkkkkDDDDDD",
                ".kMLLWLLkDMMMM",
                "kMLLWWLLkDBBBB",
                "kMLLLLLLkBBBBG",
                "kMMMMMMMkBBBGG",
            }};
}

PixelDef goblinPortrait()
{
    return {28, 28,
            {{'k', C(16, 22, 10)}, {'D', C(54, 92, 40)}, {'M', C(90, 144, 62)}, {'L', C(140, 190, 92)},
             {'Y', C(250, 220, 70)}, {'w', C(236, 230, 200)}, {'m', C(96, 22, 22)}, {'H', C(110, 76, 46)},
             {'h', C(70, 48, 28)}},
            {
                "..............",
                "..............",
                "........kkkkkk",
                "......kkhhhhhh",
                ".....khHhHHhHH",
                ".....kHhHHHHHH",
                "k...kMMMMMMMMM",
                "kLk.kMLLLLLLLL",
                "kMLkkMLLLLLLLL",
                ".kMLkMLLLLLLLL",
                ".kDMLLLkkkkLLL",
                "..kDMLkYYYkLLL",
                "...kDMkYkkkLMM",
                "....kMLkkkLLMD",
                "....kMLLLLLMDD",
                "....kMMLLLLLkk",
                "....kDkkkkkkkk",
                "....kDkwmwmwmw",
                "....kDkmmmmmmm",
                "....kDkwkwmmmm",
                ".....kDkkkkkkk",
                ".....kDDMMMMMM",
                "......kkDDDDDD",
                ".......kkkkkkk",
                "....kkhhhhhhhh",
                "..khHHhHHHHHhH",
                ".kHHHhHHHHHhHH",
                "kHHHHHhHHHHHHH",
            }};
}

PixelDef ogrePortrait()
{
    return {28, 28,
            {{'k', C(24, 16, 12)}, {'D', C(110, 80, 58)}, {'M', C(152, 112, 78)}, {'L', C(190, 150, 106)},
             {'R', C(232, 56, 40)}, {'w', C(236, 226, 190)}, {'m', C(64, 20, 16)}, {'F', C(96, 74, 52)},
             {'f', C(62, 46, 32)}},
            {
                "..............",
                ".......kkkkkkk",
                ".....kkMMMMMMM",
                "....kMMLLLLLLL",
                "...kMLLLLLLLLL",
                "...kMLLLLLLLLL",
                "..kMLLLLLLLLLL",
                "..kMLLLLLLLLLL",
                "..kMMDDDDDDLLL",
                "..kMDDDDDDDDML",
                "..kDkkkkkkkDML",
                ".kkDMkRRkMMMLL",
                "kMkDMMkkMMMLLD",
                "kMkDMMMMMMMLDD",
                "kMkDMMMMMMLDkD",
                ".kkDMMMMMMMDDD",
                "..kDMkwkMMMMMM",
                "..kDMkwwkkkkkk",
                "..kDMkwwmmmmmm",
                "..kDDMkkkkkkkk",
                "..kDDMMMMMMMMM",
                "...kDDMMMMMMMM",
                "...kkDDDDDDDDD",
                ".....kkkkkkkkk",
                "..kkfFFFFkkFFF",
                ".kfFFfFFFFFFfF",
                "kfFFFFFfFFFFFF",
                "kFFfFFFFFFFFFF",
            }};
}

PixelDef skeletonPortrait()
{
    return {28, 28,
            {{'k', C(20, 18, 16)}, {'D', C(120, 112, 96)}, {'M', C(180, 170, 148)}, {'L', C(228, 220, 198)},
             {'v', C(10, 8, 8)}, {'R', C(214, 64, 44)}},
            {
                "..............",
                "........kkkkkk",
                "......kkLLLLLL",
                ".....kLLLLLLLL",
                "....kLLLLLLLLL",
                "...kMLLLLLLLLL",
                "...kMLLLLLLLLL",
                "..kMLLLLLLLLLL",
                "..kMLLLLLLLLLL",
                "..kMLLLLLLLLLL",
                "..kMLvvvvvLLLL",
                "..kMvvvvvvvLLL",
                "..kMvvvRvvvLLL",
                "..kMvvvvvvvMLL",
                "...kMvvvvvMLLL",
                "...kDMMMMMMLLv",
                "....kDMMMMMLvv",
                "....kDMMMMMMMM",
                ".....kkkkkkkkk",
                ".....kLkLkLkLk",
                ".....kkkkkkkkk",
                "......kLkLkLkL",
                "......kkkkkkkk",
                ".......kDDDDDD",
                "........kkkkkk",
                "............kM",
                "....kkkkkkkkkM",
                "...kMMMMMMMkkM",
            }};
}

PixelDef batPortrait()
{
    return {28, 28,
            {{'k', C(16, 10, 18)}, {'D', C(66, 44, 66)}, {'M', C(112, 80, 108)}, {'L', C(150, 116, 142)},
             {'n', C(196, 120, 130)}, {'P', C(96, 62, 92)}, {'p', C(58, 36, 58)}, {'R', C(244, 70, 60)},
             {'W', C(255, 220, 200)}, {'w', C(242, 238, 224)}, {'m', C(70, 16, 22)}},
            {
                "..............",
                "..............",
                "......kk......",
                "......kLk.....",
                "......kLnk....",
                "......kLnMk...",
                "kk....kLnMMkkk",
                "kPk...kMMMMMMM",
                "kPPk.kMMMMMMMM",
                "kPPPkMMMMMMMMM",
                "kpPPkMkkkkMMMM",
                "kpPPkMkRRRkMMM",
                "kppPkMkRWRkMMM",
                "kpkPkMMkkkMMMM",
                "kpkPPkMMMMMMnn",
                "kpkPPkMMMMMMkk",
                "kpPkPPkMMMkkkk",
                "kpPkPPkMMMkwkm",
                "kppPkPkDMMMkwk",
                "kppPkPPkDMMMkk",
                "kpkkPPPkDDMMMM",
                "kk.kpPPkDMMMMM",
                "k...kpPkkDMMMM",
                ".....kpk.kDDDD",
                "......kk..kkkk",
                "..............",
                "..............",
                "..............",
            }};
}

// ---------------------------------------------------------------------------
// Misc
// ---------------------------------------------------------------------------

PixelDef skullIcon()
{
    return {11, 9,
            {{'k', C(16, 12, 10)}, {'L', C(236, 228, 206)}, {'v', C(30, 8, 8)}},
            {
                "..kkkk",
                ".kLLLL",
                "kLLLLL",
                "kLvvLL",
                "kLvvLL",
                "kLLLLv",
                ".kLLLL",
                "..kLkL",
                "..kkkk",
            }};
}

PixelDef torch()
{
    return {7, 14,
            {{'k', C(14, 12, 12)}, {'Y', C(255, 232, 120)}, {'O', C(240, 140, 40)}, {'R', C(186, 54, 28)},
             {'W', C(110, 76, 44)}, {'I', C(84, 84, 90)}},
            {
                "....",
                "...Y",
                "..YY",
                ".OYY",
                "ROYY",
                "ROOY",
                ".ROO",
                "kIII",
                "..kW",
                "..kW",
                "IIkW",
                "..kW",
                "..kk",
                "....",
            }};
}

sf::Color portraitBackdrop(Kind kind)
{
    switch (kind)
    {
    case Kind::Knight: return C(44, 70, 96);
    case Kind::Goblin: return C(52, 72, 36);
    case Kind::Ogre: return C(92, 58, 34);
    case Kind::Skeleton: return C(60, 52, 72);
    case Kind::Bat: return C(84, 34, 40);
    }
    return C(50, 50, 50);
}

PixelDef spriteDef(Kind kind)
{
    switch (kind)
    {
    case Kind::Knight: return knightSprite();
    case Kind::Goblin: return goblinSprite();
    case Kind::Ogre: return ogreSprite();
    case Kind::Skeleton: return skeletonSprite();
    case Kind::Bat: return batSprite();
    }
    return knightSprite();
}

PixelDef portraitDef(Kind kind)
{
    switch (kind)
    {
    case Kind::Knight: return knightPortrait();
    case Kind::Goblin: return goblinPortrait();
    case Kind::Ogre: return ogrePortrait();
    case Kind::Skeleton: return skeletonPortrait();
    case Kind::Bat: return batPortrait();
    }
    return knightPortrait();
}

const char* kindName(Kind kind)
{
    static constexpr std::array<const char*, kindCount> names{"knight", "goblin", "ogre", "skeleton", "bat"};
    return names[static_cast<std::size_t>(kind)];
}
} // namespace

sf::Image buildImage(const PixelDef& def, const char* debugName)
{
    sf::Image img({def.width, def.height}, sf::Color::Transparent);
    if (def.rows.size() > def.height)
        std::cerr << "[art] " << debugName << ": too many rows (" << def.rows.size() << ")\n";
    for (unsigned y = 0; y < def.height && y < def.rows.size(); ++y)
    {
        const std::string row = expandRow(def.rows[y], def.width, debugName);
        for (unsigned x = 0; x < def.width; ++x)
        {
            const char ch = row[x];
            if (ch == '.')
                continue;
            const auto it = def.palette.find(ch);
            if (it == def.palette.end())
            {
                std::cerr << "[art] " << debugName << ": unknown palette key '" << ch << "'\n";
                continue;
            }
            img.setPixel({x, y}, it->second);
        }
    }
    return img;
}

sf::Image toGrayscale(const sf::Image& src, float brightness)
{
    sf::Image out = src;
    const auto size = src.getSize();
    for (unsigned y = 0; y < size.y; ++y)
        for (unsigned x = 0; x < size.x; ++x)
        {
            const sf::Color c = src.getPixel({x, y});
            const float lum = (0.299f * c.r + 0.587f * c.g + 0.114f * c.b) * brightness;
            const std::uint8_t v = clampByte(static_cast<int>(lum));
            out.setPixel({x, y}, sf::Color(v, v, clampByte(v + 4), c.a));
        }
    return out;
}

sf::Image buildSprite(Kind kind)
{
    return buildImage(spriteDef(kind), kindName(kind));
}

sf::Image buildPortrait(Kind kind)
{
    const PixelDef def = portraitDef(kind);
    const sf::Image figure = buildImage(def, kindName(kind));

    // Stepped, ordered-dither backdrop: light at the top fading into shadow.
    static constexpr int bayer[2][2] = {{0, 2}, {3, 1}};
    const sf::Color top = portraitBackdrop(kind);
    const sf::Color bottom = shade(mix(top, C(10, 10, 12), 0.75f), 0);
    sf::Image img({def.width, def.height}, bottom);
    const int bands = 4;
    for (unsigned y = 0; y < def.height; ++y)
    {
        const float t = static_cast<float>(y) / static_cast<float>(def.height - 1) * static_cast<float>(bands);
        const int band = std::min(static_cast<int>(t), bands - 1);
        const float frac = t - static_cast<float>(band);
        for (unsigned x = 0; x < def.width; ++x)
        {
            const float threshold = (static_cast<float>(bayer[y % 2][x % 2]) + 0.5f) / 4.f;
            const int b = (frac > threshold && band < bands - 1) ? band + 1 : band;
            img.setPixel({x, y}, mix(top, bottom, static_cast<float>(b) / static_cast<float>(bands - 1)));
        }
    }
    // A faint vignette corner pixel pattern keeps the frame from looking flat.
    for (unsigned i = 0; i < 3; ++i)
    {
        img.setPixel({i, 0}, C(8, 8, 10));
        img.setPixel({0, i}, C(8, 8, 10));
        img.setPixel({def.width - 1 - i, 0}, C(8, 8, 10));
        img.setPixel({def.width - 1, i}, C(8, 8, 10));
    }
    (void)img.copy(figure, {0, 0}, {}, true);
    return img;
}

sf::Image buildBonePile()
{
    return buildImage(bonePile(), "bone pile");
}

sf::Image buildSkull()
{
    return buildImage(skullIcon(), "skull");
}

sf::Image buildPanelNoise(unsigned size, sf::Color base, unsigned seed)
{
    sf::Image img({size, size}, base);
    for (unsigned y = 0; y < size; ++y)
        for (unsigned x = 0; x < size; ++x)
        {
            const std::uint32_t h = hash3(x, y, seed);
            int d = static_cast<int>(h % 7) - 3;
            if (h % 37 == 0)
                d -= 6;
            if (h % 53 == 0)
                d += 5;
            img.setPixel({x, y}, shade(base, d));
        }
    return img;
}

sf::Image buildBattlefield(const BattlefieldSpec& s)
{
    const int W = static_cast<int>(s.panelSize.x);
    const int H = static_cast<int>(s.panelSize.y);
    const int gx = static_cast<int>(s.gridOrigin.x);
    const int gy = static_cast<int>(s.gridOrigin.y);
    const int T = static_cast<int>(s.tile);
    const int gw = static_cast<int>(s.cols) * T;
    const int gh = static_cast<int>(s.rows) * T;
    const int f = static_cast<int>(s.frame);
    const int fx0 = gx - f, fy0 = gy - f, fx1 = gx + gw + f, fy1 = gy + gh + f;  // frame rect, exclusive end

    sf::Image img(s.panelSize, sf::Color::Black);
    auto put = [&](int x, int y, sf::Color c) {
        if (x >= 0 && y >= 0 && x < W && y < H)
            img.setPixel({static_cast<unsigned>(x), static_cast<unsigned>(y)}, c);
    };

    // Torch placement on the side walls (top-left corner of the 7x14 torch).
    const int leftTorchX = fx0 / 2 - 3;
    const int rightTorchX = (fx1 + W) / 2 - 3;
    const std::array<sf::Vector2i, 4> torches{{{leftTorchX, 54}, {leftTorchX, 184}, {rightTorchX, 54}, {rightTorchX, 184}}};

    // 1. Dark brick back wall around the arena, warmed by torchlight.
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x)
        {
            if (x >= fx0 && x < fx1 && y >= fy0 && y < fy1)
                continue;
            const int bh = 7, bw = 14;
            const int row = y / bh;
            const int xo = x + ((row % 2) ? bw / 2 : 0);
            const int col = xo / bw;
            const bool mortar = (y % bh == bh - 1) || (xo % bw == bw - 1);
            sf::Color c;
            if (mortar)
                c = C(16, 15, 14);
            else
            {
                const std::uint32_t bhsh = hash3(static_cast<std::uint32_t>(col), static_cast<std::uint32_t>(row), s.seed + 3);
                const int g = 34 + static_cast<int>(bhsh % 12);
                c = C(clampByte(g + 2), clampByte(g), clampByte(g - 3));
                c = shade(c, static_cast<int>(hash3(static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y), s.seed + 4) % 5) - 2);
                if (y % bh == 0)
                    c = shade(c, 6);
            }
            float glow = 0.f;
            for (const auto& t : torches)
            {
                const float dx = static_cast<float>(x - (t.x + 3));
                const float dy = static_cast<float>(y - (t.y + 4));
                const float d = std::sqrt(dx * dx + dy * dy);
                glow = std::max(glow, std::max(0.f, 1.f - d / 40.f));
            }
            glow *= glow;
            c = C(clampByte(c.r + static_cast<int>(70 * glow)), clampByte(c.g + static_cast<int>(36 * glow)),
                  clampByte(c.b + static_cast<int>(8 * glow)));
            put(x, y, c);
        }

    // 2. Heavy stone frame around the play area.
    for (int y = fy0; y < fy1; ++y)
        for (int x = fx0; x < fx1; ++x)
        {
            if (x >= gx && x < gx + gw && y >= gy && y < gy + gh)
                continue;
            const int lx = x - fx0, ly = y - fy0;
            const int fw = fx1 - fx0, fh = fy1 - fy0;
            const bool horizontal = ly < f || ly >= fh - f;
            int bx, by, blockW, blockH, id;
            if (horizontal)
            {
                blockW = 16;
                blockH = f;
                const int off = (ly < f) ? 0 : 8;
                bx = (lx + off) % blockW;
                by = (ly < f) ? ly : ly - (fh - f);
                id = (lx + off) / blockW + (ly < f ? 0 : 100);
            }
            else
            {
                blockW = f;
                blockH = 12;
                bx = (lx < f) ? lx : lx - (fw - f);
                by = (ly - f) % blockH;
                id = (ly - f) / blockH + (lx < f ? 200 : 300);
            }
            const std::uint32_t h = hash3(static_cast<std::uint32_t>(id), 7u, s.seed);
            int g = 84 + static_cast<int>(h % 18) - 9;
            sf::Color c(clampByte(g + 4), clampByte(g + 2), clampByte(g - 6));
            int d = static_cast<int>(hash3(static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y), s.seed + 9) % 7) - 3;
            if (bx == 0 || by == 0)
                d += 20;
            if (bx == blockW - 2 || by == blockH - 2)
                d -= 16;
            c = shade(c, d);
            if (bx == blockW - 1 || by == blockH - 1)
                c = C(22, 20, 18);
            const bool outerEdge = lx == 0 || ly == 0 || lx == fw - 1 || ly == fh - 1;
            if (outerEdge)
                c = C(12, 11, 10);
            put(x, y, c);
        }
    // Inner lip where the frame meets the floor.
    for (int x = gx - 1; x <= gx + gw; ++x)
    {
        put(x, gy - 1, C(14, 13, 12));
        put(x, gy + gh, C(120, 114, 100));
    }
    for (int y = gy - 1; y <= gy + gh; ++y)
    {
        put(gx - 1, y, C(14, 13, 12));
        put(gx + gw, y, C(110, 104, 92));
    }

    // 3. Weathered flagstone floor.
    for (int r = 0; r < static_cast<int>(s.rows); ++r)
        for (int c = 0; c < static_cast<int>(s.cols); ++c)
        {
            const std::uint32_t th = hash3(static_cast<std::uint32_t>(c), static_cast<std::uint32_t>(r), s.seed);
            const int g = 66 + static_cast<int>(th % 15) - 7;
            const sf::Color base(clampByte(g + 6), clampByte(g + 4), clampByte(g - 3));
            const int split = static_cast<int>((th >> 4) % 6);  // 0: horizontal split, 1: vertical split
            const bool crack = ((th >> 8) % 9) == 0;
            const bool moss = ((th >> 12) % 11) == 0;
            const bool dirt = ((th >> 16) % 7) == 0;

            std::array<std::array<sf::Color, 15>, 15> px{};
            std::array<std::array<bool, 15>, 15> isMortar{};
            for (int ty = 0; ty < T; ++ty)
                for (int tx = 0; tx < T; ++tx)
                {
                    const int x = gx + c * T + tx, y = gy + r * T + ty;
                    const bool mortar = tx == T - 1 || ty == T - 1 || (split == 0 && ty == 7) || (split == 1 && tx == 7);
                    isMortar[ty][tx] = mortar;
                    if (mortar)
                    {
                        px[ty][tx] = C(28, 26, 24);
                        continue;
                    }
                    const std::uint32_t ph = hash3(static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y), s.seed + 1);
                    int d = static_cast<int>(ph % 9) - 4;
                    if (ph % 31 == 0)
                        d -= 14;
                    const bool hi = tx == 0 || ty == 0 || (split == 0 && ty == 8) || (split == 1 && tx == 8);
                    const bool lo = tx == T - 2 || ty == T - 2 || (split == 0 && ty == 6) || (split == 1 && tx == 6);
                    if (hi)
                        d += 13;
                    if (lo)
                        d -= 12;
                    px[ty][tx] = shade(base, d);
                }
            if (crack)
            {
                int cx = 2 + static_cast<int>(th % 10), cy = 2 + static_cast<int>((th >> 3) % 10);
                std::uint32_t walk = th;
                for (int i = 0; i < 9; ++i)
                {
                    if (!isMortar[cy][cx])
                    {
                        px[cy][cx] = C(32, 30, 27);
                        if (cy + 1 < T && !isMortar[cy + 1][cx])
                            px[cy + 1][cx] = shade(px[cy + 1][cx], 10);
                    }
                    walk = walk * 1103515245u + 12345u;
                    cx = std::clamp(cx + static_cast<int>((walk >> 16) % 3) - 1, 1, T - 3);
                    cy = std::clamp(cy + static_cast<int>((walk >> 20) % 2), 1, T - 3);
                }
            }
            if (moss)
            {
                const int cx = ((th >> 20) & 1) ? 1 : T - 3;
                const int cy = ((th >> 21) & 1) ? 1 : T - 3;
                for (int ty = 0; ty < T; ++ty)
                    for (int tx = 0; tx < T; ++tx)
                    {
                        const int dx = tx - cx, dy = ty - cy;
                        if (dx * dx + dy * dy > 20 || isMortar[ty][tx])
                            continue;
                        const std::uint32_t mh = hash3(static_cast<std::uint32_t>(tx + c * 31), static_cast<std::uint32_t>(ty + r * 17), s.seed + 5);
                        if (mh % 3 != 0)
                            px[ty][tx] = (mh % 2) ? C(70, 94, 50) : C(54, 76, 40);
                    }
            }
            if (dirt)
            {
                std::uint32_t dh = th ^ 0x9e3779b9u;
                for (int i = 0; i < 12; ++i)
                {
                    dh = dh * 1664525u + 1013904223u;
                    const int tx = static_cast<int>((dh >> 8) % 14), ty = static_cast<int>((dh >> 16) % 14);
                    if (!isMortar[ty][tx])
                        px[ty][tx] = (i % 2) ? C(90, 74, 54) : C(76, 62, 46);
                }
            }
            for (int ty = 0; ty < T; ++ty)
                for (int tx = 0; tx < T; ++tx)
                {
                    const int x = gx + c * T + tx, y = gy + r * T + ty;
                    // The frame casts a short shadow onto the top and left of the floor.
                    const int shadow = std::max(std::max(0, 4 - (y - gy)), std::max(0, 3 - (x - gx))) * 9;
                    put(x, y, shade(px[ty][tx], -shadow));
                }
        }

    // 4. Torches on the side walls.
    const sf::Image torchImg = buildImage(torch(), "torch");
    for (const auto& t : torches)
        (void)img.copy(torchImg, {static_cast<unsigned>(t.x), static_cast<unsigned>(t.y)}, {}, true);

    return img;
}
} // namespace art
