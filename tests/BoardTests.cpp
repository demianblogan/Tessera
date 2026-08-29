#include "doctest/doctest.h"

#include "gameplay/Board.h"
#include "gameplay/Tetromino.h"

namespace
{
	// The O piece occupies columns (x+1, x+2) and rows (y, y+1). Five of them at
	// x = -1, 1, 3, 5, 7 fill two whole rows of the 10-wide board.
	void LockO(Board& board, int x, int y)
	{
		board.LockTetromino(Tetromino(Tetromino::Type::O, { x, y }));
	}

	void FillBottomTwoRows(Board& board)
	{
		for (int x = -1; x <= 7; x += 2)
		{
			LockO(board, x, Board::HEIGHT - 2);
		}
	}
}

TEST_CASE("a fresh board has no occupied cells")
{
	const Board board;

	for (int y = 0; y < Board::HEIGHT; y++)
	{
		for (int x = 0; x < Board::WIDTH; x++)
		{
			CHECK_FALSE(board.GetGrid()[y][x].occupied);
		}
	}
}

TEST_CASE("LockTetromino marks exactly the piece's cells, with its type")
{
	Board board;
	board.LockTetromino(Tetromino(Tetromino::Type::O, { 0, 0 }));

	// O at (0,0) -> columns 1,2 rows 0,1.
	CHECK(board.GetGrid()[0][1].occupied);
	CHECK(board.GetGrid()[0][2].occupied);
	CHECK(board.GetGrid()[1][1].occupied);
	CHECK(board.GetGrid()[1][2].occupied);
	CHECK(board.GetGrid()[0][1].tetrominoType == Tetromino::Type::O);

	CHECK_FALSE(board.GetGrid()[0][0].occupied);
	CHECK_FALSE(board.GetGrid()[2][1].occupied);
}

TEST_CASE("LockTetromino ignores cells outside the grid")
{
	Board board;

	// O at (-2, HEIGHT-1): left column is -1 (out), bottom row is HEIGHT (out).
	CHECK_NOTHROW(board.LockTetromino(Tetromino(Tetromino::Type::O, { -2, Board::HEIGHT - 1 })));

	CHECK(board.GetGrid()[Board::HEIGHT - 1][0].occupied);
}

TEST_CASE("FindFullRows reports every completely filled row and nothing else")
{
	Board board;
	FillBottomTwoRows(board);

	const std::vector<int> fullRows = board.FindFullRows();

	REQUIRE(fullRows.size() == 2);
	CHECK(fullRows[0] == Board::HEIGHT - 2);
	CHECK(fullRows[1] == Board::HEIGHT - 1);
}

TEST_CASE("FindFullRows ignores a row with a gap")
{
	Board board;

	// Fill the bottom row except columns 8-9 (drop the last O piece).
	for (int x = -1; x <= 5; x += 2)
	{
		LockO(board, x, Board::HEIGHT - 2);
	}

	CHECK(board.FindFullRows().empty());
}

TEST_CASE("ClearRows removes the given rows and drops everything above by that many")
{
	Board board;
	FillBottomTwoRows(board);

	// A marker piece at the very top: O at (-1, 0) -> columns 0,1 rows 0,1.
	LockO(board, -1, 0);

	board.ClearRows({ Board::HEIGHT - 2, Board::HEIGHT - 1 });

	// The two filled rows are gone.
	CHECK(board.FindFullRows().empty());

	// The marker fell two rows: was at rows 0-1, now at rows 2-3.
	CHECK_FALSE(board.GetGrid()[0][0].occupied);
	CHECK_FALSE(board.GetGrid()[1][0].occupied);
	CHECK(board.GetGrid()[2][0].occupied);
	CHECK(board.GetGrid()[3][0].occupied);
}

TEST_CASE("ClearRows with an empty list changes nothing")
{
	Board board;
	FillBottomTwoRows(board);

	board.ClearRows({});

	CHECK(board.FindFullRows().size() == 2);
}

TEST_CASE("CanPlace is false against a wall and against a locked cell")
{
	Board board;

	// Off the left edge.
	CHECK_FALSE(board.CanPlace(Tetromino(Tetromino::Type::O, { -3, 0 })));

	// Below the floor.
	CHECK_FALSE(board.CanPlace(Tetromino(Tetromino::Type::O, { 0, Board::HEIGHT })));

	// Overlapping a locked piece.
	board.LockTetromino(Tetromino(Tetromino::Type::O, { 3, 5 }));
	CHECK_FALSE(board.CanPlace(Tetromino(Tetromino::Type::O, { 3, 5 })));

	// A clear spot.
	CHECK(board.CanPlace(Tetromino(Tetromino::Type::O, { 3, 0 })));
}
