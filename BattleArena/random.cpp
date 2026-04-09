#include "random.h"

int Random::getUniformInt(int min, int max) {
    std::uniform_int_distribution<int> dist(min, max);
    return dist(getEngine());
}

std::mt19937& Random::getEngine() {
    static std::mt19937 engine(std::random_device{}());
    return engine;
}
