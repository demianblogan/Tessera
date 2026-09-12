#include "TSpinRule.h"

#include <array>
#include <cstddef>

#include "Board.h"

namespace
{
	// The four cells diagonally adjacent to the T's centre, in TL / TR / BL / BR
	// order; which two are "front" (the side the T's point faces) depends on
	// its rotation state.
	constexpr std::array<sf::Vector2i, 4> CornerOffsets = { { { -1, -1 }, { 1, -1 }, { -1, 1 }, { 1, 1 } } };
	constexpr std::array<std::array<int, 2>, 4> FrontCornerIndices = { { { 0, 1 }, { 1, 3 }, { 2, 3 }, { 0, 2 } } };

	// Off the board -- a wall, the floor, or above the buffer -- counts as
	// filled for the corner rule, same as a locked cell.
	[[nodiscard]] bool IsFilled(const Board& board, sf::Vector2i cell)
	{
		if (cell.x < 0 || cell.x >= Board::WIDTH || cell.y < 0 || cell.y >= Board::HEIGHT)
		{
			return true;
		}

		return board.GetGrid()[cell.y][cell.x].occupied;
	}
}

TSpinRule::Result TSpinRule::Detect(const Board& board, const Tetromino& piece, bool lastActionWasRotation)
{
	if (piece.GetType() != Tetromino::Type::T || !lastActionWasRotation)
	{
		return Result::None;
	}

	// The T's centre -- local (1, 1) in every rotation state -- is always one
	// of its own occupied cells, so it doubles as the pivot the corner rule
	// checks around.
	const sf::Vector2i centre = piece.GetPosition() + sf::Vector2i{ 1, 1 };

	std::array<bool, 4> cornerFilled{};
	int totalFilled = 0;

	for (std::size_t i = 0; i < CornerOffsets.size(); ++i)
	{
		cornerFilled[i] = IsFilled(board, centre + CornerOffsets[i]);
		if (cornerFilled[i])
		{
			++totalFilled;
		}
	}

	if (totalFilled < 3)
	{
		return Result::None;
	}

	const std::array<int, 2>& front = FrontCornerIndices[static_cast<std::size_t>(piece.GetRotationIndex())];

	// Both corners on the side the T points toward filled -> a full T-spin.
	// Otherwise (three corners filled some other way) it's a Mini.
	return (cornerFilled[static_cast<std::size_t>(front[0])] && cornerFilled[static_cast<std::size_t>(front[1])])
		? Result::Full
		: Result::Mini;
}
