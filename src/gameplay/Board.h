#pragma once

#include <array>
#include <vector>

#include "Cell.h"
#include "Tetromino.h"

class Board
{
public:
	static constexpr int WIDTH = 10;

	// The playfield the player sees.
	static constexpr int VisibleHeight = 20;

	// Hidden rows above the visible field (the "buffer" / vanish zone). Pieces
	// spawn here and drop into view; a piece that locks entirely inside it is a
	// lock-out. The grid stores buffer rows first (y in [0, BufferHeight)), then
	// the visible rows (y in [BufferHeight, HEIGHT)).
	static constexpr int BufferHeight = 20;

	static constexpr int HEIGHT = BufferHeight + VisibleHeight;

	using GridRow = std::array<Cell, WIDTH>;
	using Grid = std::array<GridRow, HEIGHT>;

private:
	Grid grid;

	// CanPlace() is the whole test; these two are its halves, split only so the
	// intent reads clearly (inside the field, and not overlapping a locked cell).
	[[nodiscard]] bool Contains(const Tetromino& tetromino) const;
	[[nodiscard]] bool IntersectsLockedCells(const Tetromino& tetromino) const;

public:
	// `golden` marks every cell the piece locks into as Cell::Kind::Golden
	// instead of Normal -- an escalation bonus piece (see EscalationDirector).
	void LockTetromino(const Tetromino& tetromino, bool golden = false);

	// FindFullRows() only inspects; ClearRows() only removes the rows it is
	// given and lets everything above fall. Splitting them lets the caller find
	// the rows once (to drive a clear animation) and remove exactly those rows
	// when the animation ends, with no second scan that could disagree.
	[[nodiscard]] std::vector<int> FindFullRows() const;
	void ClearRows(const std::vector<int>& rows);

	// True if any cell in the given rows is a golden lock -- checked before
	// ClearRows() removes them, to decide whether a clear earns the escalation
	// double-score bonus.
	[[nodiscard]] bool RowsContainGolden(const std::vector<int>& rows) const;

	// Escalation's "Garbage" tier: raises one row from the bottom (occupied
	// except for `gapColumn`), pushing every row above up by one. Returns false
	// (and leaves the board untouched) instead of pushing locked cells off the
	// top -- the caller should treat that as a top-out.
	[[nodiscard]] bool PushGarbageRow(int gapColumn);

	[[nodiscard]] bool CanPlace(const Tetromino& tetromino) const;
	[[nodiscard]] const Grid& GetGrid() const;

	// True once every cell is empty -- a Perfect Clear.
	[[nodiscard]] bool IsEmpty() const;
};
