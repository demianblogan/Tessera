#pragma once

#include <array>

#include <SFML/Graphics/Color.hpp>

#include "../gameplay/Tetromino.h"

namespace UI
{
	// The seven classic tetromino colors (I, O, T, S, Z, J, L) -- the game's
	// signature palette, shared by the title letters, the ring entries and the
	// ambient menu sparks.
	inline constexpr std::array<sf::Color, static_cast<std::size_t>(Tetromino::Type::Count)> TetrominoColors
	{
		sf::Color{ 0, 240, 240 },    // I - cyan
		sf::Color{ 245, 220, 40 },   // O - yellow
		sf::Color{ 180, 60, 240 },   // T - purple
		sf::Color{ 60, 230, 90 },    // S - green
		sf::Color{ 240, 60, 70 },    // Z - red
		sf::Color{ 70, 110, 240 },   // J - blue
		sf::Color{ 245, 160, 40 },   // L - orange
	};

	// A disabled menu entry's flat grey.
	inline constexpr sf::Color DisabledEntryColor{ 146, 150, 158 };
}
