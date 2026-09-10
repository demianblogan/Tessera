#pragma once

#include <array>
#include <vector>

#include <SFML/System/Vector2.hpp>

#include "Board.h"
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

	// Gravity plus the row-clear delay countdown.
	void Update(float deltaTime);

	[[nodiscard]] Events ConsumeEvents();

	[[nodiscard]] Phase GetPhase() const { return phase; }
	[[nodiscard]] bool IsFalling() const { return phase == Phase::Falling; }
	[[nodiscard]] GameOverReason GetGameOverReason() const { return gameOverReason; }

	[[nodiscard]] const Board& GetBoard() const { return board; }
	[[nodiscard]] const Tetromino& GetCurrentTetromino() const { return currentTetromino; }
	[[nodiscard]] const Tetromino& GetNextTetromino() const { return nextTetromino; }
	[[nodiscard]] Tetromino GetGhostTetromino() const;

	[[nodiscard]] const std::vector<int>& GetClearingRows() const { return clearingRows; }

	[[nodiscard]] int GetScore() const { return score; }
	[[nodiscard]] int GetLevel() const { return level; }
	[[nodiscard]] int GetLinesCleared() const { return totalLinesCleared; }
	[[nodiscard]] float GetElapsedSeconds() const { return elapsedSeconds; }

private:
	static constexpr int ScorePerLevel = 50;
	static constexpr int ScorePerRow = 10;
	static constexpr float BaseFallDelay = 0.5f;
	static constexpr float MinFallDelay = 0.1f;
	static constexpr float FallDelayPerLevel = 0.05f;

	// Kept in step with EffectsController::RowClearDuration: the board removal
	// happens when the clear animation ends.
	static constexpr float RowClearDelay = 0.45f;

	void LockAndScan();
	bool SpawnNextTetromino();
	void EndGame(GameOverReason reason);

	[[nodiscard]] static bool IsEntirelyInBuffer(const Tetromino& tetromino);

	Board board;
	TetrominoBag tetrominoBag;
	Tetromino currentTetromino;
	Tetromino nextTetromino;

	Phase phase = Phase::Falling;

	float fallTimer = 0.f;
	float fallDelay = BaseFallDelay;

	std::vector<int> clearingRows;
	float clearTimer = 0.f;

	int score = 0;
	int level = 1;
	int totalLinesCleared = 0;
	float elapsedSeconds = 0.f;

	GameOverReason gameOverReason = GameOverReason::None;

	Events pendingEvents;
};
