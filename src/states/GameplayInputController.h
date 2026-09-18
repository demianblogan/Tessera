#pragma once

#include <functional>

#include "../gameplay/Tetromino.h"
#include "../input/ActionMap.h"
#include "../input/DirectionalRepeater.h"
#include "../input/InputHandler.h"

struct Context;
class GameplaySession;
class EffectsController;
class SceneMotion;
class BoardRenderer;
class GameplayHUD;

namespace sf
{
	class Event;
}

// The player-input half of the gameplay screen: turns keyboard/gamepad input
// into GameplaySession actions (move, rotate, hard drop, hold, soft drop),
// plus the sound/haptic/parallax feedback tied directly to that action (a
// move's footstep, a rotate's click, a wall bump's shake). GameplayState
// keeps the other half -- reacting to what GameplaySession::Update() reports
// happened (row clears, level ups, game over) in ReactToEvents -- since that
// reaction isn't triggered by any particular input.
class GameplayInputController
{
public:
	// `onPauseRequested` fires once for the keyboard Pause binding and once for
	// the gamepad's pause button; GameplayState supplies OpenPause() for it.
	GameplayInputController(Context& context, GameplaySession& session, EffectsController& effects,
		SceneMotion& sceneMotion, BoardRenderer& boardRenderer, GameplayHUD& HUD,
		std::function<void()> onPauseRequested);

	// Keyboard OnPress actions (rotate, hard drop, hold, pause) and the
	// gamepad's own pause button. Its d-pad / stick / trigger actions have no
	// event -- they're polled every frame in Update() instead.
	void HandleEvent(const sf::Event& event);

	// Polls held movement/soft-drop input and advances horizontal auto-repeat.
	void Update(float deltaTime);

	// Set by PerformHardDrop() right before session.HardDropTetromino() (which locks the
	// piece instantly -- lock timing has to stay exact). GameplayState reads
	// these back once the resulting `landed` event arrives, so the purely
	// cosmetic slide/dust know this lock was a hard drop, and how far it fell.
	[[nodiscard]] bool IsHardDropAnimationPending() const noexcept;
	void ClearHardDropAnimationPending() noexcept;
	[[nodiscard]] int GetHardDropStartRow() const noexcept;
	[[nodiscard]] Tetromino::Type GetHardDropType() const noexcept;

private:
	enum class GameplayAction
	{
		MoveLeft,
		MoveRight,
		SoftDrop,
		HardDrop,
		RotateClockwise,
		RotateCounterClockwise,
		Hold,
		Pause
	};

	void SetUpInputBindings();

	void PollHeldInput();
	void ApplyGamepadActions();
	void ApplyHorizontalRepeat(float deltaTime);
	void ApplySoftDrop(float deltaTime);

	void TryRotate(bool isClockwise);
	void PerformHardDrop();
	void TryHold();

	Context& context;
	GameplaySession& session;
	EffectsController& effects;
	SceneMotion& sceneMotion;
	BoardRenderer& boardRenderer;
	GameplayHUD& HUD;
	std::function<void()> onPauseRequested;

	ActionMap<GameplayAction> gameplayActions;
	InputHandler<GameplayAction> gameplayInput;
	DirectionalRepeater horizontalRepeater;

	int heldHorizontal = 0;
	int previousHeldHorizontal = 0;
	bool isHorizontalBlocked = false;
	bool isSoftDropHeld = false;
	float softDropTimer = 0.f;

	bool isHardDropAnimationPending = false;
	int hardDropStartRow = 0;
	Tetromino::Type hardDropType = Tetromino::Type::I;
};
