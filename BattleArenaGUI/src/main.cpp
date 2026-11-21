// BattleArena graphical interface mockup.
//
// A standalone SFML 3.1 application that demonstrates the proposed layout using
// hardcoded sample data. It is intentionally NOT connected to the game model.
//
// Command-line options (handy for reviews and documentation):
//   --screenshot <file.png>   render one frame to an image and exit
//   --select <index>          start with a character selected (0 = hero, 1-4 = monsters)
//   --hover <x> <y>           simulate the mouse at a view position (with --screenshot)
//   --log-scroll <lines>      start with the combat log scrolled up (clamped)
//   --log-entries <n>         keep only the n newest sample log entries

#include "Layout.hpp"
#include "LogScroll.hpp"
#include "MockData.hpp"
#include "Renderer.hpp"

#include <SFML/Graphics.hpp>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <optional>
#include <string>

namespace
{
// Letterbox the fixed 1600x900 design view inside whatever window size we have.
sf::View makeView(sf::Vector2u windowSize)
{
    sf::View view(sf::FloatRect({0.f, 0.f}, {layout::viewW, layout::viewH}));
    const float windowRatio = static_cast<float>(windowSize.x) / static_cast<float>(windowSize.y);
    const float viewRatio = layout::viewW / layout::viewH;
    if (windowRatio > viewRatio)
    {
        const float w = viewRatio / windowRatio;
        view.setViewport(sf::FloatRect({(1.f - w) / 2.f, 0.f}, {w, 1.f}));
    }
    else
    {
        const float h = windowRatio / viewRatio;
        view.setViewport(sf::FloatRect({0.f, (1.f - h) / 2.f}, {1.f, h}));
    }
    return view;
}

// Start at 1600x900, shrinking (never growing) to fit smaller displays such as a VDI session.
sf::Vector2u initialWindowSize()
{
    const sf::Vector2u desktop = sf::VideoMode::getDesktopMode().size;
    const float fit = std::min((static_cast<float>(desktop.x) - 40.f) / layout::viewW,
                               (static_cast<float>(desktop.y) - 100.f) / layout::viewH);
    const float scale = std::clamp(fit, 0.5f, 1.f);
    return {static_cast<unsigned>(layout::viewW * scale), static_cast<unsigned>(layout::viewH * scale)};
}

logscroll::Geometry logGeometry(const MockState& state, const UiState& ui)
{
    return logscroll::geometry(static_cast<int>(state.log.size()), ui.logScroll);
}

void updateHover(UiState& ui, const MockState& state, sf::Vector2f mouse)
{
    ui.mouse = mouse;
    ui.hovered = characterAt(mouse, state);
    ui.hoveredTile = tileAt(mouse);
}

void handleKey(sf::Keyboard::Key key, UiState& ui, MockState& state, sf::RenderWindow& window)
{
    using K = sf::Keyboard::Key;
    constexpr std::size_t maxPending = 8;
    auto push = [&](char c) {
        if (ui.pending.size() < maxPending)
            ui.pending += c;
    };
    switch (key)
    {
    case K::W: push('W'); break;
    case K::A: push('A'); break;
    case K::S: push('S'); break;
    case K::D: push('D'); break;
    case K::R: ui.pending = "R"; break;
    case K::Backspace:
        if (!ui.pending.empty())
            ui.pending.pop_back();
        break;
    case K::Enter:
        if (!ui.pending.empty())
        {
            // Mockup only: echo the command; validation and movement belong to the model (Arena).
            const std::string what = ui.pending == "R" ? "Bob rests (mockup: no game logic)."
                                                       : "Command " + ui.pending + " submitted (mockup: no game logic).";
            state.log.push_back({state.round, what, LogKind::System});
            ui.pending.clear();
            ui.logScroll = 0;
        }
        break;
    case K::Escape: window.close(); break;
    default: break;
    }
}
} // namespace

int main(int argc, char** argv)
{
    std::optional<std::string> screenshotPath;
    std::optional<int> logEntries;
    UiState ui;
    for (int i = 1; i < argc; ++i)
    {
        const std::string arg = argv[i];
        if (arg == "--screenshot" && i + 1 < argc)
            screenshotPath = argv[++i];
        else if (arg == "--select" && i + 1 < argc)
            ui.selected = std::stoi(argv[++i]);
        else if (arg == "--hover" && i + 2 < argc)
        {
            ui.mouse.x = std::stof(argv[++i]);
            ui.mouse.y = std::stof(argv[++i]);
        }
        else if (arg == "--log-scroll" && i + 1 < argc)
            ui.logScroll = std::stoi(argv[++i]);
        else if (arg == "--log-entries" && i + 1 < argc)
            logEntries = std::stoi(argv[++i]);
    }

    const std::filesystem::path exeDir =
        argc > 0 ? std::filesystem::absolute(argv[0]).parent_path() : std::filesystem::current_path();
    Assets assets;
    if (!assets.load(exeDir))
        return 1;

    MockState state = makeSampleState();
    if (logEntries && *logEntries >= 0 && static_cast<std::size_t>(*logEntries) < state.log.size())
        state.log.erase(state.log.begin(), state.log.end() - *logEntries);
    ui.logScroll = logscroll::clampScroll(ui.logScroll, static_cast<int>(state.log.size()));
    ui.selected = std::clamp(ui.selected, -1, static_cast<int>(state.characters.size()) - 1);
    if (ui.mouse.x >= 0.f)
        updateHover(ui, state, ui.mouse);

    if (screenshotPath)
    {
        sf::RenderTexture canvas({static_cast<unsigned>(layout::viewW), static_cast<unsigned>(layout::viewH)});
        drawScene(canvas, assets, state, ui);
        canvas.display();
        if (!canvas.getTexture().copyToImage().saveToFile(*screenshotPath))
        {
            std::cerr << "Failed to save " << *screenshotPath << "\n";
            return 1;
        }
        std::cout << "Saved " << *screenshotPath << "\n";
        return 0;
    }

    sf::RenderWindow window(sf::VideoMode(initialWindowSize()), "BattleArena",
                            sf::Style::Titlebar | sf::Style::Close | sf::Style::Resize);
    window.setFramerateLimit(60);
    window.setView(makeView(window.getSize()));

    float wheelRemainder = 0.f;
    while (window.isOpen())
    {
        while (const std::optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
                window.close();
            else if (const auto* resized = event->getIf<sf::Event::Resized>())
                window.setView(makeView(resized->size));
            else if (const auto* key = event->getIf<sf::Event::KeyPressed>())
                handleKey(key->code, ui, state, window);
            else if (const auto* moved = event->getIf<sf::Event::MouseMoved>())
            {
                const sf::Vector2f p = window.mapPixelToCoords(moved->position);
                if (ui.draggingLogThumb)
                {
                    // Safety net: if the release happened where we could not see it, end the drag.
                    if (!sf::Mouse::isButtonPressed(sf::Mouse::Button::Left))
                        ui.draggingLogThumb = false;
                    else
                        ui.logScroll = logscroll::scrollForThumbTop(logGeometry(state, ui), p.y - ui.logThumbGrabOffset);
                }
                updateHover(ui, state, p);
            }
            else if (event->is<sf::Event::MouseLeft>())
                updateHover(ui, state, {-1.f, -1.f});  // a drag in progress continues; release ends it
            else if (event->is<sf::Event::FocusLost>())
                ui.draggingLogThumb = false;
            else if (const auto* click = event->getIf<sf::Event::MouseButtonPressed>())
            {
                if (click->button == sf::Mouse::Button::Left)
                {
                    const sf::Vector2f p = window.mapPixelToCoords(click->position);
                    const logscroll::Geometry sb = logGeometry(state, ui);
                    if (sb.track.contains(p))
                    {
                        // Scrollbar clicks never change the selection.
                        if (sb.enabled() && sb.thumb.contains(p))
                        {
                            ui.draggingLogThumb = true;
                            ui.logThumbGrabOffset = p.y - sb.thumb.position.y;
                        }
                        else
                            ui.logScroll = logscroll::pageScroll(sb, ui.logScroll, p.y);
                    }
                    else
                    {
                        // Clicking only inspects; it never moves or attacks.
                        const int hit = characterAt(p, state);
                        ui.selected = (hit == ui.selected) ? -1 : hit;
                    }
                }
            }
            else if (const auto* released = event->getIf<sf::Event::MouseButtonReleased>())
            {
                if (released->button == sf::Mouse::Button::Left)
                    ui.draggingLogThumb = false;
            }
            else if (const auto* wheel = event->getIf<sf::Event::MouseWheelScrolled>())
            {
                if (isOverLog(window.mapPixelToCoords(wheel->position)))
                {
                    // One line per notch; multi-notch and fractional (touchpad) deltas accumulate.
                    wheelRemainder += wheel->delta;
                    const int lines = static_cast<int>(wheelRemainder);
                    wheelRemainder -= static_cast<float>(lines);
                    ui.logScroll = logscroll::clampScroll(ui.logScroll + lines, static_cast<int>(state.log.size()));
                }
            }
        }

        drawScene(window, assets, state, ui);
        window.display();
    }
    return 0;
}
