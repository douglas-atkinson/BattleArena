#pragma once

// Presentation layer for the mockup. Reads MockState, never modifies it.

#include "Art.hpp"
#include "MockData.hpp"

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Texture.hpp>

#include <array>
#include <filesystem>
#include <optional>
#include <string>

struct Assets
{
    sf::Font font;
    sf::Texture battlefield;
    sf::Texture panelNoise;
    sf::Texture cardNoise;
    std::array<sf::Texture, art::kindCount> sprites;
    std::array<sf::Texture, art::kindCount> spritesGray;
    std::array<sf::Texture, art::kindCount> portraits;
    std::array<sf::Texture, art::kindCount> portraitsGray;
    sf::Texture bonePile;
    sf::Texture skull;

    // Looks for assets/ next to the executable, then in the working directory.
    bool load(const std::filesystem::path& exeDir);
};

struct UiState
{
    sf::Vector2f mouse{-1.f, -1.f};
    int hovered = -1;                       // character index under the mouse
    int selected = -1;                      // character index clicked
    std::optional<sf::Vector2i> hoveredTile;
    std::string pending = "WA";             // unsubmitted movement command
    int logScroll = 0;                      // lines scrolled up from the newest entry
    bool draggingLogThumb = false;          // left button held on the log scrollbar thumb
    float logThumbGrabOffset = 0.f;         // mouse y minus thumb top when the drag began
};

void drawScene(sf::RenderTarget& target, const Assets& assets, const MockState& state, const UiState& ui);

// Hit testing in view coordinates.
int characterAt(sf::Vector2f point, const MockState& state);
std::optional<sf::Vector2i> tileAt(sf::Vector2f point);
bool isOverLog(sf::Vector2f point);
