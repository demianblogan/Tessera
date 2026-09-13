#include "GameplayState.h"

#include <algorithm>
#include <cmath>
#include <optional>
#include <string>
#include <vector>

#include <SFML/Window/Event.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/View.hpp>

#include "../audio/AudioPlayer.h"
#include "../audio/MusicPlayer.h"
#include "../gameplay/Board.h"
#include "../resources/Assets.h"
#include "../core/Context.h"
#include "../core/StateMachine.h"
#include "../input/GamepadManager.h"
#include "../input/InputBinding.h"
#include "../config/HapticSettings.h"
#include "../input/gamepad/GamepadHaptics.h"
#include "../input/gamepad/HapticPulse.h"
#include "../localization/LocalizationManager.h"
#include "../localization/TextKeys.h"
#include "../settings/SettingsManager.h"
#include "../settings/GameSettings.h"
#include "../display/DisplayManager.h"
#include "PauseState.h"
#include "GameOverState.h"

namespace
{
	constexpr float SoftDropInterval = 0.03f;
	constexpr float Pi = 3.14159265f;
	constexpr float BackgroundScale = 1.07f;

	// Parallax impulses handed to SceneMotion. The backdrop lags the action, so
	// each shove points the way the "camera" would drift.
	constexpr float MoveNudge = 3.f;
	constexpr float RotateNudge = 3.f;
	constexpr float SoftDropNudge = 1.f;
	constexpr float HardDropNudge = 16.f;
	constexpr float HoldNudge = 6.f;
	constexpr float LandNudge = 5.f;
	constexpr float RowClearNudge = 12.f;
	constexpr float TetrisNudge = 26.f;

	// On-board callout look: colour by what earned it; text grows with the
	// event's rank (0 = least special), one fixed smaller size for the combo
	// count underneath it.
	constexpr unsigned int CalloutBaseSize = 66;
	constexpr unsigned int CalloutSizePerRank = 8;
	constexpr unsigned int CalloutComboSize = 48;

	const sf::Color DefaultClearColour{ 235, 240, 248 };
	const sf::Color TetrisColour{ 120, 230, 255 };
	const sf::Color TSpinColour{ 220, 130, 255 };
	const sf::Color BackToBackColour{ 255, 190, 80 };
	const sf::Color PerfectClearColour{ 255, 215, 60 };
	const sf::Color ComboColour{ 160, 220, 255 };

	// Escalation (see EscalationDirector).
	const sf::Color SpeedSurgeColour{ 255, 90, 70 };
	const sf::Color GoldenColour{ 255, 205, 40 };

	// Screen-space centre of a board grid cell -- the same placement
	// BoardRenderer draws locked cells at, used to spawn row-clear shards and
	// to centre the T-spin burst on the piece that just locked.
	[[nodiscard]] sf::Vector2f CellCentre(int gridX, int gridY)
	{
		return
		{
			BoardRenderer::BoardPosition.x + (static_cast<float>(gridX) + 0.5f) * BoardRenderer::BlockSize,
			BoardRenderer::BoardPosition.y
				+ (static_cast<float>(gridY - Board::BufferHeight) + 0.5f) * BoardRenderer::BlockSize
		};
	}
}

GameplayState::GameplayState(Context& context, bool playIntro)
	: State(context.stateMachine)
	, context(context)
	, session(GameplaySession::Config{
		static_cast<int>(context.settings.GetSettings().nextQueueLength),
		context.settings.GetSettings().sevenBagEnabled })
	, boardRenderer(context)
	, neonGlow(context.shaders.Get(Assets::ShaderID::NeonDilate), context.shaders.Get(Assets::ShaderID::NeonBlur))
	, hud(context)
	, boardCallouts(context)
	, gameplayInput(gameplayActions)
	, horizontalRepeater({ context.hapticSettings.delayedAutoShift, context.hapticSettings.autoRepeatRate })
	, backgroundSprite(context.textures.Get(Assets::TextureID::GameplayBackground))
	, seenLocalizationRevision(context.localization.Revision())
{
	introActive = playIntro;

	// The backdrop is drawn slightly oversized and centred so SceneMotion can
	// slide it a little without exposing an edge.
	backgroundSprite.setColor(sf::Color(150, 150, 150));
	const sf::Vector2f backgroundSize(context.textures.Get(Assets::TextureID::GameplayBackground).getSize());
	backgroundSprite.setOrigin(backgroundSize * 0.5f);
	backgroundSprite.setScale({ BackgroundScale, BackgroundScale });

	SetUpInputBindings();
	ApplyGameplaySettings();

	// Switches MusicPlayer to the shuffled gameplay playlist, stopping
	// whatever was playing before (the menu shell track, normally).
	context.musicPlayer.PlayGameplay();
}

void GameplayState::ApplyGameplaySettings()
{
	if (seenLocalizationRevision != context.localization.Revision())
	{
		seenLocalizationRevision = context.localization.Revision();
		hud.RefreshText();
	}

	const GameSettings& settings = context.settings.GetSettings();

	hud.SetVisible(GameplayHud::Element::Hold, settings.hudHold && settings.holdEnabled);
	hud.SetVisible(GameplayHud::Element::Next, settings.hudNext);
	hud.SetVisible(GameplayHud::Element::Score, settings.hudScore);
	hud.SetVisible(GameplayHud::Element::Lines, settings.hudLines);
	hud.SetVisible(GameplayHud::Element::Level, settings.hudLevel);
	hud.SetVisible(GameplayHud::Element::Time, settings.hudTime);
	hud.SetVisible(GameplayHud::Element::ControlsLegend, settings.hudControlsLegend);
	hud.RefreshControlsLegend(settings.controls, settings.holdEnabled);

	effects.SetShakeEnabled(settings.screenShakeEnabled);
	boardRenderer.SetGhostEnabled(settings.ghostPieceEnabled);
}

void GameplayState::SetUpInputBindings()
{
	using Trigger = InputBinding::TriggerType;
	const ControlSettings& controls = context.settings.GetSettings().controls;

	gameplayActions.AddBinding(GameplayAction::MoveLeft, InputBinding(controls.moveLeft, Trigger::WhileHeld));
	gameplayActions.AddBinding(GameplayAction::MoveRight, InputBinding(controls.moveRight, Trigger::WhileHeld));
	gameplayActions.AddBinding(GameplayAction::SoftDrop, InputBinding(controls.softDrop, Trigger::WhileHeld));
	gameplayActions.AddBinding(GameplayAction::HardDrop, InputBinding(controls.hardDrop, Trigger::OnPress));
	gameplayActions.AddBinding(GameplayAction::RotateClockwise, InputBinding(controls.rotateClockwise, Trigger::OnPress));
	gameplayActions.AddBinding(GameplayAction::RotateCounterClockwise, InputBinding(controls.rotateCounterClockwise, Trigger::OnPress));
	gameplayActions.AddBinding(GameplayAction::Hold, InputBinding(controls.hold, Trigger::OnPress));
	gameplayActions.AddBinding(GameplayAction::Pause, InputBinding(controls.pause, Trigger::OnPress));

	gameplayInput.Subscribe(GameplayAction::MoveLeft, [this] { heldHorizontal -= 1; });
	gameplayInput.Subscribe(GameplayAction::MoveRight, [this] { heldHorizontal += 1; });
	gameplayInput.Subscribe(GameplayAction::SoftDrop, [this] { softDropHeld = true; });

	gameplayInput.Subscribe(GameplayAction::HardDrop, [this] { PerformHardDrop(); });
	gameplayInput.Subscribe(GameplayAction::RotateClockwise, [this] { TryRotate(true); });
	gameplayInput.Subscribe(GameplayAction::RotateCounterClockwise, [this] { TryRotate(false); });
	gameplayInput.Subscribe(GameplayAction::Hold, [this] { TryHold(); });

	gameplayInput.Subscribe(GameplayAction::Pause, [this] { OpenPause(); });
}

void GameplayState::HandleEvent(const sf::Event& event)
{
	if (introActive)
	{
		return;
	}

	// Keyboard OnPress actions (hard drop, rotate, pause).
	gameplayInput.HandleEvent(event);

	// Gamepad pause (a button, so an event is fine). Its d-pad / stick / trigger
	// actions are polled in Update via ApplyGamepadActions.
	if (context.gamepad.IsPausePressed(event))
	{
		OpenPause();
	}
}

void GameplayState::Update(float deltaTime)
{
	// Cheap every frame (a handful of bool assignments, plus a change-checked
	// legend rebuild), so a setting changed from the pause screen -- HUD
	// visibility, feedback toggles -- shows up the instant play resumes,
	// with no dependence on exactly when/how the state stack hands control
	// back to this state.
	ApplyGameplaySettings();

	effects.Update(deltaTime);
	neonGlow.Update(deltaTime);
	hud.Update(deltaTime);
	sceneMotion.Update(deltaTime);
	boardCallouts.Update(deltaTime);

	if (introActive)
	{
		introTimer += deltaTime;
		if (introTimer >= IntroDuration)
		{
			introActive = false;
		}
		return;
	}

	if (dying)
	{
		deathTimer += deltaTime;
		if (deathTimer >= DeathDuration)
		{
			RequestChange(std::make_unique<GameOverState>(context, session.GetScore(),
				session.GetLinesCleared(), session.GetLevel(), session.GetElapsedSeconds()));
		}
		return;
	}

	PollHeldInput();
	ApplyGamepadActions();
	ApplyHorizontalRepeat(deltaTime);
	ApplySoftDrop(deltaTime);
	previousHeldHorizontal = heldHorizontal;

	session.Update(deltaTime);
	boardRenderer.Update(deltaTime, session);

	hud.Set(session.GetScore(), session.GetLevel(), session.GetLinesCleared(), session.GetElapsedSeconds());

	// Hold a green throb on the lightbar for as long as rows are clearing.
	if (session.GetPhase() == GameplaySession::Phase::ClearingRows)
	{
		Haptics::FlashLightbar(context.gamepadHaptics, context.hapticSettings.rowClearLightbar, 0.2f);
	}

	ReactToEvents(session.ConsumeEvents());
}

void GameplayState::PollHeldInput()
{
	heldHorizontal = 0;
	softDropHeld = false;

	// Keyboard WhileHeld bindings fire their callbacks, setting the members above.
	gameplayInput.Update();

	heldHorizontal = std::clamp(heldHorizontal + context.gamepad.GetHorizontalDirection(), -1, 1);

	if (context.gamepad.IsSoftDropHeld())
	{
		softDropHeld = true;
	}
}

void GameplayState::ApplyGamepadActions()
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

void GameplayState::ApplyHorizontalRepeat(float deltaTime)
{
	if (!session.IsFalling())
	{
		horizontalRepeater.Reset();
		horizontalWasBlocked = false;
		return;
	}

	const int requestedSteps = horizontalRepeater.Update(heldHorizontal, deltaTime);

	if (heldHorizontal == 0)
	{
		horizontalWasBlocked = false;
		return;
	}

	if (requestedSteps == 0)
	{
		return;
	}

	const int direction = requestedSteps > 0 ? 1 : -1;
	bool movedAny = false;

	for (int step = 0; step < std::abs(requestedSteps); step++)
	{
		if (!session.MoveHorizontal(direction))
		{
			break;
		}

		movedAny = true;
	}

	const bool isFreshPress = heldHorizontal != previousHeldHorizontal;

	if (movedAny)
	{
		sceneMotion.Nudge({ -static_cast<float>(direction) * MoveNudge, 0.f });
		horizontalWasBlocked = false;

		// Move sound on the initial step only, not on every auto-repeat step.
		if (isFreshPress)
		{
			context.audioPlayer.Play(Assets::SoundID::MovePiece);
		}
	}
	else if (isFreshPress || !horizontalWasBlocked)
	{
		// Wall contact: fire once when it happens (a fresh press into a wall, or
		// the piece reaching the wall at the end of an auto-repeat slide), then
		// stay quiet while it's held there.
		context.audioPlayer.Play(Assets::SoundID::PieceHitWall);
		effects.TriggerShake(0.06f, 4.f);
		Haptics::Pulse(context.gamepadHaptics, context.hapticSettings.wallHit);
		horizontalWasBlocked = true;

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

			const bool hitsWall = besideX < 0 || besideX >= Board::WIDTH;
			const bool hitsStack = !hitsWall
				&& grid[static_cast<std::size_t>(block.y)][static_cast<std::size_t>(besideX)].occupied;

			if (!hitsWall && !hitsStack)
			{
				continue;
			}

			// The cell's own edge facing the wall, not its centre.
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

void GameplayState::ApplySoftDrop(float deltaTime)
{
	if (!softDropHeld)
	{
		softDropTimer = 0.f;
		return;
	}

	softDropTimer += deltaTime;

	while (softDropTimer >= SoftDropInterval && session.IsFalling())
	{
		softDropTimer -= SoftDropInterval;
		session.SoftDropStep();
		sceneMotion.Nudge({ 0.f, SoftDropNudge });
	}
}

void GameplayState::TryRotate(bool clockwise)
{
	if (!session.IsFalling())
	{
		return;
	}

	if (session.Rotate(clockwise))
	{
		context.audioPlayer.Play(Assets::SoundID::RotatePiece);
		sceneMotion.Nudge({ clockwise ? RotateNudge : -RotateNudge, -2.f });
	}
	else
	{
		context.audioPlayer.Play(Assets::SoundID::PieceHitWall);
	}
}

void GameplayState::TryHold()
{
	if (!session.IsFalling() || !context.settings.GetSettings().holdEnabled)
	{
		return;
	}

	const Tetromino outgoingPiece = session.GetCurrentTetromino();
	const bool hadHeldPiece = session.HasHeldPiece();

	if (session.Hold())
	{
		// Placeholder sound borrowed from rotate -- a dedicated hold sound
		// lands with v1.7.0's audio pass. The rumble is its own, though.
		context.audioPlayer.Play(Assets::SoundID::RotatePiece);
		Haptics::Pulse(context.gamepadHaptics, context.hapticSettings.hold);
		sceneMotion.Nudge({ 0.f, -HoldNudge });

		// Fly the outgoing piece to the HOLD box; if one was already held, fly
		// it back out to the board position Hold() just gave it.
		const std::optional<Tetromino> incomingPiece = hadHeldPiece
			? std::optional<Tetromino>(session.GetCurrentTetromino())
			: std::nullopt;
		boardRenderer.TriggerHoldSwap(outgoingPiece, incomingPiece, hud.HoldPreviewArea());
	}
}

void GameplayState::PerformHardDrop()
{
	if (!session.IsFalling())
	{
		return;
	}

	// HardDrop() locks the piece instantly -- capture its pre-drop row (the
	// topmost cell of its current shape) here, before that happens, so the
	// purely cosmetic slide/dust triggered off the resulting `landed` event
	// know how far it actually fell.
	hardDropStartRow = Board::HEIGHT;
	for (const sf::Vector2i& block : session.GetCurrentTetromino().GetBlockPositions())
	{
		hardDropStartRow = std::min(hardDropStartRow, block.y);
	}
	hardDropType = session.GetCurrentTetromino().GetType();
	pendingHardDropAnimation = true;

	session.HardDrop();

	context.audioPlayer.Play(Assets::SoundID::DropPiece);
	effects.TriggerShake(0.12f, 12.f);
	sceneMotion.Nudge({ 0.f, HardDropNudge });
	Haptics::Pulse(context.gamepadHaptics, context.hapticSettings.hardDrop);
}

void GameplayState::ReactToEvents(const GameplaySession::Events& events)
{
	// Escalation tier ambience (see EscalationDirector::Tier) -- polled every
	// frame rather than off a tier-changed event, so the border glow just eases
	// toward wherever the current tier points it.
	const sf::FloatRect boardArea{ BoardRenderer::BoardPosition,
		{ Board::WIDTH * BoardRenderer::BlockSize, Board::VisibleHeight * BoardRenderer::BlockSize } };
	effects.SetEscalationTier(static_cast<int>(session.GetEscalationTier()), boardArea);

	if (events.landed)
	{
		effects.TriggerLandingFlash(events.landedBlocks);
		Haptics::Pulse(context.gamepadHaptics, context.hapticSettings.pieceLanded);
		sceneMotion.Nudge({ 0.f, LandNudge });

		if (pendingHardDropAnimation)
		{
			pendingHardDropAnimation = false;

			int landedTopRow = Board::HEIGHT;
			for (const sf::Vector2i& block : events.landedBlocks)
			{
				landedTopRow = std::min(landedTopRow, block.y);
			}

			const int droppedRows = landedTopRow - hardDropStartRow;
			boardRenderer.TriggerHardDropFlight(hardDropType, events.landedBlocks, droppedRows);

			// Dust only where a cell actually rests on something -- an already-
			// locked cell, or the floor -- never at a cell of this same piece
			// that has one of its own other cells beneath it, and never at a
			// cell left hanging above a gap.
			const Board::Grid& grid = session.GetBoard().GetGrid();
			std::vector<sf::Vector2f> impactPoints;
			for (const sf::Vector2i& block : events.landedBlocks)
			{
				const int belowRow = block.y + 1;
				const bool isOwnCellBelow = std::any_of(events.landedBlocks.begin(), events.landedBlocks.end(),
					[&](const sf::Vector2i& other) { return other.x == block.x && other.y == belowRow; });

				if (isOwnCellBelow)
				{
					continue;
				}

				const bool restsOnFloor = belowRow >= Board::HEIGHT;
				const bool restsOnStack = !restsOnFloor
					&& grid[static_cast<std::size_t>(belowRow)][static_cast<std::size_t>(block.x)].occupied;

				if (!restsOnFloor && !restsOnStack)
				{
					continue;
				}

				// Bottom edge of the cell, not its centre -- dust kicks up from
				// where it actually touches down.
				impactPoints.push_back(
					{
						BoardRenderer::BoardPosition.x + (static_cast<float>(block.x) + 0.5f) * BoardRenderer::BlockSize,
						BoardRenderer::BoardPosition.y
							+ static_cast<float>(block.y - Board::BufferHeight + 1) * BoardRenderer::BlockSize
					});
			}

			effects.TriggerHardDropDust(impactPoints);
		}

		// A lock that starts no clear breaks any combo chain in progress --
		// fade the glow out. One that does clear leaves the combo level alone
		// here; rowsCleared sets its real value once the delay resolves.
		if (!events.rowsDetected)
		{
			effects.SetCombo(0);
		}

		// A T-spin's own tell, independent of whether it cleared any lines --
		// decided at lock time, same batch as landed.
		if (events.tSpin)
		{
			sf::Vector2f centre{ 0.f, 0.f };
			for (const sf::Vector2i& block : events.landedBlocks)
			{
				centre += CellCentre(block.x, block.y);
			}
			centre /= static_cast<float>(events.landedBlocks.size());
			effects.TriggerTSpinBurst(centre);
		}
	}

	if (events.rowsDetected)
	{
		context.audioPlayer.Play(Assets::SoundID::RowCleared);

		// Rank (0 Single .. 3 Tetris) scales the flash/sweep and the shatter
		// spawned from every occupied cell in the clearing rows -- read before
		// the delay resolves and the rows actually disappear.
		const int rank = std::clamp(static_cast<int>(events.detectedRows.size()) - 1, 0, 3);

		std::vector<EffectsController::ClearedCell> clearedCells;
		const Board::Grid& grid = session.GetBoard().GetGrid();
		for (int row : events.detectedRows)
		{
			for (int x = 0; x < Board::WIDTH; ++x)
			{
				const Cell& cell = grid[static_cast<std::size_t>(row)][static_cast<std::size_t>(x)];
				if (!cell.occupied)
				{
					continue;
				}

				const int textureIndex = cell.kind == Cell::Kind::Garbage
					? BoardRenderer::WallTextureIndex
					: static_cast<int>(cell.tetrominoType);
				clearedCells.push_back({ CellCentre(x, row), textureIndex });
			}
		}

		effects.TriggerRowClear(events.detectedRows, rank, clearedCells);

		const bool isTetris = rank >= 3;
		effects.TriggerShake(0.08f + 0.09f * static_cast<float>(rank), 3.f + 9.f * static_cast<float>(rank));
		Haptics::Pulse(context.gamepadHaptics, isTetris ? context.hapticSettings.tetris : context.hapticSettings.rowCleared);
		sceneMotion.Nudge({ 0.f, -(isTetris ? TetrisNudge : RowClearNudge) });
	}

	if (events.rowsCleared)
	{
		hud.OnRowsCleared(std::clamp(events.clearedRowCount - 1, 0, 3));
		effects.SetCombo(events.comboCount);

		if (events.perfectClear)
		{
			effects.TriggerPerfectClearBurst(
				{ BoardRenderer::BoardPosition, { Board::WIDTH * BoardRenderer::BlockSize, Board::VisibleHeight * BoardRenderer::BlockSize } });
			effects.TriggerShake(0.3f, 14.f);
		}
	}

	// rowsCleared (with the combo/back-to-back/Perfect Clear verdict) only
	// arrives once the clear delay resolves, a separate ConsumeEvents() batch
	// from the landed/rowsDetected one at lock time -- landed is long since
	// false again by then. A T-spin that cleared nothing is the one case that
	// arrives together with landed, since it's decided at lock time.
	if (events.rowsCleared || events.tSpin)
	{
		ShowClearCallout(events);
		FireClearHaptics(events);
	}

	if (events.leveledUp)
	{
		context.audioPlayer.Play(Assets::SoundID::NextLevel);
		Haptics::Pulse(context.gamepadHaptics, context.hapticSettings.levelUp);
		hud.OnLevelUp();
		sceneMotion.Nudge({ 16.f, -12.f });
	}

	if (events.gameOver)
	{
		Haptics::Pulse(context.gamepadHaptics, context.hapticSettings.gameOver);
		Haptics::FlashLightbar(context.gamepadHaptics, context.hapticSettings.gameOverLightbar, 0.9f, 3);
		effects.TriggerShake(0.5f, 26.f);

		dying = true;
		deathTimer = 0.f;
	}

	// Escalation (see EscalationDirector): a Speed Surge is telegraphed with a
	// callout, a shake and a rumble, so a sudden gravity spike reads as a fair
	// warning rather than a glitch. A garbage row is deliberately quiet -- just
	// a dull thud -- since it happens often once unlocked.
	if (events.speedSurgeStarted)
	{
		boardCallouts.Show(
			{ { context.localization.GetText(TextKey::Callout::SpeedSurge), SpeedSurgeColour, CalloutBaseSize } },
			1, SpeedSurgeColour);
		Haptics::Pulse(context.gamepadHaptics, context.hapticSettings.speedSurge);
		effects.TriggerShake(0.25f, 10.f);
		effects.TriggerSpeedSurgeGlow(EscalationDirector::SurgeDuration);
	}

	if (events.garbagePushed)
	{
		Haptics::Pulse(context.gamepadHaptics, context.hapticSettings.garbageRow);
		effects.TriggerGarbageWave();
		effects.TriggerShake(0.15f, 7.f);
	}
}

void GameplayState::ShowClearCallout(const GameplaySession::Events& events)
{
	// Plain Single clears are far too common to call out; everything else
	// (Double and up, any T-spin, back-to-back, Perfect Clear, a combo) is rare
	// or noteworthy enough to earn a popup.
	const LocalizationManager& text = context.localization;

	const auto rowSuffix = [&text](int rows) -> sf::String
	{
		switch (rows)
		{
		case 1: return text.GetText(TextKey::Callout::Single);
		case 2: return text.GetText(TextKey::Callout::Double);
		case 3: return text.GetText(TextKey::Callout::Triple);
		case 4: return text.GetText(TextKey::Callout::Tetris);
		default: return {};
		}
	};

	// How special this clear is: bigger text, a bigger flash and a longer stay
	// on screen for rarer clears, on the same escalating scale as everywhere
	// else in the game.
	int rank = 0;
	if (events.clearedRowCount == 3) { rank = 1; }
	if (events.clearedRowCount == 4) { rank = 3; }
	if (events.tSpin) { rank = std::max(rank, events.tSpinMini ? 2 : 4); }
	if (events.tSpin && events.clearedRowCount >= 2) { rank += 1; }
	if (events.backToBack) { rank += 1; }
	if (events.goldenLineBonus) { rank += 1; }
	if (events.perfectClear) { rank = std::max(rank, 5) + 1; }

	const unsigned int mainSize = CalloutBaseSize + static_cast<unsigned int>(rank) * CalloutSizePerRank;

	std::vector<BoardCallouts::Line> lines;
	sf::Color accent = DefaultClearColour;

	if (events.perfectClear)
	{
		lines.push_back({ text.GetText(TextKey::Callout::PerfectClear), PerfectClearColour, mainSize });
		accent = PerfectClearColour;
	}

	if (events.goldenLineBonus)
	{
		lines.push_back({ text.GetText(TextKey::Callout::Golden), GoldenColour, mainSize });
		if (!events.perfectClear)
		{
			accent = GoldenColour;
		}
	}

	sf::String main;
	if (events.backToBack)
	{
		main += text.GetText(TextKey::Callout::BackToBack) + sf::String(" ");
	}
	if (events.tSpin)
	{
		main += text.GetText(events.tSpinMini ? TextKey::Callout::TSpinMini : TextKey::Callout::TSpin);
		if (events.clearedRowCount > 0)
		{
			main += sf::String(" ") + rowSuffix(events.clearedRowCount);
		}
	}
	else if (events.clearedRowCount >= 2)
	{
		main += rowSuffix(events.clearedRowCount);
	}

	if (!main.isEmpty())
	{
		const sf::Color mainColour = events.backToBack ? BackToBackColour
			: events.tSpin ? TSpinColour
			: events.clearedRowCount == 4 ? TetrisColour
			: DefaultClearColour;
		lines.push_back({ main, mainColour, mainSize });

		if (!events.perfectClear)
		{
			accent = mainColour;
		}
	}

	if (events.rowsCleared && events.comboCount > 0)
	{
		sf::String comboText = sf::String("x") + sf::String(std::to_string(events.comboCount + 1)) + sf::String(" ")
			+ text.GetText(TextKey::Callout::Combo);
		lines.push_back({ std::move(comboText), ComboColour, CalloutComboSize });
	}

	if (!lines.empty())
	{
		boardCallouts.Show(std::move(lines), rank, accent);
	}
}

void GameplayState::FireClearHaptics(const GameplaySession::Events& events)
{
	// One pulse for whichever is the headline reason this clear stands out --
	// the row-count pulse (row_cleared / tetris) already fired separately, at
	// lock time, before this verdict was even decided.
	if (events.perfectClear)
	{
		Haptics::Pulse(context.gamepadHaptics, context.hapticSettings.perfectClear);
		Haptics::FlashLightbar(context.gamepadHaptics, context.hapticSettings.perfectClearLightbar, 0.6f, 2);
	}
	else if (events.backToBack)
	{
		Haptics::Pulse(context.gamepadHaptics, context.hapticSettings.backToBack);
	}
	else if (events.tSpin)
	{
		Haptics::Pulse(context.gamepadHaptics, context.hapticSettings.tSpin);
	}
}

void GameplayState::OpenPause()
{
	// Muffle the gameplay music while the pause menu covers the game -- as if
	// stepping into another room. PauseState::RequestResume() eases it back.
	context.musicPlayer.SetDucked(true);

	auto frame = std::make_unique<sf::RenderTexture>();

	if (frame->resize(sf::Vector2u(Display::DisplayManager::VirtualSize)))
	{
		frame->setView(sf::View(sf::FloatRect({ 0.f, 0.f }, Display::DisplayManager::VirtualSize)));
		frame->clear(sf::Color::Black);
		Render(*frame);
		frame->display();
	}
	else
	{
		frame.reset();   // capture failed -- PauseState falls back to a dim overlay
	}

	RequestPush(std::make_unique<PauseState>(context, std::move(frame)));
}

void GameplayState::Render(sf::RenderTarget& target)
{
	const sf::View originalView = target.getView();

	sf::View shakenView = originalView;
	shakenView.move(effects.GetViewOffset());
	target.setView(shakenView);

	backgroundSprite.setPosition(Display::DisplayManager::VirtualSize * 0.5f + sceneMotion.Offset());
	target.draw(backgroundSprite);

	const float deathProgress = dying
		? std::clamp(deathTimer / (DeathDuration * 0.75f), 0.f, 1.f)
		: 0.f;

	boardRenderer.Render(target, session, effects, neonGlow, deathProgress);

	if (!dying)
	{
		hud.Render(target);
		if (hud.HoldVisible())
		{
			boardRenderer.RenderHoldPreview(target, session, hud.HoldPreviewArea());
		}
		if (hud.NextVisible())
		{
			boardRenderer.RenderNextPreview(target, session, hud.NextPreviewArea());
		}
		boardRenderer.RenderHoldFlight(target);
		boardCallouts.Render(target);
	}
	else
	{
		const float d = deathTimer / DeathDuration;

		// A red slam, front-loaded, then a fade to near-black under the crumble.
		const float flash = d < 0.28f ? std::sin(d / 0.28f * Pi) : 0.f;
		sf::RectangleShape overlay(Display::DisplayManager::VirtualSize);
		overlay.setFillColor(sf::Color(200, 32, 32, static_cast<std::uint8_t>(flash * 95.f)));
		target.draw(overlay);

		// Dims toward the game-over screen's SceneDim, no cut on the swap.
		overlay.setFillColor(sf::Color(0, 0, 0, static_cast<std::uint8_t>(std::clamp(d * 1.15f, 0.f, 1.f) * 140.f)));
		target.draw(overlay);
	}

	// Leave the view as we found it -- a state stacked on top of gameplay (the
	// pause screen) must not inherit the shake offset.
	target.setView(originalView);
}
