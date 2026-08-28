#include "Board.h"

bool Board::Contains(const Tetromino& tetromino) const
{
	const auto blockPositions = tetromino.GetBlockPositions();

	for (const sf::Vector2i& blockPosition : blockPositions)
	{
		if (blockPosition.x < 0)
		{
			return false;
		}

		if (blockPosition.x >= WIDTH)
		{
			return false;
		}

		if (blockPosition.y < 0)
		{
			return false;
		}

		if (blockPosition.y >= HEIGHT)
		{
			return false;
		}
	}

	return true;
}

bool Board::IntersectsLockedCells(const Tetromino& tetromino) const
{
	const auto blockPositions = tetromino.GetBlockPositions();

	for (const sf::Vector2i& blockPosition : blockPositions)
	{
		// Out-of-bounds blocks are a wall/floor collision, which is Contains()'s
		// responsibility -- not a locked-cell overlap. Indexing grid[][] with
		// them here would read past the fixed-size arrays.
		if (blockPosition.x < 0 || blockPosition.x >= WIDTH ||
			blockPosition.y < 0 || blockPosition.y >= HEIGHT)
		{
			continue;
		}

		if (grid[blockPosition.y][blockPosition.x].occupied)
		{
			return true;
		}
	}

	return false;
}

bool Board::CanPlace(const Tetromino& tetromino) const
{
	return Contains(tetromino) && !IntersectsLockedCells(tetromino);
}

void Board::LockTetromino(const Tetromino& tetromino)
{
	const auto blockPositions =	tetromino.GetBlockPositions();

	for (const sf::Vector2i& blockPosition : blockPositions)
	{
		Cell& cell = grid[blockPosition.y][blockPosition.x];

		cell.occupied = true;
		cell.tetrominoType = tetromino.GetType();
	}
}

int Board::ClearFullRows()
{
	int clearedRowsCount = 0;

	for (int y = HEIGHT - 1; y >= 0; y--)
	{
		bool rowIsFull = true;

		for (int x = 0; x < WIDTH; x++)
		{
			if (!grid[y][x].occupied)
			{
				rowIsFull = false;
				break;
			}
		}

		if (!rowIsFull)
		{
			continue;
		}

		clearedRowsCount++;

		// Move all rows above downward.
		for (int row = y; row > 0; row--)
		{
			grid[row] = grid[row - 1];
		}

		// Clear top row.
		grid[0] = {};

		// Recheck same row after shifting.
		++y;
	}

	
	return clearedRowsCount;
}

std::vector<int> Board::GetFullRows() const
{
	std::vector<int> rows;

	for (int y = 0; y < HEIGHT; y++)
	{
		bool full = true;

		for (int x = 0; x < WIDTH; x++)
		{
			if (!grid[y][x].occupied)
			{
				full = false;
				break;
			}
		}

		if (full)
		{
			rows.push_back(y);
		}
	}

	return rows;
}

const Board::Grid& Board::GetGrid() const
{
	return grid;
}