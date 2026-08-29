#include "GameplayState.h"

#include <algorithm>
#include <cmath>
#include <string>

#include <SFML/Window/Event.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Sprite.hpp>

#include "../audio/AudioPlayer.h"
#include "../resources/Assets.h"
#include "../core/Context.h"
#include "../core/StateMachine.h"
#include "../gameplay/Board.h"
#include "../input/GamepadManager.h"
#include "../input/InputBinding.h"
#include "../localization/LocalizationManager.h"
#include "../localization/TextKeys.h"
#include "../settings/SettingsManager.h"
#include "../settings/GameSettings.h"
#include "PauseState.h"
#include "GameOverState.h"

namespace
{
	constexpr float SoftDropInterval = 0.03f;
}

GameplayState::GameplayState(Context& context)
	: State(context.stateMachine)
	, context(context)
	, boardRenderer(context)
	, neonGlow(context.shaders.Get(Assets::ShaderID::NeonDilate), context.shaders.Get(Assets::ShaderID::NeonBlur))
	, gameplayInput(gameplayActions)
	, backgroundSprite(context.textures.Get(Assets::TextureID::GameBackground))
{
	SetUpInputBindings();
	BuildHud();

	context.music.Get(Assets::MusicID::MainMenu).stop();

	sf::Music& music = context.music.Get(Assets::MusicID::Gameplay);
	music.setLooping(true);

	if (music.getStatus() != sf::Music::Status::Playing)
	{
		music.play();
	}
}

void GameplayState::BuildHud()
{
	backgroundSprite.setColor(sf::Color(150, 150, 150));

	rightHudLayout = std::make_unique<UI::Layout>(UI::Layout::Orientation::Vertical);
	rightHudLayout->SetGap(32.f);

	sf::Sprite panelSprite(context.textures.Get(Assets::TextureID::PanelBackground));

	// =====================================================
	// Next tetromino panel
	// =====================================================
	{
		auto panel = std::make_unique<UI::Panel>(panelSprite);

		auto layout = std::make_unique<UI::Layout>(UI::Layout::Orientation::Vertical);
		layout->SetGap(20.f);

		auto label = std::make_unique<UI::Label>(context.fonts.Get(Assets::FontID::Main), context.localization.GetText(TextKey::Hud::NextPiece), 60);
		label->SetFillColor(sf::Color::White);
		label->SetMaxWidth(330.f);
		layout->Add(std::move(label));

		panel->SetChild(std::move(layout));
		panel->SetWidthPixels(450.f);
		panel->SetHeightPixels(260.f);
		panel->SetPadding({ 60.f, 50.f });

		rightHudLayout->Add(std::move(panel));
	}

	// =====================================================
	// Score panel
	// =====================================================
	{
		auto panel = std::make_unique<UI::Panel>(panelSprite);

		auto layout = std::make_unique<UI::Layout>(UI::Layout::Orientation::Vertical);
		layout->SetGap(30.f);

		{
			auto label = std::make_unique<UI::Label>(context.fonts.Get(Assets::FontID::Main),
				context.localization.FormatText(TextKey::Hud::Score, "{score}", "0"), 60);
			label->SetFillColor(sf::Color::White);
			label->SetMaxWidth(220.f);
			scoreLabel = label.get();
			layout->Add(std::move(label));
		}

		{
			auto label = std::make_unique<UI::Label>(context.fonts.Get(Assets::FontID::Main),
				context.localization.FormatText(TextKey::Hud::Level, "{level}", "1"), 60);
			label->SetFillColor(sf::Color::White);
			label->SetMaxWidth(220.f);
			levelLabel = label.get();
			layout->Add(std::move(label));
		}

		panel->SetChild(std::move(layout));
		panel->SetWidthPixels(340.f);
		panel->SetHeightPixels(200.f);
		panel->SetPadding({ 60.f, 50.f });

		rightHudLayout->Add(std::move(panel));
	}

	// =====================================================
	// Controls panel
	// =====================================================

	sf::Sprite controlsSprite(context.textures.Get(Assets::TextureID::PanelBackground));

	controlsPanel = std::make_unique<UI::Panel>(controlsSprite);

	auto controlsLayout = std::make_unique<UI::Layout>(UI::Layout::Orientation::Vertical);

	controlsLayout->SetPadding(
		{
			.left = 80.f,
			.top = 50.f,
		}
	);

	auto controlsLabel = std::make_unique<UI::Label>(context.fonts.Get(Assets::FontID::Main),
		context.localization.GetText(TextKey::Hud::Controls), 45);
	controlsLabel->SetFillColor(sf::Color::White);
	controlsLabel->SetMaxWidth(520.f);
	controlsLayout->Add(std::move(controlsLabel));

	controlsPanel->SetChild(std::move(controlsLayout));
	controlsPanel->SetWidthPixels(620.f);
	controlsPanel->SetHeightPixels(300.f);

	const sf::Vector2f rightHudSize = rightHudLayout->Measure();

	rightHudLayout->Arrange(
		{
			BoardRenderer::BoardPosition.x + Board::WIDTH * BoardRenderer::BlockSize + 100.f,
			BoardRenderer::BoardPosition.y
		},
		rightHudSize
	);

	controlsPanel->Arrange(
		{
			10.f,
			BoardRenderer::BoardPosition.y
		},
		{ 620.f, 300.f }
	);

	// Centre of the "Next Tetromino" panel's preview area: the panel sits at
	// (board right edge + 100) and is 450 wide, so its centre is +325; the
	// preview goes below the panel's title.
	nextTetrominoPreviewPosition =
	{
		BoardRenderer::BoardPosition.x + Board::WIDTH * BoardRenderer::BlockSize + 325.f,
		BoardRenderer::BoardPosition.y + 185.f
	};
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

	gameplayInput.Subscribe(GameplayAction::Pause, [this]
		{
			RequestPush(std::make_unique<PauseState>(context));
		});
}

void GameplayState::HandleEvent(const sf::Event& event)
{
	// Keyboard OnPress actions (hard drop, rotate, pause).
	gameplayInput.HandleEvent(event);

	// Gamepad pause (a button, so an event is fine). Its d-pad / stick / trigger
	// actions are polled in Update via ApplyGamepadActions.
	if (context.gamepad.IsPausePressed(event))
	{
		RequestPush(std::make_unique<PauseState>(context));
	}
}

void GameplayState::Update(float deltaTime)
{
	PollHeldInput();
	ApplyGamepadActions();
	ApplyHorizontalRepeat(deltaTime);
	ApplySoftDrop(deltaTime);
	previousHeldHorizontal = heldHorizontal;

	session.Update(deltaTime);
	effects.Update(deltaTime);
	neonGlow.Update(deltaTime);

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
}

void GameplayState::ReactToEvents(const GameplaySession::Events& events)
{
	if (events.landed)
	{
		effects.TriggerLandingFlash(events.landedBlocks);
	}

	if (events.rowsDetected)
	{
		context.audioPlayer.Play(Assets::SoundID::RowCleared);
		effects.TriggerRowClear(events.detectedRows);
	}

	if (events.rowsCleared)
	{
		scoreLabel->SetString(context.localization.FormatText(TextKey::Hud::Score, "{score}", std::to_string(session.GetScore())));
		levelLabel->SetString(context.localization.FormatText(TextKey::Hud::Level, "{level}", std::to_string(session.GetLevel())));
	}

	if (events.leveledUp)
	{
		context.audioPlayer.Play(Assets::SoundID::NextLevel);
	}

	if (events.gameOver)
	{
		RequestChange(std::make_unique<GameOverState>(context, session.GetScore()));
	}
}

void GameplayState::Render(sf::RenderTarget& target)
{
	sf::View shakenView = target.getView();
	shakenView.move(effects.GetViewOffset());
	target.setView(shakenView);

	target.draw(backgroundSprite);

	boardRenderer.Render(target, session, effects, neonGlow);

	rightHudLayout->Render(target);
	controlsPanel->Render(target);

	boardRenderer.RenderNextPreview(target, session, nextTetrominoPreviewPosition);
}
