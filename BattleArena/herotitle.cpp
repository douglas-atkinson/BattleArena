#include <algorithm>
#include <stdexcept>
#include "herotitle.h"

std::string heroTitleToString(HeroTitle title) {
    switch (title) {
    case HeroTitle::Knight: return "Knight";
    case HeroTitle::Wizard: return "Wizard";
    case HeroTitle::Ranger: return "Ranger";
    default: return "Knight";
    }
}

HeroTitle stringToHeroTitle(const std::string& text) {
    std::string lower = text;
    std::transform(lower.begin(), lower.end(), lower.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    if (lower == "knight") return HeroTitle::Knight;
    if (lower == "wizard") return HeroTitle::Wizard;
    if (lower == "ranger") return HeroTitle::Ranger;

    throw std::invalid_argument("Invalid hero title: " + text);
}