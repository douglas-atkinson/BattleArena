#pragma once

// Screen-ready combat log rows. A source LogEntry may occupy multiple visual rows;
// all UI scrolling counts rows, while the original log remains unchanged.

#include "Layout.hpp"
#include "ViewState.hpp"

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Text.hpp>

#include <cstddef>
#include <sstream>
#include <string>
#include <vector>

namespace loglayout
{
struct Row
{
    std::size_t entryIndex;
    std::string text;
    bool firstOfEntry;
};

inline std::vector<Row> buildRows(const std::vector<LogEntry>& entries, const sf::Font& font)
{
    constexpr unsigned fontSize = 16;
    // 42px to the message column, 22px for the scrollbar and right padding.
    const float maxTextWidth = layout::logArea.size.x - 64.f;
    const auto fits = [&](const std::string& text) {
        return sf::Text(font, text, fontSize).getLocalBounds().size.x <= maxTextWidth;
    };

    std::vector<Row> rows;
    for (std::size_t i = 0; i < entries.size(); ++i)
    {
        std::istringstream words(entries[i].text);
        std::string word, current;
        bool first = true;
        const auto emit = [&]() {
            rows.push_back({i, current, first});
            current.clear();
            first = false;
        };

        while (words >> word)
        {
            if (!current.empty() && fits(current + " " + word))
            {
                current += " " + word;
                continue;
            }
            if (!current.empty())
                emit();

            // A single very long token can still wrap instead of being truncated.
            while (word.size() > 1 && !fits(word))
            {
                std::size_t cut = 1;
                while (cut < word.size() && fits(word.substr(0, cut + 1)))
                    ++cut;
                rows.push_back({i, word.substr(0, cut), first});
                first = false;
                word.erase(0, cut);
            }
            current = word;
        }
        if (!current.empty() || first)  // include an empty source message as one row
            emit();
    }
    return rows;
}
} // namespace loglayout
