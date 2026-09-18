#pragma once

#include "Tetromino.h"

class Board;

// The Tetris Guideline "three-corner rule" for recognising a T-spin. A pure
// function of board state, deliberately kept outside GameplaySession (like
// KickData/PieceData) so it can be unit-tested against hand-built boards
// instead of needing a real, randomly-dealt game to stumble into one.
namespace TSpinRule
{
	enum class Result
	{
		None,
		Mini,
		Full
	};

	// `piece` is the T resting at its final position, about to lock.
	// `isLastActionRotation` must be true -- a T-spin is only ever the payoff
	// of a rotation, never of sliding or falling into place -- or this always
	// returns None. Also always None for anything other than a T piece.
	[[nodiscard]] Result DetectTSpin(const Board& board, const Tetromino& piece, bool isLastActionRotation);
}
