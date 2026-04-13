#pragma once

struct Position {
	int row;
	int col;

	Position() : row(0), col(0) {}
	Position(int row, int col) : row(row), col(col) {}

	bool operator==(const Position&) const = default;

};
