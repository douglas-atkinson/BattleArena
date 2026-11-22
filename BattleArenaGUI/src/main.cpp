// BattleArena graphical interface.
//
// Builds the real demonstration Arena and draws its snapshot. Typed commands are checked
// with Arena::previewCommand(), which changes nothing; submitCommand() is never called,
// so no turn can happen yet.
//
// Command-line options (handy for reviews and documentation):
//   --screenshot <file.png>   render one frame to an image and exit
//   --select <index>          start with a character selected (0 = hero, 1-4 = monsters)
//   --hover <x> <y>           simulate the mouse at a view position (with --screenshot)
//   --log-scroll <lines>      start with the combat log scrolled up (clamped)
//   --preview <command>       start with a pending command and its live preview (never submitted)
// Visual test mode (labelled on screen as "VISUAL TEST", never the live game):
//   --test-grid <cols>x<rows> show the characters on another grid size (positions rescaled)
//   --test-log <n>            add n placeholder log lines to exercise the log scrollbar

#include "Layout.hpp"
#include "LogScroll.hpp"
#include "Renderer.hpp"
#include "ViewState.hpp"

#include "battlefactory.h"

#include <SFML/Graphics.hpp>

#include <algorithm>
#include <cctype>
#include <charconv>
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

// Parses "<cols>x<rows>", e.g. "10x10". Returns nothing if the text is malformed.
std::optional<layout::Grid> parseGrid(const std::string& text)
{
    const std::size_t x = text.find_first_of("xX");
    if (x == std::string::npos)
        return std::nullopt;
    layout::Grid grid;
    const char* begin = text.data();
    const char* end = begin + text.size();
    const auto cols = std::from_chars(begin, begin + x, grid.cols);
    const auto rows = std::from_chars(begin + x + 1, end, grid.rows);
    if (cols.ec != std::errc{} || cols.ptr != begin + x || rows.ec != std::errc{} || rows.ptr != end)
        return std::nullopt;
    return grid;
}

logscroll::Geometry logGeometry(const ViewState& state, const UiState& ui)
{
    return logscroll::geometry(static_cast<int>(state.log.size()), ui.logScroll);
}

void updateHover(UiState& ui, const ViewState& state, sf::Vector2f mouse)
{
    ui.mouse = mouse;
    ui.hovered = characterAt(mouse, state);
    ui.hoveredTile = tileAt(mouse, state);
}

// Edits the pending command and returns true if it changed. R and F are standalone
// actions; a movement key after one of them starts a new movement command. Up to 8
// characters are kept so the Arena can reject commands longer than the hero's speed.
constexpr std::size_t maxPending = 8;  // what the command box can show

bool handleKey(sf::Keyboard::Key key, UiState& ui, sf::RenderWindow& window)
{
    using K = sf::Keyboard::Key;
    const std::string before = ui.pending;
    auto move = [&](char c) {
        if (ui.pending == "R" || ui.pending == "F")
            ui.pending.clear();
        if (ui.pending.size() < maxPending)
            ui.pending += c;
    };
    switch (key)
    {
    case K::W: move('W'); break;
    case K::A: move('A'); break;
    case K::S: move('S'); break;
    case K::D: move('D'); break;
    case K::R: ui.pending = "R"; break;
    case K::F: ui.pending = "F"; break;
    case K::Backspace:
        if (!ui.pending.empty())
            ui.pending.pop_back();
        break;
    case K::Enter: ui.submitNotice = true; break;  // not enabled yet: nothing is submitted
    case K::Escape: window.close(); break;
    default: break;
    }
    if (ui.pending == before)
        return false;
    ui.submitNotice = false;
    return true;
}

// Asks the Arena about the pending command and stores what the renderer should show.
// The Arena is const here: previewCommand() is the only call, and it changes nothing.
// In the --test-grid visual test the characters are drawn at rescaled positions, so a
// preview of the real Arena would not match the picture and is not shown.
void updatePreview(ViewState& state, const Arena& arena, const std::string& pending, bool livePositions)
{
    state.preview = {};
    if (pending.empty())
        return;
    if (!livePositions)
    {
        state.preview.feedback = "Command previews are off in the test-grid visual test";
        return;
    }
    state.preview = makeCommandPreviewView(arena.previewCommand(pending), state.characters.front().tile);
}
} // namespace

int main(int argc, char** argv)
{
    std::optional<std::string> screenshotPath;
    std::optional<layout::Grid> testGrid;
    int testLogLines = 0;
    std::string initialCommand;
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
        else if (arg == "--preview" && i + 1 < argc)
        {
            initialCommand = std::string(argv[++i]).substr(0, maxPending);
            for (char& ch : initialCommand)
                ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
        }
        else if (arg == "--test-log" && i + 1 < argc)
            testLogLines = std::max(0, std::stoi(argv[++i]));
        else if (arg == "--test-grid" && i + 1 < argc)
        {
            const std::string text = argv[++i];
            const std::optional<layout::Grid> parsed = parseGrid(text);
            if (!parsed)
            {
                std::cerr << "Invalid --test-grid value '" << text << "'. Use <cols>x<rows>, for example 10x10.\n";
                return 2;
            }
            if (!layout::Grid::supports(parsed->cols, parsed->rows))
            {
                std::cerr << "Unsupported grid size " << parsed->cols << "x" << parsed->rows << ". Columns must be "
                          << layout::Grid::minCols << "-" << layout::Grid::maxCols << " and rows "
                          << layout::Grid::minRows << "-" << layout::Grid::maxRows << ".\n";
                return 2;
            }
            testGrid = *parsed;
        }
    }

    const std::filesystem::path exeDir =
        argc > 0 ? std::filesystem::absolute(argv[0]).parent_path() : std::filesystem::current_path();

    // The Arena owns the game state; the GUI only draws copies taken from its snapshot.
    Arena arena = createDefaultArena();
    ViewState state = makeViewState(arena.getSnapshot());
    if (!layout::Grid::supports(state.grid.cols, state.grid.rows))
    {
        std::cerr << "The Arena's " << state.grid.cols << "x" << state.grid.rows
                  << " battlefield does not fit the battlefield panel.\n";
        return 1;
    }
    state.log.push_back({state.round, "The battle begins!", LogKind::System});

    if (testGrid)
    {
        applyTestGrid(state, *testGrid);
        state.visualTest = std::to_string(testGrid->cols) + "x" + std::to_string(testGrid->rows) + " grid";
    }
    if (testLogLines > 0)
    {
        for (int n = 1; n <= testLogLines; ++n)
            state.log.push_back({state.round, "Placeholder log line " + std::to_string(n) + ".", LogKind::Info});
        state.visualTest += (state.visualTest.empty() ? "" : ", ") + std::to_string(testLogLines) + " placeholder log lines";
    }

    Assets assets;
    if (!assets.load(exeDir, state.grid))
        return 1;

    ui.logScroll = logscroll::clampScroll(ui.logScroll, static_cast<int>(state.log.size()));
    ui.selected = std::clamp(ui.selected, -1, static_cast<int>(state.characters.size()) - 1);
    ui.pending = initialCommand;
    updatePreview(state, arena, ui.pending, !testGrid);
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
            {
                if (handleKey(key->code, ui, window))
                    updatePreview(state, arena, ui.pending, !testGrid);
            }
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
