#include "GameplayState.h"

#include <algorithm>
#include <cmath>
#include <numbers>
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
#include "../haptics/HapticSettings.h"
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
	constexpr float Pi = std::numbers::pi_v<float>;
	constexpr float BackgroundScale = 1.07f;

	// Parallax impulses handed to SceneMotion, for events GameplayState itself
	// reacts to (see GameplayInputController.cpp for the input-driven ones).
	constexpr float LandNudge = 5.f;
	constexpr float RowClearNudge = 12.f;
	constexpr float TetrisNudge = 26.f;

	// On-board callout look: color by what earned it; text grows with the
	// event's rank (0 = least special), one fixed smaller size for the combo
	// count underneath it.
	constexpr unsigned int CalloutBaseSize = 66;
	constexpr unsigned int CalloutSizePerRank = 8;
	constexpr unsigned int CalloutComboSize = 48;

	const sf::Color DefaultClearColor{ 235, 240, 248 };
	const sf::Color TetrisColor{ 120, 230, 255 };
	const sf::Color TSpinColor{ 220, 130, 255 };
	const sf::Color BackToBackColor{ 255, 190, 80 };
	const sf::Color PerfectClearColor{ 255, 215, 60 };
	const sf::Color ComboColor{ 160, 220, 255 };

	// Escalation (see EscalationDirector).
	const sf::Color SpeedSurgeColor{ 255, 90, 70 };
	const sf::Color GoldenColor{ 255, 205, 40 };

	// The backdrop is desaturated toward this grey before SceneMotion's parallax
	// and the death-beat overlays darken it further.
	const sf::Color BackdropTint{ 150, 150, 150 };

	// Update() -- how long the lightbar throbs green per frame while rows are
	// clearing (re-flashed every frame the phase holds, so this only needs to
	// outlast one frame).
	constexpr float RowClearLightbarDuration = 0.2f;

	// ReactToEvents() -- row-clear shake: base duration/amplitude, plus a
	// per-rank increment so a Tetris hits harder than a Single.
	constexpr float RowClearShakeDurationBase = 0.08f;
	constexpr float RowClearShakeDurationPerRank = 0.09f;
	constexpr float RowClearShakeAmplitudeBase = 3.f;
	constexpr float RowClearShakeAmplitudePerRank = 9.f;

	constexpr float PerfectClearShakeDuration = 0.3f;
	constexpr float PerfectClearShakeAmplitude = 14.f;
	constexpr float PerfectClearLightbarDuration = 0.6f;
	constexpr int PerfectClearLightbarFlashes = 2;

	constexpr sf::Vector2f LevelUpNudge{ 16.f, -12.f };

	constexpr float GameOverShakeDuration = 0.5f;
	constexpr float GameOverShakeAmplitude = 26.f;
	constexpr float GameOverLightbarDuration = 0.9f;
	constexpr int GameOverLightbarFlashes = 3;

	constexpr float SpeedSurgeShakeDuration = 0.25f;
	constexpr float SpeedSurgeShakeAmplitude = 10.f;

	constexpr float GarbagePushedShakeDuration = 0.15f;
	constexpr float GarbagePushedShakeAmplitude = 7.f;

	// Render()'s death beat: the red slam fires over the first fraction of
	// DeathDuration and fades; the black overlay ramps up a little faster than
	// linear so it's fully dark before the game-over screen (which dims to the
	// same alpha) cuts in.
	constexpr float DeathVisibleFraction = 0.75f;
	constexpr float DeathFlashSpan = 0.28f;
	constexpr std::uint8_t DeathFlashAlpha = 95;
	constexpr float DeathDimRampScale = 1.15f;
	constexpr std::uint8_t DeathDimAlpha = 140;
	const sf::Color DeathFlashColor{ 200, 32, 32 };

	// Screen-space center of a board grid cell -- the same placement
	// BoardRenderer draws locked cells at, used to spawn row-clear shards and
	// to center the T-spin burst on the piece that just locked.
	[[nodiscard]] sf::Vector2f CellCenter(int gridX, int gridY)
	{
		return
		{
			BoardRenderer::BoardPosition.x + (static_cast<float>(gridX) + 0.5f) * BoardRenderer::BlockSize,
			BoardRenderer::BoardPosition.y
				+ (static_cast<float>(gridY - Board::BufferHeight) + 0.5f) * BoardRenderer::BlockSize
		};
	}
}

GameplayState::GameplayState(Context& context, bool isIntroPlayed)
	: State(context.stateMachine)
	, context(context)
	, session(GameplaySession::Config{
		static_cast<int>(context.settings.GetSettings().nextQueueLength),
		context.settings.GetSettings().isSevenBagEnabled })
	, boardRenderer(context)
	, neonGlow(context.shaders.Get(Assets::ShaderID::NeonDilate), context.shaders.Get(Assets::ShaderID::NeonBlur))
	, HUD(context)
	, boardCallouts(context)
	, inputController(context, session, effects, sceneMotion, boardRenderer, HUD, [this] { OpenPause(); })
	, backgroundSprite(context.textures.Get(Assets::TextureID::GameplayBackground))
	, seenLocalizationRevision(context.localization.GetRevision())
{
	isIntroActive = isIntroPlayed;

	// The backdrop is drawn slightly oversized and centered so SceneMotion can
	// slide it a little without exposing an edge.
	backgroundSprite.setColor(BackdropTint);
	const sf::Vector2f backgroundSize(context.textures.Get(Assets::TextureID::GameplayBackground).getSize());
	backgroundSprite.setOrigin(backgroundSize * 0.5f);
	backgroundSprite.setScale({ BackgroundScale, BackgroundScale });

	ApplyGameplaySettings();

	// Switches MusicPlayer to the shuffled gameplay playlist, stopping
	// whatever was playing before (the menu shell track, normally).
	context.musicPlayer.PlayGameplay();
}

void GameplayState::ApplyGameplaySettings()
{
	if (seenLocalizationRevision != context.localization.GetRevision())
	{
		seenLocalizationRevision = context.localization.GetRevision();
		HUD.RefreshText();
	}

	const GameSettings& settings = context.settings.GetSettings();

	HUD.SetVisible(GameplayHUD::Element::Hold, settings.isHoldTetrominoPanelVisible && settings.isHoldTetrominoEnabled);
	HUD.SetVisible(GameplayHUD::Element::Next, settings.isNextTetrominoPanelVisible);
	HUD.SetVisible(GameplayHUD::Element::Score, settings.isScorePanelVisible);
	HUD.SetVisible(GameplayHUD::Element::Lines, settings.isLinesPanelVisible);
	HUD.SetVisible(GameplayHUD::Element::Level, settings.isLevelPanelVisible);
	HUD.SetVisible(GameplayHUD::Element::Time, settings.isTimePanelVisible);
	HUD.SetVisible(GameplayHUD::Element::ControlsLegend, settings.isControlsLegendPanelVisible);
	HUD.RefreshControlsLegend(settings.controls, settings.isHoldTetrominoEnabled);

	effects.SetShakeEnabled(settings.isScreenShakeEnabled);
	boardRenderer.SetGhostEnabled(settings.isGhostPieceEnabled);
}

void GameplayState::HandleEvent(const sf::Event& event)
{
	if (isIntroActive)
	{
		return;
	}

	inputController.HandleEvent(event);
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
	HUD.Update(deltaTime);
	sceneMotion.Update(deltaTime);
	boardCallouts.Update(deltaTime);

	if (isIntroActive)
	{
		introTimer += deltaTime;
		if (introTimer >= IntroDuration)
		{
			isIntroActive = false;
		}
		return;
	}

	if (isDying)
	{
		deathTimer += deltaTime;
		if (deathTimer >= DeathDuration)
		{
			RequestChange(std::make_unique<GameOverState>(context, session.GetScore(),
				session.GetLinesCleared(), session.GetLevel(), session.GetElapsedSeconds()));
		}
		return;
	}

	inputController.Update(deltaTime);

	session.Update(deltaTime);
	boardRenderer.Update(deltaTime, session);

	HUD.Set(session.GetScore(), session.GetLevel(), session.GetLinesCleared(), session.GetElapsedSeconds());

	// Hold a green throb on the lightbar for as long as rows are clearing.
	if (session.GetPhase() == GameplaySession::Phase::ClearingRows)
	{
		Haptics::FlashLightbar(context.gamepadHaptics, context.hapticSettings.rowClearLightbar, RowClearLightbarDuration);
	}

	ReactToEvents(session.ConsumeEvents());
}

void GameplayState::ReactToEvents(const GameplaySession::Events& events)
{
	// Escalation tier ambience (see EscalationDirector::Tier) -- polled every
	// frame rather than off a tier-changed event, so the border glow just eases
	// toward wherever the current tier points it.
	const sf::FloatRect boardArea{ BoardRenderer::BoardPosition,
		{ Board::Width * BoardRenderer::BlockSize, Board::VisibleHeight * BoardRenderer::BlockSize } };
	effects.SetEscalationTier(static_cast<int>(session.GetEscalationTier()), boardArea);

	if (events.hasLanded)
	{
		effects.TriggerLandingFlash(events.landedBlocks);
		Haptics::TriggerPulse(context.gamepadHaptics, context.hapticSettings.pieceLanded);
		sceneMotion.Nudge({ 0.f, LandNudge });

		if (inputController.IsHardDropAnimationPending())
		{
			inputController.ClearHardDropAnimationPending();

			int landedTopRow = Board::Height;
			for (const sf::Vector2i& block : events.landedBlocks)
			{
				landedTopRow = std::min(landedTopRow, block.y);
			}

			const int droppedRows = landedTopRow - inputController.GetHardDropStartRow();
			boardRenderer.TriggerHardDropFlight(inputController.GetHardDropType(), events.landedBlocks, droppedRows);

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

				const bool restsOnFloor = belowRow >= Board::Height;
				const bool restsOnStack = !restsOnFloor
					&& grid[static_cast<std::size_t>(belowRow)][static_cast<std::size_t>(block.x)].isOccupied;

				if (!restsOnFloor && !restsOnStack)
				{
					continue;
				}

				// Bottom edge of the cell, not its center -- dust kicks up from
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
		if (!events.hasDetectedRows)
		{
			effects.SetCombo(0);
		}

		// A T-spin's own tell, independent of whether it cleared any lines --
		// decided at lock time, same batch as landed.
		if (events.isTSpin)
		{
			sf::Vector2f center{ 0.f, 0.f };
			for (const sf::Vector2i& block : events.landedBlocks)
			{
				center += CellCenter(block.x, block.y);
			}
			center /= static_cast<float>(events.landedBlocks.size());
			effects.TriggerTSpinBurst(center);
		}
	}

	if (events.hasDetectedRows)
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
			for (int x = 0; x < Board::Width; ++x)
			{
				const Cell& cell = grid[static_cast<std::size_t>(row)][static_cast<std::size_t>(x)];
				if (!cell.isOccupied)
				{
					continue;
				}

				const int textureIndex = cell.kind == Cell::Kind::Garbage
					? BoardRenderer::WallTextureIndex
					: static_cast<int>(cell.tetrominoType);
				clearedCells.push_back({ CellCenter(x, row), textureIndex });
			}
		}

		effects.TriggerRowClear(events.detectedRows, rank, clearedCells);

		const bool isTetris = rank >= 3;
		effects.TriggerShake(RowClearShakeDurationBase + RowClearShakeDurationPerRank * static_cast<float>(rank),
			RowClearShakeAmplitudeBase + RowClearShakeAmplitudePerRank * static_cast<float>(rank));
		Haptics::TriggerPulse(context.gamepadHaptics, isTetris ? context.hapticSettings.tetris : context.hapticSettings.rowCleared);
		sceneMotion.Nudge({ 0.f, -(isTetris ? TetrisNudge : RowClearNudge) });
	}

	if (events.hasClearedRows)
	{
		HUD.OnRowsCleared(std::clamp(events.clearedRowCount - 1, 0, 3));
		effects.SetCombo(events.comboCount);

		if (events.isPerfectClear)
		{
			effects.TriggerPerfectClearBurst(
				{ BoardRenderer::BoardPosition, { Board::Width * BoardRenderer::BlockSize, Board::VisibleHeight * BoardRenderer::BlockSize } });
			effects.TriggerShake(PerfectClearShakeDuration, PerfectClearShakeAmplitude);
		}
	}

	// rowsCleared (with the combo/back-to-back/Perfect Clear verdict) only
	// arrives once the clear delay resolves, a separate ConsumeEvents() batch
	// from the landed/rowsDetected one at lock time -- landed is long since
	// false again by then. A T-spin that cleared nothing is the one case that
	// arrives together with landed, since it's decided at lock time.
	if (events.hasClearedRows || events.isTSpin)
	{
		ShowClearCallout(events);
		FireClearHaptics(events);
	}

	if (events.hasLeveledUp)
	{
		context.audioPlayer.Play(Assets::SoundID::NextLevel);
		Haptics::TriggerPulse(context.gamepadHaptics, context.hapticSettings.levelUp);
		HUD.OnLevelUp();
		sceneMotion.Nudge(LevelUpNudge);
	}

	if (events.isGameOver)
	{
		Haptics::TriggerPulse(context.gamepadHaptics, context.hapticSettings.gameOver);
		Haptics::FlashLightbar(context.gamepadHaptics, context.hapticSettings.gameOverLightbar,
			GameOverLightbarDuration, GameOverLightbarFlashes);
		effects.TriggerShake(GameOverShakeDuration, GameOverShakeAmplitude);

		isDying = true;
		deathTimer = 0.f;
	}

	// Escalation (see EscalationDirector): a Speed Surge is telegraphed with a
	// callout, a shake and a rumble, so a sudden gravity spike reads as a fair
	// warning rather than a glitch. A garbage row is deliberately quiet -- just
	// a dull thud -- since it happens often once unlocked.
	if (events.hasSpeedSurgeStarted)
	{
		boardCallouts.Show(
			{ { context.localization.GetText(TextKey::Callout::SpeedSurge), SpeedSurgeColor, CalloutBaseSize } },
			1, SpeedSurgeColor);
		Haptics::TriggerPulse(context.gamepadHaptics, context.hapticSettings.speedSurge);
		effects.TriggerShake(SpeedSurgeShakeDuration, SpeedSurgeShakeAmplitude);
		effects.TriggerSpeedSurgeGlow(EscalationDirector::SurgeDuration);
	}

	if (events.hasGarbagePushed)
	{
		Haptics::TriggerPulse(context.gamepadHaptics, context.hapticSettings.garbageRow);
		effects.TriggerGarbageWave();
		effects.TriggerShake(GarbagePushedShakeDuration, GarbagePushedShakeAmplitude);
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
	if (events.isTSpin) { rank = std::max(rank, events.isTSpinMini ? 2 : 4); }
	if (events.isTSpin && events.clearedRowCount >= 2) { rank += 1; }
	if (events.hasBackToBack) { rank += 1; }
	if (events.hasGoldenLineBonus) { rank += 1; }
	if (events.isPerfectClear) { rank = std::max(rank, 5) + 1; }

	const unsigned int mainSize = CalloutBaseSize + static_cast<unsigned int>(rank) * CalloutSizePerRank;

	std::vector<BoardCallouts::Line> lines;
	sf::Color accent = DefaultClearColor;

	if (events.isPerfectClear)
	{
		lines.push_back({ text.GetText(TextKey::Callout::PerfectClear), PerfectClearColor, mainSize });
		accent = PerfectClearColor;
	}

	if (events.hasGoldenLineBonus)
	{
		lines.push_back({ text.GetText(TextKey::Callout::Golden), GoldenColor, mainSize });
		if (!events.isPerfectClear)
		{
			accent = GoldenColor;
		}
	}

	sf::String main;
	if (events.hasBackToBack)
	{
		main += text.GetText(TextKey::Callout::BackToBack) + sf::String(" ");
	}
	if (events.isTSpin)
	{
		main += text.GetText(events.isTSpinMini ? TextKey::Callout::TSpinMini : TextKey::Callout::TSpin);
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
		const sf::Color mainColor = events.hasBackToBack ? BackToBackColor
			: events.isTSpin ? TSpinColor
			: events.clearedRowCount == 4 ? TetrisColor
			: DefaultClearColor;
		lines.push_back({ main, mainColor, mainSize });

		if (!events.isPerfectClear)
		{
			accent = mainColor;
		}
	}

	if (events.hasClearedRows && events.comboCount > 0)
	{
		sf::String comboText = sf::String("x") + sf::String(std::to_string(events.comboCount + 1)) + sf::String(" ")
			+ text.GetText(TextKey::Callout::Combo);
		lines.push_back({ std::move(comboText), ComboColor, CalloutComboSize });
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
	if (events.isPerfectClear)
	{
		Haptics::TriggerPulse(context.gamepadHaptics, context.hapticSettings.perfectClear);
		Haptics::FlashLightbar(context.gamepadHaptics, context.hapticSettings.perfectClearLightbar,
			PerfectClearLightbarDuration, PerfectClearLightbarFlashes);
	}
	else if (events.hasBackToBack)
	{
		Haptics::TriggerPulse(context.gamepadHaptics, context.hapticSettings.backToBack);
	}
	else if (events.isTSpin)
	{
		Haptics::TriggerPulse(context.gamepadHaptics, context.hapticSettings.tSpin);
	}
}

bool GameplayState::IsCursorVisible() const
{
	return false;
}

void GameplayState::OpenPause()
{
	// Muffle the gameplay music while the pause menu covers the game -- as if
	// stepping into another room. PauseState::RequestResume() eases it back.
	context.musicPlayer.SetDucked(true);

	auto frame = std::make_unique<sf::RenderTexture>();

	if (frame->resize(sf::Vector2u(Display::VirtualSize)))
	{
		frame->setView(sf::View(sf::FloatRect({ 0.f, 0.f }, Display::VirtualSize)));
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

	backgroundSprite.setPosition(Display::VirtualSize * 0.5f + sceneMotion.GetOffset());
	target.draw(backgroundSprite);

	const float deathProgress = isDying
		? std::clamp(deathTimer / (DeathDuration * DeathVisibleFraction), 0.f, 1.f)
		: 0.f;

	boardRenderer.Render(target, session, effects, neonGlow, deathProgress);

	if (!isDying)
	{
		HUD.Render(target);
		if (HUD.HoldVisible())
		{
			boardRenderer.RenderHoldPreview(target, session, HUD.HoldPreviewArea());
		}
		if (HUD.NextVisible())
		{
			boardRenderer.RenderNextPreview(target, session, HUD.NextPreviewArea());
		}
		boardRenderer.RenderHoldFlight(target);
		boardCallouts.Render(target);
	}
	else
	{
		const float deathFraction = deathTimer / DeathDuration;

		// A red slam, front-loaded, then a fade to near-black under the crumble.
		const float flash = deathFraction < DeathFlashSpan ? std::sin(deathFraction / DeathFlashSpan * Pi) : 0.f;
		sf::RectangleShape overlay(Display::VirtualSize);
		overlay.setFillColor(sf::Color(DeathFlashColor.r, DeathFlashColor.g, DeathFlashColor.b,
			static_cast<std::uint8_t>(flash * static_cast<float>(DeathFlashAlpha))));
		target.draw(overlay);

		// Dims toward the game-over screen's SceneDim, no cut on the swap.
		overlay.setFillColor(sf::Color(0, 0, 0,
			static_cast<std::uint8_t>(std::clamp(deathFraction * DeathDimRampScale, 0.f, 1.f) * static_cast<float>(DeathDimAlpha))));
		target.draw(overlay);
	}

	// Leave the view as we found it -- a state stacked on top of gameplay (the
	// pause screen) must not inherit the shake offset.
	target.setView(originalView);
}
