#include "GameplayState.h"

#include <algorithm>
#include <cmath>

#include <SFML/Window/Event.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/View.hpp>

#include "../audio/AudioPlayer.h"
#include "../resources/Assets.h"
#include "../core/Context.h"
#include "../core/StateMachine.h"
#include "../input/GamepadManager.h"
#include "../input/InputBinding.h"
#include "../config/HapticSettings.h"
#include "../input/gamepad/GamepadHaptics.h"
#include "../input/gamepad/HapticPulse.h"
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
	constexpr float LandNudge = 5.f;
	constexpr float RowClearNudge = 12.f;
	constexpr float TetrisNudge = 26.f;
}

GameplayState::GameplayState(Context& context, bool playIntro)
	: State(context.stateMachine)
	, context(context)
	, boardRenderer(context)
	, neonGlow(context.shaders.Get(Assets::ShaderID::NeonDilate), context.shaders.Get(Assets::ShaderID::NeonBlur))
	, hud(context)
	, gameplayInput(gameplayActions)
	, horizontalRepeater({ context.hapticSettings.delayedAutoShift, context.hapticSettings.autoRepeatRate })
	, backgroundSprite(context.textures.Get(Assets::TextureID::GameplayBackground))
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

	// Gameplay has no music for now -- the old track did not fit and a proper
	// dynamic-intensity score is a v1.8.0 task (Audio & HUD). Silence the shell
	// track on the way in.
	context.music.Get(Assets::MusicID::MainMenu).stop();
}

void GameplayState::ApplyGameplaySettings()
{
	const GameSettings& settings = context.settings.GetSettings();

	hud.SetVisible(GameplayHud::Element::Hold, settings.hudHold);
	hud.SetVisible(GameplayHud::Element::Next, settings.hudNext);
	hud.SetVisible(GameplayHud::Element::Score, settings.hudScore);
	hud.SetVisible(GameplayHud::Element::Lines, settings.hudLines);
	hud.SetVisible(GameplayHud::Element::Level, settings.hudLevel);
	hud.SetVisible(GameplayHud::Element::Time, settings.hudTime);
	hud.SetVisible(GameplayHud::Element::ControlsLegend, settings.hudControlsLegend);

	effects.SetShakeEnabled(settings.screenShakeEnabled);
}

void GameplayState::OnResume()
{
	ApplyGameplaySettings();
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
	gameplayActions.AddBinding(GameplayAction::Pause, InputBinding(controls.pause, Trigger::OnPress));

	gameplayInput.Subscribe(GameplayAction::MoveLeft, [this] { heldHorizontal -= 1; });
	gameplayInput.Subscribe(GameplayAction::MoveRight, [this] { heldHorizontal += 1; });
	gameplayInput.Subscribe(GameplayAction::SoftDrop, [this] { softDropHeld = true; });

	gameplayInput.Subscribe(GameplayAction::HardDrop, [this] { PerformHardDrop(); });
	gameplayInput.Subscribe(GameplayAction::RotateClockwise, [this] { TryRotate(true); });
	gameplayInput.Subscribe(GameplayAction::RotateCounterClockwise, [this] { TryRotate(false); });

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
	effects.Update(deltaTime);
	neonGlow.Update(deltaTime);
	hud.Update(deltaTime);
	sceneMotion.Update(deltaTime);

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

void GameplayState::PerformHardDrop()
{
	if (!session.IsFalling())
	{
		return;
	}

	session.HardDrop();

	context.audioPlayer.Play(Assets::SoundID::DropPiece);
	effects.TriggerShake(0.12f, 12.f);
	sceneMotion.Nudge({ 0.f, HardDropNudge });
	Haptics::Pulse(context.gamepadHaptics, context.hapticSettings.hardDrop);
}

void GameplayState::ReactToEvents(const GameplaySession::Events& events)
{
	if (events.landed)
	{
		effects.TriggerLandingFlash(events.landedBlocks);
		Haptics::Pulse(context.gamepadHaptics, context.hapticSettings.pieceLanded);
		sceneMotion.Nudge({ 0.f, LandNudge });
	}

	if (events.rowsDetected)
	{
		context.audioPlayer.Play(Assets::SoundID::RowCleared);
		effects.TriggerRowClear(events.detectedRows);

		const bool isTetris = events.detectedRows.size() >= 4;
		Haptics::Pulse(context.gamepadHaptics, isTetris ? context.hapticSettings.tetris : context.hapticSettings.rowCleared);
		sceneMotion.Nudge({ 0.f, -(isTetris ? TetrisNudge : RowClearNudge) });
	}

	if (events.rowsCleared)
	{
		hud.OnRowsCleared();
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
}

void GameplayState::OpenPause()
{
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
		if (hud.NextVisible())
		{
			boardRenderer.RenderNextPreview(target, session, hud.NextPreviewArea());
		}
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
