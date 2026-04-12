#pragma once

#include <string>

enum class HeroTitle {
    Knight,
    Wizard,
    Ranger
};

std::string heroTitleToString(HeroTitle title);
HeroTitle stringToHeroTitle(const std::string& text);
