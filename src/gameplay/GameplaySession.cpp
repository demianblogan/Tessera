#include "GameplaySession.h"

#include <algorithm>
#include <cmath>
#include <utility>

#include "KickData.h"
#include "../utils/Random.h"

namespace
{
	// Pieces spawn low in the hidden buffer, so they sit just above the visible
	// field and drop into view the way modern Tetris shows them entering.
	constexpr sf::Vector2i SpawnPosition{ Board::WIDTH / 2 - 2, Board::BufferHeight - 2 };
}

GameplaySession::GameplaySession(Config config)
	: tetrominoBag(config.sevenBagEnabled)
	, currentTetromino(tetrominoBag.Next(), SpawnPosition)
	, nextQueueLength(std::clamp(config.nextQueueLength, MinNextQueueLength, MaxNextQueueLength))
{
	for (int i = 0; i < nextQueueLength; ++i)
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
	lastActionWasRotation = false;
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
			lastActionWasRotation = true;
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
		// A swap, not a spawn -- the piece coming back out never carries a
		// golden bonus (and the one going in loses whatever it had).
		currentPieceIsGolden = false;
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
		score += SoftDropScorePerCell;
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

	int cellsDropped = 0;

	while (true)
	{
		Tetromino movedTetromino = currentTetromino;
		movedTetromino.Move(0, 1);

		if (!board.CanPlace(movedTetromino))
		{
			break;
		}

		currentTetromino = movedTetromino;
		++cellsDropped;
	}

	score += cellsDropped * HardDropScorePerCell;

	fallTimer = 0.f;
	LockAndScan();
}

void GameplaySession::Update(float deltaTime)
{
	if (phase != Phase::GameOver)
	{
		elapsedSeconds += deltaTime;
		escalation.Update(deltaTime);

		const EscalationDirector::Events escalationEvents = escalation.ConsumeEvents();
		pendingEvents.speedSurgeStarted = pendingEvents.speedSurgeStarted || escalationEvents.surgeStarted;
		pendingEvents.speedSurgeEnded = pendingEvents.speedSurgeEnded || escalationEvents.surgeEnded;
	}

	if (phase == Phase::ClearingRows)
	{
		clearTimer += deltaTime;

		if (clearTimer < RowClearDelay)
		{
			return;
		}

		const int clearedRows = static_cast<int>(clearingRows.size());

		// Checked before ClearRows() erases the rows it would otherwise read.
		const bool goldenBonus = board.RowsContainGolden(clearingRows);

		board.ClearRows(clearingRows);
		clearingRows.clear();

		escalation.NotifyLinesCleared(clearedRows);

		const TSpinRule::Result tSpin = pendingTSpinResult;
		pendingTSpinResult = TSpinRule::Result::None;

		// Guideline scoring: Single/Double/Triple/Tetris, at the level the clear
		// happened at -- before this clear's own lines can push the level up. A
		// T-spin clear uses its own (higher) table instead.
		const int scoringRows = std::clamp(clearedRows, 1, static_cast<int>(LineClearScores.size()));
		int lineScore = 0;

		if (tSpin == TSpinRule::Result::Full)
		{
			const int index = std::clamp(scoringRows, 1, static_cast<int>(TSpinClearScores.size())) - 1;
			lineScore = TSpinClearScores[index] * level;
		}
		else if (tSpin == TSpinRule::Result::Mini)
		{
			const int index = std::clamp(scoringRows, 1, static_cast<int>(TSpinMiniClearScores.size())) - 1;
			lineScore = TSpinMiniClearScores[index] * level;
		}
		else
		{
			lineScore = LineClearScores[scoringRows - 1] * level;
		}

		// Back-to-back: a Tetris or a T-spin clear right after another one of
		// either (nothing smaller in between) scores the line-clear part at
		// 1.5x. Evaluated against the state left by the *previous* clear, then
		// updated for the next one.
		const bool isDifficultClear = tSpin != TSpinRule::Result::None
			|| scoringRows == static_cast<int>(LineClearScores.size());
		const bool earnedBackToBack = isDifficultClear && backToBackActive;
		if (earnedBackToBack)
		{
			lineScore = static_cast<int>(static_cast<float>(lineScore) * BackToBackMultiplier);
		}
		backToBackActive = isDifficultClear;

		// Escalation's Chaos-tier golden bonus: a clear that took a golden lock
		// doubles the line-clear score, on top of any back-to-back multiplier.
		if (goldenBonus)
		{
			lineScore *= 2;
		}

		// Combo: every clear beyond the first in an unbroken chain adds its own
		// bonus, on top of (not multiplied by) the line-clear score above.
		++comboCount;
		if (comboCount > 0)
		{
			score += ComboScorePerLevel * comboCount * level;
		}

		score += lineScore;

		totalLinesCleared += clearedRows;

		const int previousLevel = level;
		level = totalLinesCleared / LinesPerLevel + 1;

		// Perfect Clear: nothing left on the board at all.
		const bool isPerfectClear = board.IsEmpty();
		if (isPerfectClear)
		{
			score += PerfectClearScores[scoringRows - 1] * level;
		}

		pendingEvents.rowsCleared = true;
		pendingEvents.clearedRowCount = clearedRows;
		pendingEvents.comboCount = comboCount;   // never negative here: it was just incremented from >= -1
		pendingEvents.backToBack = earnedBackToBack;
		pendingEvents.perfectClear = isPerfectClear;
		pendingEvents.goldenLineBonus = goldenBonus;
		pendingEvents.tSpin = tSpin != TSpinRule::Result::None;
		pendingEvents.tSpinMini = tSpin == TSpinRule::Result::Mini;
		pendingEvents.leveledUp = level > previousLevel;

		fallDelay = GravityDelayForLevel(level);

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
	// the first step it can't take -- the lock delay below takes over there. A
	// Speed Surge (see EscalationDirector) divides the delay instead of scaling
	// fallTimer, so it can turn on and off mid-descent without losing progress.
	const float effectiveFallDelay = fallDelay / escalation.FallSpeedMultiplier();

	fallTimer += deltaTime;

	while (fallTimer >= effectiveFallDelay)
	{
		fallTimer -= effectiveFallDelay;

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

	// T-spin check happens against the resting position, before this piece's
	// own cells join the board (they're never diagonal from its centre, so it
	// wouldn't matter either way, but the intent reads clearer this way).
	const TSpinRule::Result tSpin = TSpinRule::Detect(board, currentTetromino, lastActionWasRotation);

	// Lock-out: the piece came to rest without any part reaching the visible
	// field, so the stack has overflowed the top.
	const bool lockedOut = IsEntirelyInBuffer(currentTetromino);

	board.LockTetromino(currentTetromino, currentPieceIsGolden);

	if (lockedOut)
	{
		EndGame(GameOverReason::LockOut);
		return;
	}

	const std::vector<int> fullRows = board.FindFullRows();

	if (!fullRows.empty())
	{
		// The actual score depends on how many rows clear, which isn't decided
		// until the clear delay in Update() elapses -- carry the classification
		// forward until then.
		pendingTSpinResult = tSpin;

		pendingEvents.rowsDetected = true;
		pendingEvents.detectedRows = fullRows;

		clearingRows = fullRows;
		clearTimer = 0.f;
		phase = Phase::ClearingRows;
		return;
	}

	// This piece locked without clearing anything -- any combo chain ends here.
	comboCount = -1;

	// A T-spin that clears nothing still scores, and neither breaks nor extends
	// back-to-back (that's tied to clears, and this one didn't clear anything).
	if (tSpin != TSpinRule::Result::None)
	{
		score += (tSpin == TSpinRule::Result::Full ? TSpinNoClearScore : TSpinMiniNoClearScore) * level;

		pendingEvents.tSpin = true;
		pendingEvents.tSpinMini = tSpin == TSpinRule::Result::Mini;
	}

	if (!SpawnNextTetromino())
	{
		EndGame(GameOverReason::BlockOut);
	}
}

bool GameplaySession::SpawnNextTetromino()
{
	// Escalation's Garbage tier: applied here, right before a new piece takes
	// the field, so it never appears out from under one mid-fall. A push that
	// tops out fails the spawn exactly like an unspawnable position would.
	if (escalation.ConsumePendingGarbageRow())
	{
		if (!board.PushGarbageRow(Random::Int(0, Board::WIDTH - 1)))
		{
			return false;
		}
		pendingEvents.garbagePushed = true;
	}

	const Tetromino::Type type = nextQueue.front();
	nextQueue.pop_front();
	nextQueue.push_back(tetrominoBag.Next());
	++spawnCount;

	currentTetromino = { type, SpawnPosition };
	currentPieceIsGolden = escalation.ShouldSpawnGoldenPiece();

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
	lastActionWasRotation = false;
}

float GameplaySession::GravityDelayForLevel(int level)
{
	// The guideline gravity curve: (0.8 - (level-1)*0.007) ^ (level-1) seconds
	// per row. Level 1 is 1s/row (any base to the power 0 is 1) and it eases
	// down from there; the base is floored so a very high level can't make it
	// negative before the exponent gets a chance to matter.
	const float base = std::max(0.001f, 0.8f - static_cast<float>(level - 1) * 0.007f);
	const float delay = std::pow(base, static_cast<float>(level - 1));

	return std::max(MinFallDelay, delay);
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
	// Gravity/soft-drop movement, so whatever T-spin setup a rotation left
	// behind no longer applies -- except a hard drop's fall, which never calls
	// this (see HardDrop()), so rotating into a spin and hard-dropping it still
	// counts.
	lastActionWasRotation = false;

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

