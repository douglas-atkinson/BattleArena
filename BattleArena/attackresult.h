#pragma once

#include <string>
#include <vector>

struct AttackResult {
	bool hit{ false };
	int damage{ 0 };
	bool targetDefeated{ false };
	std::vector<std::string> messages;  // optional descriptions; Arena prints them
};
