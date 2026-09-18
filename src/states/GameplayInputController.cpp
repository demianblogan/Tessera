#include "GameplayInputController.h"

#include <algorithm>
#include <cmath>
#include <optional>
#include <vector>

#include <SFML/Window/Event.hpp>

#include "../audio/AudioPlayer.h"
#include "../core/Context.h"
#include "../gameplay/Board.h"
#include "../gameplay/GameplaySession.h"
#include "../haptics/HapticSettings.h"
#include "../input/InputBinding.h"
#include "../input/gamepad/GamepadHaptics.h"
#include "../input/gamepad/GamepadManager.h"
#include "../input/gamepad/HapticPulse.h"
#include "../resources/Assets.h"
#include "../rendering/BoardRenderer.h"
#include "../rendering/EffectsController.h"
#include "../rendering/GameplayHUD.h"
#include "../rendering/SceneMotion.h"
#include "../settings/GameSettings.h"
#include "../settings/SettingsManager.h"

namespace
{
	constexpr float SoftDropInterval = 0.03f;

	// Parallax impulses handed to SceneMotion. The backdrop lags the action, so
	// each shove points the way the "camera" would drift.
	constexpr float MoveNudge = 3.f;
	constexpr float RotateNudge = 3.f;
	constexpr float SoftDropNudge = 1.f;
	constexpr float HardDropNudge = 16.f;
	constexpr float HoldNudge = 6.f;
	constexpr float RotateNudgeY = -2.f;

	// ApplyHorizontalRepeat()'s wall-hit shake vs PerformHardDrop()'s landing
	// shake -- the drop hits much harder.
	constexpr float WallHitShakeDuration = 0.06f;
	constexpr float WallHitShakeAmplitude = 4.f;
	constexpr float HardDropShakeDuration = 0.12f;
	constexpr float HardDropShakeAmplitude = 12.f;
}

GameplayInputController::GameplayInputController(Context& context, GameplaySession& session, EffectsController& effects,
	SceneMotion& sceneMotion, BoardRenderer& boardRenderer, GameplayHUD& HUD, std::function<void()> onPauseRequested)
	: context(context)
	, session(session)
	, effects(effects)
	, sceneMotion(sceneMotion)
	, boardRenderer(boardRenderer)
	, HUD(HUD)
	, onPauseRequested(std::move(onPauseRequested))
	, gameplayInput(gameplayActions)
	, horizontalRepeater({ context.hapticSettings.delayedAutoShift, context.hapticSettings.autoRepeatRate })
{
	SetUpInputBindings();
}

void GameplayInputController::SetUpInputBindings()
{
	using Trigger = InputBinding::TriggerType;
	const ControlSettings& controls = context.settings.GetSettings().controls;

	gameplayActions.AddBinding(GameplayAction::MoveLeft, InputBinding(controls.moveTetrominoLeft, Trigger::WhileHeld));
	gameplayActions.AddBinding(GameplayAction::MoveRight, InputBinding(controls.moveTetrominoRight, Trigger::WhileHeld));
	gameplayActions.AddBinding(GameplayAction::SoftDrop, InputBinding(controls.softDropTetromino, Trigger::WhileHeld));
	gameplayActions.AddBinding(GameplayAction::HardDrop, InputBinding(controls.hardDropTetromino, Trigger::OnPress));
	gameplayActions.AddBinding(GameplayAction::RotateClockwise, InputBinding(controls.rotateTetrominoClockwise, Trigger::OnPress));
	gameplayActions.AddBinding(GameplayAction::RotateCounterClockwise, InputBinding(controls.rotateTetrominoCounterClockwise, Trigger::OnPress));
	gameplayActions.AddBinding(GameplayAction::Hold, InputBinding(controls.holdTetromino, Trigger::OnPress));
	gameplayActions.AddBinding(GameplayAction::Pause, InputBinding(controls.pauseGame, Trigger::OnPress));

	gameplayInput.Subscribe(GameplayAction::MoveLeft, [this] { heldHorizontal -= 1; });
	gameplayInput.Subscribe(GameplayAction::MoveRight, [this] { heldHorizontal += 1; });
	gameplayInput.Subscribe(GameplayAction::SoftDrop, [this] { isSoftDropHeld = true; });

	gameplayInput.Subscribe(GameplayAction::HardDrop, [this] { PerformHardDrop(); });
	gameplayInput.Subscribe(GameplayAction::RotateClockwise, [this] { TryRotate(true); });
	gameplayInput.Subscribe(GameplayAction::RotateCounterClockwise, [this] { TryRotate(false); });
	gameplayInput.Subscribe(GameplayAction::Hold, [this] { TryHold(); });

	gameplayInput.Subscribe(GameplayAction::Pause, [this] { onPauseRequested(); });
}

bool GameplayInputController::IsHardDropAnimationPending() const noexcept
{
	return isHardDropAnimationPending;
}

void GameplayInputController::ClearHardDropAnimationPending() noexcept
{
	isHardDropAnimationPending = false;
}

int GameplayInputController::GetHardDropStartRow() const noexcept
{
	return hardDropStartRow;
}

Tetromino::Type GameplayInputController::GetHardDropType() const noexcept
{
	return hardDropType;
}

void GameplayInputController::HandleEvent(const sf::Event& event)
{
	// Keyboard OnPress actions (hard drop, rotate, hold, pause).
	gameplayInput.HandleEvent(event);

	// Gamepad pause (a button, so an event is fine). Its d-pad / stick / trigger
	// actions are polled in Update via ApplyGamepadActions.
	if (context.gamepad.IsPausePressed(event))
	{
		onPauseRequested();
	}
}

void GameplayInputController::Update(float deltaTime)
{
	PollHeldInput();
	ApplyGamepadActions();
	ApplyHorizontalRepeat(deltaTime);
	ApplySoftDrop(deltaTime);
	previousHeldHorizontal = heldHorizontal;
}

void GameplayInputController::PollHeldInput()
{
	heldHorizontal = 0;
	isSoftDropHeld = false;

	// Keyboard WhileHeld bindings fire their callbacks, setting the members above.
	gameplayInput.Update();

	heldHorizontal = std::clamp(heldHorizontal + context.gamepad.GetHorizontalDirection(), -1, 1);

	if (context.gamepad.IsSoftDropHeld())
	{
		isSoftDropHeld = true;
	}
}

void GameplayInputController::ApplyGamepadActions()
{
	if (!session.IsFalling())
	{
		return;
	}

	if (context.gamepad.WasHardDropPressed())
	{
		PerformHardDrop();
	}

	if (context.gamepad.WasRotateClockwisePressed())
	{
		TryRotate(true);
	}

	if (context.gamepad.WasRotateCounterClockwisePressed())
	{
		TryRotate(false);
	}

	if (context.gamepad.WasHoldPressed())
	{
		TryHold();
	}
}

void GameplayInputController::ApplyHorizontalRepeat(float deltaTime)
{
	if (!session.IsFalling())
	{
		horizontalRepeater.Reset();
		isHorizontalBlocked = false;
		return;
	}

	const int requestedSteps = horizontalRepeater.Update(heldHorizontal, deltaTime);

	if (heldHorizontal == 0)
	{
		isHorizontalBlocked = false;
		return;
	}

	if (requestedSteps == 0)
	{
		return;
	}

	const int direction = requestedSteps > 0 ? 1 : -1;
	bool hasMovedAny = false;

	for (int step = 0; step < std::abs(requestedSteps); step++)
	{
		if (!session.MoveTetrominoHorizontal(direction))
		{
			break;
		}

		hasMovedAny = true;
	}

	const bool isFreshPress = heldHorizontal != previousHeldHorizontal;

	if (hasMovedAny)
	{
		sceneMotion.Nudge({ -static_cast<float>(direction) * MoveNudge, 0.f });
		isHorizontalBlocked = false;

		// Move sound on the initial step only, not on every auto-repeat step.
		if (isFreshPress)
		{
			context.audioPlayer.Play(Assets::SoundID::MovePiece);
		}
	}
	else if (isFreshPress || !isHorizontalBlocked)
	{
		// Wall contact: fire once when it happens (a fresh press into a wall, or
		// the piece reaching the wall at the end of an auto-repeat slide), then
		// stay quiet while it's held there.
		context.audioPlayer.Play(Assets::SoundID::PieceHitWall);
		effects.TriggerShake(WallHitShakeDuration, WallHitShakeAmplitude);
		Haptics::TriggerPulse(context.gamepadHaptics, context.hapticSettings.wallHit);
		isHorizontalBlocked = true;

		// Dust only where a cell actually touches whatever blocked it -- the
		// board's side wall, or an already-locked cell immediately beside it in
		// the direction it was pushed -- mirroring the hard-drop dust's
		// bottom-contact check, just rotated 90 degrees.
		const auto blocks = session.GetCurrentTetromino().GetBlockPositions();
		const Board::Grid& grid = session.GetBoard().GetGrid();
		std::vector<sf::Vector2f> impactPoints;
		for (const sf::Vector2i& block : blocks)
		{
			const int besideX = block.x + direction;
			const bool isOwnCellBeside = std::any_of(blocks.begin(), blocks.end(),
				[&](const sf::Vector2i& other) { return other.y == block.y && other.x == besideX; });

			if (isOwnCellBeside)
			{
				continue;
			}

			const bool hitsWall = besideX < 0 || besideX >= Board::Width;
			const bool hitsStack = !hitsWall
				&& grid[static_cast<std::size_t>(block.y)][static_cast<std::size_t>(besideX)].isOccupied;

			if (!hitsWall && !hitsStack)
			{
				continue;
			}

			// The cell's own edge facing the wall, not its center.
			impactPoints.push_back(
				{
					BoardRenderer::BoardPosition.x
						+ static_cast<float>(block.x + (direction > 0 ? 1 : 0)) * BoardRenderer::BlockSize,
					BoardRenderer::BoardPosition.y
						+ (static_cast<float>(block.y - Board::BufferHeight) + 0.5f) * BoardRenderer::BlockSize
				});
		}

		effects.TriggerWallDust(impactPoints, direction);
	}
}

void GameplayInputController::ApplySoftDrop(float deltaTime)
{
	if (!isSoftDropHeld)
	{
		softDropTimer = 0.f;
		return;
	}

	softDropTimer += deltaTime;

	while (softDropTimer >= SoftDropInterval && session.IsFalling())
	{
		softDropTimer -= SoftDropInterval;
		session.SoftDropTetrominoStep();
		sceneMotion.Nudge({ 0.f, SoftDropNudge });
	}
}

void GameplayInputController::TryRotate(bool isClockwise)
{
	if (!session.IsFalling())
	{
		return;
	}

	if (session.RotateTetromino(isClockwise))
	{
		context.audioPlayer.Play(Assets::SoundID::RotatePiece);
		sceneMotion.Nudge({ isClockwise ? RotateNudge : -RotateNudge, RotateNudgeY });
	}
	else
	{
		context.audioPlayer.Play(Assets::SoundID::PieceHitWall);
	}
}

void GameplayInputController::TryHold()
{
	if (!session.IsFalling() || !context.settings.GetSettings().isHoldTetrominoEnabled)
	{
		return;
	}

	const Tetromino outgoingPiece = session.GetCurrentTetromino();
	const bool hadHeldPiece = session.HasHeldPiece();

	if (session.HoldTetromino())
	{
		// Borrowed from rotate -- no dedicated hold sound exists yet. The
		// rumble is its own, though (see HapticSettings::hold).
		context.audioPlayer.Play(Assets::SoundID::RotatePiece);
		Haptics::TriggerPulse(context.gamepadHaptics, context.hapticSettings.hold);
		sceneMotion.Nudge({ 0.f, -HoldNudge });

		// Fly the outgoing piece to the HOLD box; if one was already held, fly
		// it back out to the board position Hold() just gave it.
		const std::optional<Tetromino> incomingPiece = hadHeldPiece
			? std::optional<Tetromino>(session.GetCurrentTetromino())
			: std::nullopt;
		boardRenderer.TriggerHoldSwap(outgoingPiece, incomingPiece, HUD.HoldPreviewArea());
	}
}

void GameplayInputController::PerformHardDrop()
{
	if (!session.IsFalling())
	{
		return;
	}

	// HardDrop() locks the piece instantly -- capture its pre-drop row (the
	// topmost cell of its current shape) here, before that happens, so the
	// purely cosmetic slide/dust triggered off the resulting `landed` event
	// know how far it actually fell.
	hardDropStartRow = Board::Height;
	for (const sf::Vector2i& block : session.GetCurrentTetromino().GetBlockPositions())
	{
		hardDropStartRow = std::min(hardDropStartRow, block.y);
	}
	hardDropType = session.GetCurrentTetromino().GetType();
	isHardDropAnimationPending = true;

	session.HardDropTetromino();

	context.audioPlayer.Play(Assets::SoundID::DropPiece);
	effects.TriggerShake(HardDropShakeDuration, HardDropShakeAmplitude);
	sceneMotion.Nudge({ 0.f, HardDropNudge });
	Haptics::TriggerPulse(context.gamepadHaptics, context.hapticSettings.hardDrop);
}
