// BattleArena graphical interface.
//
// Builds the real demonstration Arena and draws its snapshot. Typed commands are checked
// with Arena::previewCommand(), which changes nothing; Enter sends the command to
// Arena::submitCommand(), which plays one complete round. The GUI never applies game
// rules itself: everything it shows comes from the Arena's snapshots and messages.
//
// Command-line options (handy for reviews and documentation):
//   --screenshot <file.png>   render one frame to an image and exit
//   --select <index>          start with a character selected (0 = hero, 1-4 = monsters)
//   --hover <x> <y>           simulate the mouse at a view position (with --screenshot)
//   --log-scroll <lines>      start with the combat log scrolled up (clamped)
//   --preview <command>       start with a pending command and its live preview (never submitted)
// Visual test mode (labelled on screen as "VISUAL TEST", never the live game; turns are disabled):
//   --test-grid <cols>x<rows> show the characters on another grid size (positions rescaled)
//   --test-log <n>            add n placeholder log lines to exercise the log scrollbar
//   --test-defeat <seconds>   inspect a defeat cutscene or plaque frame without playing a battle

#include "CombatLogExport.hpp"
#include "Layout.hpp"
#include "LogScroll.hpp"
#include "Renderer.hpp"
#include "ViewState.hpp"

#include "battlefactory.h"

#include <SFML/Graphics.hpp>
#include <SFML/System/Clock.hpp>

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

logscroll::Geometry logGeometry(const UiState& ui)
{
    return logscroll::geometry(static_cast<int>(ui.logRows.size()), ui.logScroll);
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

bool handleKey(sf::Keyboard::Key key, UiState& ui, bool battleOver, sf::RenderWindow& window)
{
    using K = sf::Keyboard::Key;
    if (key == K::Escape)
    {
        window.close();
        return false;
    }
    if (battleOver)
        return false;  // the battle has ended: no more commands
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
    default: break;
    }
    return ui.pending != before;
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

// Enter: sends the pending command to the Arena, which validates it and, if it is legal,
// plays one complete round. A rejected command changes nothing and stays in the box for
// correction; an accepted one refreshes the whole view from the returned snapshot.
void submitPending(Arena& arena, ViewState& state, UiState& ui, const sf::Font& font)
{
    if (state.battleOver)
        return;
    if (!state.visualTest.empty())
    {
        state.preview = {"Turns are disabled in visual test mode", FeedbackTone::Neutral, std::nullopt};
        return;
    }
    if (ui.pending.empty())
    {
        state.preview = {"Type a command, then press Enter", FeedbackTone::Neutral, std::nullopt};
        return;
    }

    const TurnResult result = arena.submitCommand(ui.pending);
    if (!result.accepted)
    {
        state.preview = {result.messages.empty() ? "Command rejected." : result.messages.front(),
                         FeedbackTone::Invalid, std::nullopt};
        return;
    }

    const int previousRows = static_cast<int>(ui.logRows.size());
    applyTurnResult(state, result);
    ui.logRows = loglayout::buildRows(state.log, font);
    const int totalRows = static_cast<int>(ui.logRows.size());
    if (ui.logScroll > 0)  // hold the same visible text rows while new messages arrive
        ui.logScroll = logscroll::clampScroll(ui.logScroll + totalRows - previousRows, totalRows);
    ui.pending.clear();
    updatePreview(state, arena, ui.pending, true);
    updateHover(ui, state, ui.mouse);  // characters may have moved under the mouse
}
} // namespace

int main(int argc, char** argv)
{
    std::optional<std::string> screenshotPath;
    std::optional<layout::Grid> testGrid;
    int testLogLines = 0;
    std::optional<float> testDefeatSeconds;
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
        else if (arg == "--test-defeat" && i + 1 < argc)
        {
            const float seconds = std::stof(argv[++i]);
            if (!std::isfinite(seconds) || seconds < 0.f)
            {
                std::cerr << "--test-defeat requires a nonnegative number of seconds.\n";
                return 2;
            }
            testDefeatSeconds = seconds;
        }
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

    if (testDefeatSeconds)
    {
        // Presentation-only test: the real Arena and its state are not changed.
        state.battleOver = true;
        state.heroWon = false;
        state.characters.front().hp = 0;
        state.characters.front().alive = false;
        state.characters.front().onBattlefield = true;
        state.visualTest += (state.visualTest.empty() ? "" : ", ") + std::string("defeat cutscene");
        ui.outcomeElapsedSeconds = *testDefeatSeconds;
    }

    Assets assets;
    if (!assets.load(exeDir, state.grid))
        return 1;

    ui.logRows = loglayout::buildRows(state.log, assets.font);
    ui.logScroll = logscroll::clampScroll(ui.logScroll, static_cast<int>(ui.logRows.size()));
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
    bool enterHeld = false;
    bool f8Held = false;
    sf::Clock exportNoticeClock;
    bool exportNoticeActive = false;
    sf::Clock outcomeClock;
    bool outcomeStarted = false;
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
                if (key->code == sf::Keyboard::Key::F8)
                {
                    // Export only once per physical key press, including after battle over.
                    if (!f8Held)
                    {
                        const logexport::Result result = logexport::save(state.log);
                        ui.logExportNotice = result.message;
                        ui.logExportSucceeded = result.success;
                        exportNoticeClock.restart();
                        exportNoticeActive = true;
                    }
                    f8Held = true;
                }
                else if (key->code == sf::Keyboard::Key::Enter)
                {
                    // One submission per physical press: key-repeat events are ignored.
                    if (!enterHeld)
                    {
                        submitPending(arena, state, ui, assets.font);
                        if (!assets.prepareBattlefield(state.grid))  // no-op while the grid is unchanged
                            return 1;
                    }
                    enterHeld = true;
                }
                else if (handleKey(key->code, ui, state.battleOver, window))
                    updatePreview(state, arena, ui.pending, !testGrid);
            }
            else if (const auto* keyUp = event->getIf<sf::Event::KeyReleased>())
            {
                if (keyUp->code == sf::Keyboard::Key::Enter)
                    enterHeld = false;
                else if (keyUp->code == sf::Keyboard::Key::F8)
                    f8Held = false;
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
                        ui.logScroll = logscroll::scrollForThumbTop(logGeometry(ui), p.y - ui.logThumbGrabOffset);
                }
                updateHover(ui, state, p);
            }
            else if (event->is<sf::Event::MouseLeft>())
                updateHover(ui, state, {-1.f, -1.f});  // a drag in progress continues; release ends it
            else if (event->is<sf::Event::FocusLost>())
            {
                ui.draggingLogThumb = false;
                enterHeld = false;
                f8Held = false;
            }
            else if (const auto* click = event->getIf<sf::Event::MouseButtonPressed>())
            {
                if (click->button == sf::Mouse::Button::Left)
                {
                    const sf::Vector2f p = window.mapPixelToCoords(click->position);
                    const logscroll::Geometry sb = logGeometry(ui);
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
                    ui.logScroll = logscroll::clampScroll(ui.logScroll + lines, static_cast<int>(ui.logRows.size()));
                }
            }
        }

        // Run the cutscene and/or plaque once after a real battle outcome.
        if (state.battleOver && !outcomeStarted && !testDefeatSeconds)
        {
            outcomeClock.restart();
            outcomeStarted = true;
        }
        if (outcomeStarted)
            ui.outcomeElapsedSeconds = outcomeClock.getElapsedTime().asSeconds();
        if (exportNoticeActive && exportNoticeClock.getElapsedTime().asSeconds() >= 5.f)
        {
            ui.logExportNotice.clear();
            exportNoticeActive = false;
        }

        drawScene(window, assets, state, ui);
        window.display();
    }
    return 0;
}
