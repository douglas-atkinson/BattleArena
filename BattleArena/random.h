#pragma once

#include <random>

class Random {
public:
    static int getUniformInt(int min, int max);

private:
    static std::mt19937& getEngine();
};