#pragma once

#include <array>
#include <deque>
#include <optional>
#include <vector>

#include <SFML/System/Vector2.hpp>

#include "Board.h"
#include "TSpinRule.h"
#include "Tetromino.h"
#include "TetrominoBag.h"
#include "TetrominoShapes.h"

// One playthrough of Tessera, start to game over: the pure rules and state of
// the board, the falling piece, gravity, scoring and levelling. It holds no
// SFML render objects and no Context -- everything here could run headless.
//
// The host (GameplayState) feeds it input intents, calls Update() once per
// frame, then drains ConsumeEvents() to react with sound, HUD text and screen
// effects.
class GameplaySession
{
public:
	enum class Phase
	{
		Falling,        // a piece is in play and responds to input
		ClearingRows,   // full rows found; the clear delay is running, input is ignored
		GameOver        // the session is finished -- see GameOverReason
	};

	// Why the session ended. BlockOut: a new piece had no room to spawn. LockOut:
	// a piece came to rest entirely inside the hidden buffer, above the field.
	enum class GameOverReason
	{
		None,
		BlockOut,
		LockOut
	};

	// Everything that happened during the last MoveHorizontal / Rotate /
	// SoftDropStep / HardDrop / Update call, accumulated until ConsumeEvents().
	struct Events
	{
		bool landed = false;
		std::array<sf::Vector2i, TetrominoShapes::BLOCK_COUNT> landedBlocks{};

		bool rowsDetected = false;
		std::vector<int> detectedRows;

		bool rowsCleared = false;
		int clearedRowCount = 0;

		// 0 on an isolated clear; increments with each clear that directly
		// follows another (no non-clearing lock in between). Meaningless unless
		// rowsCleared is true.
		int comboCount = 0;
		// True if this clear's line-clear score got the back-to-back bonus (a
		// Tetris or a T-spin clear directly following another one).
		bool backToBack = false;
		// True if this clear left the board completely empty.
		bool perfectClear = false;

		// This lock was recognised as a T-spin (the three-corner rule, and the
		// last thing done to the piece was a rotation) -- set whether or not it
		// cleared any lines. tSpinMini distinguishes Mini from full.
		bool tSpin = false;
		bool tSpinMini = false;

		bool leveledUp = false;

		bool gameOver = false;
		GameOverReason gameOverReason = GameOverReason::None;
	};

	GameplaySession();

	// Input intents. The horizontal / rotate calls return whether the piece
	// actually moved so the caller can drive its own move / wall-contact
	// feedback; landing-related consequences arrive through ConsumeEvents().
	bool MoveHorizontal(int direction);
	bool Rotate(bool clockwise);
	void SoftDropStep();
	void HardDrop();

	// Swap the active piece into the hold slot: the first time, it stashes the
	// active piece and draws the next queued one; after that, it swaps with
	// whatever is already held. Once per piece in play -- false if hold was
	// already used this piece, or nothing is falling.
	bool Hold();

	// Gravity plus the row-clear delay countdown.
	void Update(float deltaTime);

	[[nodiscard]] Events ConsumeEvents();

	[[nodiscard]] Phase GetPhase() const { return phase; }
	[[nodiscard]] bool IsFalling() const { return phase == Phase::Falling; }
	[[nodiscard]] GameOverReason GetGameOverReason() const { return gameOverReason; }

	[[nodiscard]] const Board& GetBoard() const { return board; }
	[[nodiscard]] const Tetromino& GetCurrentTetromino() const { return currentTetromino; }
	[[nodiscard]] Tetromino GetGhostTetromino() const;

	// The upcoming pieces, in order (index 0 is the piece that spawns next).
	[[nodiscard]] int GetNextCount() const { return static_cast<int>(nextQueue.size()); }
	[[nodiscard]] Tetromino GetNextPiece(int index) const;

	// How many times a piece has spawned so far. Purely a change signal for the
	// renderer (e.g. to animate the next-queue sliding up) -- nothing here reads
	// the count itself.
	[[nodiscard]] int GetSpawnCount() const { return spawnCount; }

	[[nodiscard]] bool HasHeldPiece() const { return heldType.has_value(); }
	[[nodiscard]] bool CanHold() const { return !holdUsedThisTurn; }
	// Only meaningful when HasHeldPiece() is true.
	[[nodiscard]] Tetromino GetHeldPiece() const;

	[[nodiscard]] const std::vector<int>& GetClearingRows() const { return clearingRows; }

	[[nodiscard]] int GetScore() const { return score; }
	[[nodiscard]] int GetLevel() const { return level; }
	[[nodiscard]] int GetLinesCleared() const { return totalLinesCleared; }
	[[nodiscard]] float GetElapsedSeconds() const { return elapsedSeconds; }

private:
	// Guideline scoring: points per line clear (Single/Double/Triple/Tetris, by
	// row count 1..4), multiplied by the level the clear happened at. Soft/hard
	// drop award a small bonus per cell dropped, win or lose the race to lock.
	static constexpr std::array<int, 4> LineClearScores = { 100, 300, 500, 800 };
	static constexpr int SoftDropScorePerCell = 1;
	static constexpr int HardDropScorePerCell = 2;

	// T-spin scoring. A T-spin can clear at most 3 lines (the piece is never
	// more than 3 rows tall), so these tables stop at Triple, unlike the plain
	// line-clear table above. "No lines" is scored separately, immediately at
	// lock time, since ClearingRows is never entered for it.
	static constexpr int TSpinNoClearScore = 400;
	static constexpr int TSpinMiniNoClearScore = 100;
	static constexpr std::array<int, 3> TSpinClearScores = { 800, 1200, 1600 };
	static constexpr std::array<int, 2> TSpinMiniClearScores = { 200, 400 };

	// Combo: 50 * comboCount * level, on top of the line-clear score, for every
	// clear beyond the first in an unbroken chain of clears.
	static constexpr int ComboScorePerLevel = 50;

	// Back-to-back: a Tetris or T-spin clear directly following another one (no
	// ordinary clear in between) scores its line-clear component at this
	// multiplier. A T-spin that clears nothing neither breaks nor extends this.
	static constexpr float BackToBackMultiplier = 1.5f;

	// Perfect Clear: the board is completely empty after the clear. Indexed the
	// same way as LineClearScores (by row count 1..4), added on top of it.
	static constexpr std::array<int, 4> PerfectClearScores = { 800, 1200, 1800, 2000 };

	// A new level every this many total lines cleared.
	static constexpr int LinesPerLevel = 10;

	// How many upcoming pieces the queue holds (and the HUD shows). Fixed for
	// now; a player setting for this arrives with the other gameplay toggles.
	static constexpr int NextQueueLength = 5;

	// However high the level climbs, gravity never gets faster than this --
	// GravityDelayForLevel() otherwise keeps shrinking indefinitely.
	static constexpr float MinFallDelay = 0.02f;

	// Kept in step with EffectsController::RowClearDuration: the board removal
	// happens when the clear animation ends.
	static constexpr float RowClearDelay = 0.45f;

	// A piece that can no longer fall is given this long before it locks, during
	// which it still takes input. Each successful move or rotation resets the
	// countdown, but only up to MaxLockResets times at a given depth -- past that
	// the piece locks regardless, so it can't be stalled forever by spinning.
	static constexpr float LockDelay = 0.5f;
	static constexpr int MaxLockResets = 15;

	void LockAndScan();
	bool SpawnNextTetromino();
	void EndGame(GameOverReason reason);

	// Seconds per row of gravity at `level`, following the guideline curve
	// (level 1 = 1s/row, easing down from there), floored at MinFallDelay.
	[[nodiscard]] static float GravityDelayForLevel(int level);

	// Lock-delay bookkeeping.
	void ResetLockState();
	void OnPieceDescended();   // after the piece moves down a row
	void OnPieceShifted();     // after a successful horizontal move or rotation
	[[nodiscard]] bool IsResting() const;
	[[nodiscard]] int PieceBottomRow() const;

	[[nodiscard]] static bool IsEntirelyInBuffer(const Tetromino& tetromino);

	Board board;
	TetrominoBag tetrominoBag;
	Tetromino currentTetromino;
	std::deque<Tetromino::Type> nextQueue;

	Phase phase = Phase::Falling;

	float fallTimer = 0.f;
	float fallDelay = GravityDelayForLevel(1);

	float lockTimer = 0.f;
	int lockResets = 0;
	int lowestRow = 0;   // deepest row the active piece's lowest block has reached

	int spawnCount = 0;

	std::optional<Tetromino::Type> heldType;
	bool holdUsedThisTurn = false;

	std::vector<int> clearingRows;
	float clearTimer = 0.f;

	int score = 0;
	int level = 1;
	int totalLinesCleared = 0;
	float elapsedSeconds = 0.f;

	int comboCount = -1;         // -1 = not currently chaining clears
	bool backToBackActive = false;

	// True if the last thing done to the active piece was a successful
	// rotation (cleared by any move, including gravity) -- required for a
	// T-spin. Carries a lock's T-spin classification through the row-clear
	// delay, since Update() resolves the actual clear later than LockAndScan().
	bool lastActionWasRotation = false;
	TSpinRule::Result pendingTSpinResult = TSpinRule::Result::None;

	GameOverReason gameOverReason = GameOverReason::None;

	Events pendingEvents;
};
