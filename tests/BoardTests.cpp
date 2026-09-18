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
			LockO(board, x, Board::Height - 2);
		}
	}
}

TEST_CASE("the grid is the visible field plus a hidden buffer above it")
{
	CHECK(Board::VisibleHeight == 20);
	CHECK(Board::Height == Board::BufferHeight + Board::VisibleHeight);
	CHECK(Board::BufferHeight > 0);
}

TEST_CASE("IsEmpty is true for a fresh board and false once anything locks")
{
	Board board;

	CHECK(board.IsEmpty());

	LockO(board, 0, 0);
	CHECK_FALSE(board.IsEmpty());
}

TEST_CASE("IsEmpty is true again once every locked cell clears")
{
	Board board;
	FillBottomTwoRows(board);
	CHECK_FALSE(board.IsEmpty());

	board.ClearRows(board.FindFullRows());
	CHECK(board.IsEmpty());
}

TEST_CASE("a fresh board has no occupied cells")
{
	const Board board;

	for (int y = 0; y < Board::Height; y++)
	{
		for (int x = 0; x < Board::Width; x++)
		{
			CHECK_FALSE(board.GetGrid()[y][x].isOccupied);
		}
	}
}

TEST_CASE("LockTetromino marks exactly the piece's cells, with its type")
{
	Board board;
	board.LockTetromino(Tetromino(Tetromino::Type::O, { 0, 0 }));

	// O at (0,0) -> columns 1,2 rows 0,1.
	CHECK(board.GetGrid()[0][1].isOccupied);
	CHECK(board.GetGrid()[0][2].isOccupied);
	CHECK(board.GetGrid()[1][1].isOccupied);
	CHECK(board.GetGrid()[1][2].isOccupied);
	CHECK(board.GetGrid()[0][1].tetrominoType == Tetromino::Type::O);

	CHECK_FALSE(board.GetGrid()[0][0].isOccupied);
	CHECK_FALSE(board.GetGrid()[2][1].isOccupied);
}

TEST_CASE("LockTetromino ignores cells outside the grid")
{
	Board board;

	// O at (-2, Height-1): left column is -1 (out), bottom row is Height (out).
	CHECK_NOTHROW(board.LockTetromino(Tetromino(Tetromino::Type::O, { -2, Board::Height - 1 })));

	CHECK(board.GetGrid()[Board::Height - 1][0].isOccupied);
}

TEST_CASE("FindFullRows reports every completely filled row and nothing else")
{
	Board board;
	FillBottomTwoRows(board);

	const std::vector<int> fullRows = board.FindFullRows();

	REQUIRE(fullRows.size() == 2);
	CHECK(fullRows[0] == Board::Height - 2);
	CHECK(fullRows[1] == Board::Height - 1);
}

TEST_CASE("FindFullRows ignores a row with a gap")
{
	Board board;

	// Fill the bottom row except columns 8-9 (drop the last O piece).
	for (int x = -1; x <= 5; x += 2)
	{
		LockO(board, x, Board::Height - 2);
	}

	CHECK(board.FindFullRows().empty());
}

TEST_CASE("ClearRows removes the given rows and drops everything above by that many")
{
	Board board;
	FillBottomTwoRows(board);

	// A marker piece at the very top: O at (-1, 0) -> columns 0,1 rows 0,1.
	LockO(board, -1, 0);

	board.ClearRows({ Board::Height - 2, Board::Height - 1 });

	// The two filled rows are gone.
	CHECK(board.FindFullRows().empty());

	// The marker fell two rows: was at rows 0-1, now at rows 2-3.
	CHECK_FALSE(board.GetGrid()[0][0].isOccupied);
	CHECK_FALSE(board.GetGrid()[1][0].isOccupied);
	CHECK(board.GetGrid()[2][0].isOccupied);
	CHECK(board.GetGrid()[3][0].isOccupied);
}

TEST_CASE("ClearRows with an empty list changes nothing")
{
	Board board;
	FillBottomTwoRows(board);

	board.ClearRows({});

	CHECK(board.FindFullRows().size() == 2);
}

TEST_CASE("LockTetromino marks every cell Golden when asked, Normal otherwise")
{
	Board board;
	board.LockTetromino(Tetromino(Tetromino::Type::O, { 0, 0 }), true);

	CHECK(board.GetGrid()[0][1].kind == Cell::Kind::Golden);
	CHECK(board.GetGrid()[1][2].kind == Cell::Kind::Golden);

	board.LockTetromino(Tetromino(Tetromino::Type::O, { 4, 0 }));
	CHECK(board.GetGrid()[0][5].kind == Cell::Kind::Normal);
}

TEST_CASE("RowsContainGolden is true only for rows holding a golden lock")
{
	Board board;
	FillBottomTwoRows(board);

	CHECK_FALSE(board.RowsContainGolden({ Board::Height - 2, Board::Height - 1 }));

	board.LockTetromino(Tetromino(Tetromino::Type::O, { -1, 0 }), true);

	CHECK_FALSE(board.RowsContainGolden({ Board::Height - 2, Board::Height - 1 }));
	CHECK(board.RowsContainGolden({ 0, 1 }));
}

TEST_CASE("PushGarbageRow raises one row from the bottom with a single gap")
{
	Board board;

	CHECK(board.PushGarbageRow(3));

	const Board::GridRow& bottomRow = board.GetGrid()[Board::Height - 1];
	for (int x = 0; x < Board::Width; x++)
	{
		if (x == 3)
		{
			CHECK_FALSE(bottomRow[x].isOccupied);
		}
		else
		{
			CHECK(bottomRow[x].isOccupied);
			CHECK(bottomRow[x].kind == Cell::Kind::Garbage);
		}
	}
}

TEST_CASE("PushGarbageRow shifts every existing row up by one")
{
	Board board;
	LockO(board, -1, Board::Height - 2);   // bottom-left corner, rows Height-2/-1

	REQUIRE(board.PushGarbageRow(9));

	// The marker (locked at rows Height-2/-1) shifted up by exactly one row.
	CHECK(board.GetGrid()[Board::Height - 3][0].isOccupied);
	CHECK(board.GetGrid()[Board::Height - 2][0].isOccupied);
}

TEST_CASE("PushGarbageRow refuses to push locked cells off the top")
{
	Board board;
	LockO(board, -1, 0);   // occupies the very top row

	CHECK_FALSE(board.PushGarbageRow(0));

	// Untouched: the marker is still exactly where it was.
	CHECK(board.GetGrid()[0][0].isOccupied);
	CHECK(board.GetGrid()[1][0].isOccupied);
}

TEST_CASE("CanPlace is false against a wall and against a locked cell")
{
	Board board;

	// Off the left edge.
	CHECK_FALSE(board.CanPlace(Tetromino(Tetromino::Type::O, { -3, 0 })));

	// Below the floor.
	CHECK_FALSE(board.CanPlace(Tetromino(Tetromino::Type::O, { 0, Board::Height })));

	// Overlapping a locked piece.
	board.LockTetromino(Tetromino(Tetromino::Type::O, { 3, 5 }));
	CHECK_FALSE(board.CanPlace(Tetromino(Tetromino::Type::O, { 3, 5 })));

	// A clear spot.
	CHECK(board.CanPlace(Tetromino(Tetromino::Type::O, { 3, 0 })));
}
