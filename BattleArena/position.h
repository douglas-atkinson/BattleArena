#pragma once

#include <cstdlib>

struct Position {
	int row;
	int col;

	Position() : row(0), col(0) {}
	Position(int row, int col) : row(row), col(col) {}

	bool operator==(const Position&) const = default;

};

// ---------- Utility Functions ----------

inline int manhattanDistance(const Position& a, const Position& b) {
	return std::abs(a.row - b.row) + std::abs(a.col - b.col);
}

inline bool areOrthogonallyAdjacent(const Position& a, const Position& b) {
	return manhattanDistance(a, b) == 1;
}
