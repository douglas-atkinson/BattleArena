#pragma once

struct Position {
	int row;
	int col;

	Position() : row(0), col(0) {}
	Position(int row, int col) : row(row), col(col) {}

	friend bool operator==(Position rhs, Position lhs) {
		return rhs.row == lhs.row && rhs.col == lhs.col;
	}

	friend bool operator!=(Position rhs, Position lhs) {
		return !(rhs == lhs);
	}
};
