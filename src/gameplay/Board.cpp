#include "Board.h"

namespace
{
	bool IsInsideGrid(const sf::Vector2i& cell)
	{
		return cell.x >= 0 && cell.x < Board::WIDTH &&
			cell.y >= 0 && cell.y < Board::HEIGHT;
	}
}

bool Board::Contains(const Tetromino& tetromino) const
{
	for (const sf::Vector2i& blockPosition : tetromino.GetBlockPositions())
	{
		if (!IsInsideGrid(blockPosition))
		{
			return false;
		}
	}

	return true;
}

bool Board::IntersectsLockedCells(const Tetromino& tetromino) const
{
	for (const sf::Vector2i& blockPosition : tetromino.GetBlockPositions())
	{
		// Out-of-bounds blocks are a wall/floor collision, which is Contains()'s
		// responsibility -- not a locked-cell overlap. Indexing grid[][] with
		// them here would read past the fixed-size arrays.
		if (!IsInsideGrid(blockPosition))
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
	for (const sf::Vector2i& blockPosition : tetromino.GetBlockPositions())
	{
		if (!IsInsideGrid(blockPosition))
		{
			continue;
		}

		Cell& cell = grid[blockPosition.y][blockPosition.x];
		cell.occupied = true;
		cell.tetrominoType = tetromino.GetType();
	}
}

std::vector<int> Board::FindFullRows() const
{
	std::vector<int> fullRows;

	for (int y = 0; y < HEIGHT; y++)
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

		if (rowIsFull)
		{
			fullRows.push_back(y);
		}
	}

	return fullRows;
}

void Board::ClearRows(const std::vector<int>& rows)
{
	if (rows.empty())
	{
		return;
	}

	std::array<bool, HEIGHT> isCleared = {};

	for (int row : rows)
	{
		if (row >= 0 && row < HEIGHT)
		{
			isCleared[row] = true;
		}
	}

	// Compact the surviving rows toward the bottom, then blank the rows left
	// over at the top.
	int writeRow = HEIGHT - 1;

	for (int readRow = HEIGHT - 1; readRow >= 0; readRow--)
	{
		if (isCleared[readRow])
		{
			continue;
		}

		grid[writeRow] = grid[readRow];
		writeRow--;
	}

	while (writeRow >= 0)
	{
		grid[writeRow] = {};
		writeRow--;
	}
}

const Board::Grid& Board::GetGrid() const
{
	return grid;
}
