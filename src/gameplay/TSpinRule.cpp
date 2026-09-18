#include "TSpinRule.h"

#include <array>
#include <cstddef>

#include "Board.h"

namespace
{
	// One diagonal corner per adjacent cell around the T's center.
	constexpr std::size_t CornerCount = 4;

	// Both corners of the T's "front" -- the side its point faces -- must be
	// filled for a full T-spin (see DetectTSpin).
	constexpr std::size_t FrontCornerCount = 2;

	// Fewer than this many filled corners is not a T-spin at all.
	constexpr int MinFilledCornersForTSpin = 3;

	// The four cells diagonally adjacent to the T's center, in TL / TR / BL / BR
	// order; which two are "front" (the side the T's point faces) depends on
	// its rotation state.
	constexpr std::array<sf::Vector2i, CornerCount> CornerOffsets =
	{ {
		{ -1, -1 },
		{ 1, -1 },
		{ -1, 1 },
		{ 1, 1 }
	} };

	constexpr std::array<std::array<int, FrontCornerCount>, CornerCount> FrontCornerIndices =
	{ {
		{ 0, 1 },
		{ 1, 3 },
		{ 2, 3 },
		{ 0, 2 }
	} };

	// Off the board -- a wall, the floor, or above the buffer -- counts as
	// filled for the corner rule, same as a locked cell.
	[[nodiscard]] bool IsCellFilled(const Board& board, sf::Vector2i cell)
	{
		if (cell.x < 0 || cell.x >= Board::Width || cell.y < 0 || cell.y >= Board::Height)
			return true;

		return board.GetGrid()[cell.y][cell.x].isOccupied;
	}
}

TSpinRule::Result TSpinRule::DetectTSpin(const Board& board, const Tetromino& piece, bool isLastActionRotation)
{
	if (piece.GetType() != Tetromino::Type::T || !isLastActionRotation)
		return Result::None;

	// The T's center -- local (1, 1) in every rotation state -- is always one
	// of its own occupied cells, so it doubles as the pivot the corner rule
	// checks around.
	const sf::Vector2i center = piece.GetPosition() + sf::Vector2i{ 1, 1 };

	std::array<bool, CornerCount> cornerFilled{};
	int totalFilled = 0;

	for (std::size_t i = 0; i < CornerOffsets.size(); i++)
	{
		cornerFilled[i] = IsCellFilled(board, center + CornerOffsets[i]);
		if (cornerFilled[i])
			totalFilled++;
	}

	if (totalFilled < MinFilledCornersForTSpin)
		return Result::None;

	const std::array<int, FrontCornerCount>& front = FrontCornerIndices[static_cast<std::size_t>(piece.GetRotationIndex())];
	const bool isBothFrontCornersFilled =
		cornerFilled[static_cast<std::size_t>(front[0])] &&
		cornerFilled[static_cast<std::size_t>(front[1])];

	// Both corners on the side the T points toward filled -> a full T-spin.
	// Otherwise (three corners filled some other way) it's a Mini.
	return isBothFrontCornersFilled ? Result::Full : Result::Mini;
}
