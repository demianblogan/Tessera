#include "GameplaySession.h"

#include <algorithm>
#include <utility>

GameplaySession::GameplaySession()
	: currentTetromino(tetrominoBag.Next(), { Board::WIDTH / 2 - 2, 0 })
	, nextTetromino(tetrominoBag.Next(), { 0, 0 })
{
	// No code
}

bool GameplaySession::MoveHorizontal(int direction)
{
	if (phase != Phase::Falling || direction == 0)
	{
		return false;
	}

	Tetromino movedTetromino = currentTetromino;
	movedTetromino.Move(direction > 0 ? 1 : -1, 0);

	if (!board.CanPlace(movedTetromino))
	{
		return false;
	}

	currentTetromino = movedTetromino;
	return true;
}

bool GameplaySession::Rotate(bool clockwise)
{
	if (phase != Phase::Falling)
	{
		return false;
	}

	Tetromino rotatedTetromino = currentTetromino;

	if (clockwise)
	{
		rotatedTetromino.RotateClockwise();
	}
	else
	{
		rotatedTetromino.RotateCounterClockwise();
	}

	if (!board.CanPlace(rotatedTetromino))
	{
		return false;
	}

	currentTetromino = rotatedTetromino;
	return true;
}

void GameplaySession::SoftDropStep()
{
	if (phase != Phase::Falling)
	{
		return;
	}

	Tetromino movedTetromino = currentTetromino;
	movedTetromino.Move(0, 1);

	if (board.CanPlace(movedTetromino))
	{
		currentTetromino = movedTetromino;
		fallTimer = 0.f;
	}
	else
	{
		LockAndScan();
	}
}

void GameplaySession::HardDrop()
{
	if (phase != Phase::Falling)
	{
		return;
	}

	while (true)
	{
		Tetromino movedTetromino = currentTetromino;
		movedTetromino.Move(0, 1);

		if (!board.CanPlace(movedTetromino))
		{
			break;
		}

		currentTetromino = movedTetromino;
	}

	fallTimer = 0.f;
	LockAndScan();
}

void GameplaySession::Update(float deltaTime)
{
	if (phase == Phase::ClearingRows)
	{
		clearTimer += deltaTime;

		if (clearTimer < RowClearDelay)
		{
			return;
		}

		const int clearedRows = static_cast<int>(clearingRows.size());
		board.ClearRows(clearingRows);
		clearingRows.clear();

		score += clearedRows * ScorePerRow;

		const int previousLevel = level;
		level = score / ScorePerLevel + 1;

		pendingEvents.rowsCleared = true;
		pendingEvents.clearedRowCount = clearedRows;
		pendingEvents.leveledUp = level > previousLevel;

		fallDelay = std::max(MinFallDelay, BaseFallDelay - (level - 1) * FallDelayPerLevel);

		phase = Phase::Falling;

		if (!SpawnNextTetromino())
		{
			phase = Phase::GameOver;
			pendingEvents.gameOver = true;
		}

		return;
	}

	if (phase != Phase::Falling)
	{
		return;
	}

	fallTimer += deltaTime;

	if (fallTimer >= fallDelay)
	{
		fallTimer -= fallDelay;

		Tetromino movedTetromino = currentTetromino;
		movedTetromino.Move(0, 1);

		if (board.CanPlace(movedTetromino))
		{
			currentTetromino = movedTetromino;
		}
		else
		{
			LockAndScan();
		}
	}
}

GameplaySession::Events GameplaySession::ConsumeEvents()
{
	Events consumed = std::move(pendingEvents);
	pendingEvents = {};
	return consumed;
}

Tetromino GameplaySession::GetGhostTetromino() const
{
	Tetromino ghostTetromino = currentTetromino;

	while (true)
	{
		Tetromino movedTetromino = ghostTetromino;
		movedTetromino.Move(0, 1);

		if (!board.CanPlace(movedTetromino))
		{
			break;
		}

		ghostTetromino = movedTetromino;
	}

	return ghostTetromino;
}

void GameplaySession::LockAndScan()
{
	pendingEvents.landed = true;
	pendingEvents.landedBlocks = currentTetromino.GetBlockPositions();

	board.LockTetromino(currentTetromino);

	const std::vector<int> fullRows = board.FindFullRows();

	if (!fullRows.empty())
	{
		pendingEvents.rowsDetected = true;
		pendingEvents.detectedRows = fullRows;

		clearingRows = fullRows;
		clearTimer = 0.f;
		phase = Phase::ClearingRows;
		return;
	}

	if (!SpawnNextTetromino())
	{
		phase = Phase::GameOver;
		pendingEvents.gameOver = true;
	}
}

bool GameplaySession::SpawnNextTetromino()
{
	currentTetromino = { nextTetromino.GetType(), { Board::WIDTH / 2 - 2, 0 } };
	nextTetromino = { tetrominoBag.Next(), { 0, 0 } };

	return board.CanPlace(currentTetromino);
}
