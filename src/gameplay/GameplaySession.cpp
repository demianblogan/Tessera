#include "GameplaySession.h"

#include <algorithm>
#include <utility>

#include "KickData.h"

namespace
{
	// Pieces spawn low in the hidden buffer, so they sit just above the visible
	// field and drop into view the way modern Tetris shows them entering.
	constexpr sf::Vector2i SpawnPosition{ Board::WIDTH / 2 - 2, Board::BufferHeight - 2 };
}

GameplaySession::GameplaySession()
	: currentTetromino(tetrominoBag.Next(), SpawnPosition)
{
	for (int i = 0; i < NextQueueLength; ++i)
	{
		nextQueue.push_back(tetrominoBag.Next());
	}

	ResetLockState();
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
	OnPieceShifted();
	return true;
}

bool GameplaySession::Rotate(bool clockwise)
{
	if (phase != Phase::Falling)
	{
		return false;
	}

	const int fromRotation = currentTetromino.GetRotationIndex();
	const int toRotation = (fromRotation + (clockwise ? 1 : 3)) % 4;

	// SRS wall kicks: try each offset in order and take the first that fits, so a
	// rotation into a wall or the stack slides clear instead of failing.
	for (const sf::Vector2i& kick : KickData::Offsets(currentTetromino.GetType(), fromRotation, toRotation))
	{
		Tetromino candidate = currentTetromino;

		if (clockwise)
		{
			candidate.RotateClockwise();
		}
		else
		{
			candidate.RotateCounterClockwise();
		}

		candidate.Move(kick.x, kick.y);

		if (board.CanPlace(candidate))
		{
			currentTetromino = candidate;
			OnPieceShifted();
			return true;
		}
	}

	return false;
}

bool GameplaySession::Hold()
{
	if (phase != Phase::Falling || holdUsedThisTurn)
	{
		return false;
	}

	const Tetromino::Type currentType = currentTetromino.GetType();

	if (heldType)
	{
		// Swap: the piece that was held drops in at the spawn position; the
		// active piece takes its place in the hold slot. The queue is untouched.
		const Tetromino::Type swapped = *heldType;
		heldType = currentType;
		currentTetromino = { swapped, SpawnPosition };
		ResetLockState();

		if (!board.CanPlace(currentTetromino))
		{
			EndGame(GameOverReason::BlockOut);
		}
	}
	else
	{
		// First hold: stash the active piece and draw the next queued one, same
		// as a normal spawn.
		heldType = currentType;
		if (!SpawnNextTetromino())
		{
			EndGame(GameOverReason::BlockOut);
		}
	}

	holdUsedThisTurn = true;
	return true;
}

Tetromino GameplaySession::GetHeldPiece() const
{
	return { heldType.value_or(Tetromino::Type::I), { 0, 0 } };
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
		OnPieceDescended();
	}
	// Otherwise the piece is resting; the lock delay in Update() locks it. Soft
	// drop is faster gravity, not an instant lock.
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
	if (phase != Phase::GameOver)
	{
		elapsedSeconds += deltaTime;
	}

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

		totalLinesCleared += clearedRows;
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
			EndGame(GameOverReason::BlockOut);
		}

		return;
	}

	if (phase != Phase::Falling)
	{
		return;
	}

	// Gravity: step the piece down for each fall-delay's worth of time. Stop at
	// the first step it can't take -- the lock delay below takes over there.
	fallTimer += deltaTime;

	while (fallTimer >= fallDelay)
	{
		fallTimer -= fallDelay;

		Tetromino movedTetromino = currentTetromino;
		movedTetromino.Move(0, 1);

		if (!board.CanPlace(movedTetromino))
		{
			fallTimer = 0.f;
			break;
		}

		currentTetromino = movedTetromino;
		OnPieceDescended();
	}

	// Lock delay: once the piece can't fall, count down and lock when it expires.
	if (IsResting())
	{
		lockTimer += deltaTime;

		if (lockTimer >= LockDelay)
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

	// Lock-out: the piece came to rest without any part reaching the visible
	// field, so the stack has overflowed the top.
	const bool lockedOut = IsEntirelyInBuffer(currentTetromino);

	board.LockTetromino(currentTetromino);

	if (lockedOut)
	{
		EndGame(GameOverReason::LockOut);
		return;
	}

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
		EndGame(GameOverReason::BlockOut);
	}
}

bool GameplaySession::SpawnNextTetromino()
{
	const Tetromino::Type type = nextQueue.front();
	nextQueue.pop_front();
	nextQueue.push_back(tetrominoBag.Next());
	++spawnCount;

	currentTetromino = { type, SpawnPosition };

	ResetLockState();

	// A genuinely new piece is in play, so hold is available again. (Hold()
	// itself also reaches this path the first time it's used, in which case it
	// immediately sets holdUsedThisTurn back to true afterwards.)
	holdUsedThisTurn = false;

	return board.CanPlace(currentTetromino);
}

Tetromino GameplaySession::GetNextPiece(int index) const
{
	return { nextQueue.at(static_cast<std::size_t>(index)), { 0, 0 } };
}

void GameplaySession::ResetLockState()
{
	lockTimer = 0.f;
	lockResets = 0;
	lowestRow = PieceBottomRow();
}

int GameplaySession::PieceBottomRow() const
{
	int bottom = 0;
	for (const sf::Vector2i& block : currentTetromino.GetBlockPositions())
	{
		bottom = std::max(bottom, block.y);
	}
	return bottom;
}

bool GameplaySession::IsResting() const
{
	Tetromino below = currentTetromino;
	below.Move(0, 1);
	return !board.CanPlace(below);
}

void GameplaySession::OnPieceDescended()
{
	const int bottom = PieceBottomRow();
	if (bottom > lowestRow)
	{
		lowestRow = bottom;
		lockTimer = 0.f;
		lockResets = 0;
	}
}

void GameplaySession::OnPieceShifted()
{
	const int bottom = PieceBottomRow();
	if (bottom > lowestRow)
	{
		lowestRow = bottom;
		lockTimer = 0.f;
		lockResets = 0;
		return;
	}

	// A move or rotation that keeps the piece at (or above) its deepest row so
	// far buys more lock time, up to MaxLockResets times.
	if (lockResets < MaxLockResets && IsResting())
	{
		lockTimer = 0.f;
		++lockResets;
	}
}

void GameplaySession::EndGame(GameOverReason reason)
{
	phase = Phase::GameOver;
	gameOverReason = reason;
	pendingEvents.gameOver = true;
	pendingEvents.gameOverReason = reason;
}

bool GameplaySession::IsEntirelyInBuffer(const Tetromino& tetromino)
{
	for (const sf::Vector2i& block : tetromino.GetBlockPositions())
	{
		if (block.y >= Board::BufferHeight)
		{
			return false;
		}
	}

	return true;
}
