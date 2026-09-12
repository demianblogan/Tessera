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

void Board::LockTetromino(const Tetromino& tetromino, bool golden)
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
		cell.kind = golden ? Cell::Kind::Golden : Cell::Kind::Normal;
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

bool Board::RowsContainGolden(const std::vector<int>& rows) const
{
	for (int row : rows)
	{
		if (row < 0 || row >= HEIGHT)
		{
			continue;
		}

		for (const Cell& cell : grid[row])
		{
			if (cell.occupied && cell.kind == Cell::Kind::Golden)
			{
				return true;
			}
		}
	}

	return false;
}

bool Board::PushGarbageRow(int gapColumn)
{
	// The very top row already holds something -- there is nowhere for it to
	// go once everything shifts up, so the stack has topped out.
	for (const Cell& cell : grid.front())
	{
		if (cell.occupied)
		{
			return false;
		}
	}

	for (int row = 0; row + 1 < HEIGHT; row++)
	{
		grid[row] = grid[row + 1];
	}

	GridRow& bottomRow = grid[HEIGHT - 1];
	for (int x = 0; x < WIDTH; x++)
	{
		bottomRow[x] = (x == gapColumn)
			? Cell{}
			: Cell{ true, Tetromino::Type::I, Cell::Kind::Garbage };
	}

	return true;
}

const Board::Grid& Board::GetGrid() const
{
	return grid;
}

bool Board::IsEmpty() const
{
	for (const GridRow& row : grid)
	{
		for (const Cell& cell : row)
		{
			if (cell.occupied)
			{
				return false;
			}
		}
	}

	return true;
}
