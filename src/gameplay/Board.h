#pragma once

#include <array>
#include <vector>

#include "Cell.h"
#include "Tetromino.h"

class Board
{
public:
	static constexpr int WIDTH = 10;
	static constexpr int HEIGHT = 20;

	using GridRow = std::array<Cell, WIDTH>;
	using Grid = std::array<GridRow, HEIGHT>;

private:
	Grid grid;

	// CanPlace() is the whole test; these two are its halves, split only so the
	// intent reads clearly (inside the field, and not overlapping a locked cell).
	[[nodiscard]] bool Contains(const Tetromino& tetromino) const;
	[[nodiscard]] bool IntersectsLockedCells(const Tetromino& tetromino) const;

public:
	void LockTetromino(const Tetromino& tetromino);

	// FindFullRows() only inspects; ClearRows() only removes the rows it is
	// given and lets everything above fall. Splitting them lets the caller find
	// the rows once (to drive a clear animation) and remove exactly those rows
	// when the animation ends, with no second scan that could disagree.
	[[nodiscard]] std::vector<int> FindFullRows() const;
	void ClearRows(const std::vector<int>& rows);

	[[nodiscard]] bool CanPlace(const Tetromino& tetromino) const;
	[[nodiscard]] const Grid& GetGrid() const;
};
