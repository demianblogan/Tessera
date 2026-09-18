#pragma once

#include <SFML/System/String.hpp>

struct HighScoreEntry
{
	sf::String playerName;
	int score = 0;
	int lines = 0;
	int level = 1;
};
