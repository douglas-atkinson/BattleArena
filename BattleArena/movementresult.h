#pragma once

#include "position.h"

enum class MovementResult {
	Success,
	InvalidCommand,
	Blocked
};

// The outcome of checking a player command without carrying it out.
struct MovementValidation {
	MovementResult status;
	Position destination;  // final square if valid; otherwise the hero's current square
	bool isRest;           // true only for a valid standalone R
};