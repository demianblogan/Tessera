#include "doctest/doctest.h"

#include "gameplay/Board.h"
#include "gameplay/TSpinRule.h"
#include "gameplay/Tetromino.h"

namespace
{
	// Locks an O piece so cell (x, y) is its top-left corner -- a convenient
	// way to mark a single cell occupied; Board has no raw single-cell setter.
	void MarkOccupied(Board& board, int x, int y)
	{
		board.LockTetromino(Tetromino(Tetromino::Type::O, { x - 1, y }));
	}

	// A T piece whose centre (pivot) sits at (centreX, centreY), rotated
	// clockwise `rotation` times from spawn. The centre -- local (1, 1) -- is
	// the same board cell in every rotation state, so this never has to move
	// the piece to change its orientation.
	Tetromino MakeT(int centreX, int centreY, int rotation)
	{
		Tetromino piece(Tetromino::Type::T, { centreX - 1, centreY - 1 });
		for (int i = 0; i < rotation; i++)
		{
			piece.RotateClockwise();
		}
		return piece;
	}
}

TEST_CASE("Detect is None for a non-T piece, even with every corner filled")
{
	Board board;
	MarkOccupied(board, 4, 9);
	MarkOccupied(board, 6, 9);
	MarkOccupied(board, 4, 11);
	MarkOccupied(board, 6, 11);

	const Tetromino piece(Tetromino::Type::O, { 4, 9 });
	CHECK(TSpinRule::Detect(board, piece, true) == TSpinRule::Result::None);
}

TEST_CASE("Detect is None unless the last action was a rotation")
{
	Board board;
	MarkOccupied(board, 4, 9);
	MarkOccupied(board, 6, 9);
	MarkOccupied(board, 4, 11);
	MarkOccupied(board, 6, 11);

	const Tetromino t = MakeT(5, 10, 0);
	CHECK(TSpinRule::Detect(board, t, false) == TSpinRule::Result::None);
}

TEST_CASE("Detect is None with only two corners filled")
{
	Board board;
	// Both front corners for rotation 0 (point up) -- not enough on their own.
	MarkOccupied(board, 4, 9);
	MarkOccupied(board, 6, 9);

	const Tetromino t = MakeT(5, 10, 0);
	CHECK(TSpinRule::Detect(board, t, true) == TSpinRule::Result::None);
}

TEST_CASE("Detect is Full when both front corners are filled")
{
	Board board;
	MarkOccupied(board, 4, 9);    // TL (front for rotation 0)
	MarkOccupied(board, 6, 9);    // TR (front for rotation 0)
	MarkOccupied(board, 4, 11);   // BL (back)

	const Tetromino t = MakeT(5, 10, 0);
	CHECK(TSpinRule::Detect(board, t, true) == TSpinRule::Result::Full);
}

TEST_CASE("Detect is Mini when three corners are filled but not both front ones")
{
	Board board;
	MarkOccupied(board, 4, 9);    // TL (front)
	MarkOccupied(board, 4, 11);   // BL (back)
	MarkOccupied(board, 6, 11);   // BR (back)
	// TR, the other front corner, stays empty.

	const Tetromino t = MakeT(5, 10, 0);
	CHECK(TSpinRule::Detect(board, t, true) == TSpinRule::Result::Mini);
}

TEST_CASE("Detect treats the wall as a filled corner")
{
	Board board;

	// T pointing right (rotation 1), centre pinned to column 0 by the left
	// wall -- both back corners (column -1) are off the board.
	Tetromino t(Tetromino::Type::T, { -1, 9 });
	t.RotateClockwise();

	MarkOccupied(board, 1, 9);    // TR (front for rotation 1)
	MarkOccupied(board, 1, 11);   // BR (front for rotation 1)

	CHECK(TSpinRule::Detect(board, t, true) == TSpinRule::Result::Full);
}

TEST_CASE("Detect treats the floor as a filled corner")
{
	Board board;

	// T pointing up, centre resting on the very last row.
	const int centreY = Board::HEIGHT - 1;
	MarkOccupied(board, 4, centreY - 1);   // TL
	MarkOccupied(board, 6, centreY - 1);   // TR
	// Both bottom corners are past the floor -- filled automatically.

	const Tetromino t = MakeT(5, centreY, 0);
	CHECK(TSpinRule::Detect(board, t, true) == TSpinRule::Result::Full);
}
