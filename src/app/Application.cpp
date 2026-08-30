#include "Application.h"

#include <algorithm>
#include <cmath>
#include <optional>
#include <iostream>
#include <stdexcept>

#include <SFML/Window/Event.hpp>

#include <states/MainMenuState.h>
#include <utils/AppDataPath.h>

bool Application::IsWindowOpen() const
{
	return window.isOpen();
}

void Application::ApplyWindowLifecycleEvent(const sf::Event& event)
{
	if (event.is<sf::Event::Closed>())
	{
		window.close();
	}
	else if (event.is<sf::Event::Resized>())
	{
		// Keep rendering in the fixed 1920x1080 virtual space; SFML stretches
		// it to whatever size the window now is. Proper letterboxing arrives
		// with the DisplayManager port.
		window.setView(gameView);
	}
}

void Application::UpdateCursorVisibility(const sf::Event& event)
{
	// The mouse cursor shows only while the mouse is the device in use.
	// A key press or gamepad input hides it; moving the mouse brings it back.
	bool showCursor = cursorVisible;

	if (event.is<sf::Event::MouseMoved>() || event.is<sf::Event::MouseButtonPressed>())
	{
		showCursor = true;
	}
	else if (event.is<sf::Event::KeyPressed>() || event.is<sf::Event::JoystickButtonPressed>())
	{
		showCursor = false;
	}
	else if (const auto* moved = event.getIf<sf::Event::JoystickMoved>())
	{
		if (std::abs(moved->position) > GamepadUsageThreshold)
		{
			showCursor = false;
		}
	}

	if (showCursor != cursorVisible)
	{
		window.setMouseCursorVisible(showCursor);
		cursorVisible = showCursor;
	}
}

void Application::HandleInput()
{
	while (const std::optional<sf::Event> event = window.pollEvent())
	{
		gamepad.HandleEvent(*event);
		UpdateCursorVisibility(*event);
		ApplyWindowLifecycleEvent(*event);
		if (!window.isOpen())
		{
			return;
		}

		if (State* currentState = stateMachine.GetCurrentState())
		{
			currentState->HandleEvent(*event);
		}

		if (!window.isOpen())
		{
			return;
		}

		// A state just asked for a transition -- stop feeding this frame's
		// remaining events to a state that is about to be replaced.
		if (stateMachine.HasPendingChanges())
		{
			return;
		}
	}
}

void Application::Update(float deltaTime)
{
	context.totalTime += deltaTime;
	gamepad.Update();
	gamepadHaptics.Update(deltaTime);

	if (State* currentState = stateMachine.GetCurrentState())
	{
		currentState->Update(deltaTime);
	}
}

void Application::Render()
{
	sf::Shader& crtShader = context.shaders.Get(Assets::ShaderID::CRT);
	sf::Shader& blurShader = context.shaders.Get(Assets::ShaderID::Blur);

	State* currentState = stateMachine.GetCurrentState();

	const bool blurBackdrop = currentState != nullptr
		&& currentState->GetBackdrop() == State::Backdrop::BlurredPrevious;

	// =====================================================
	// Opaque state: render the stack straight to the screen
	// =====================================================

	window.clear();
	crtShader.setUniform("time", context.totalTime);

	if (!blurBackdrop)
	{
		renderTexture.clear();
		renderTexture.setView(gameView);
		stateMachine.RenderStates(renderTexture);
		renderTexture.display();

		window.draw(sf::Sprite(renderTexture.getTexture()), &crtShader);
	}
	else
	{
		// The states below, blurred, then the top state drawn crisp on top.
		gameplayTexture.clear();
		gameplayTexture.setView(gameView);
		stateMachine.RenderStatesExceptTop(gameplayTexture);
		gameplayTexture.display();

		finalTexture.clear();
		finalTexture.draw(sf::Sprite(gameplayTexture.getTexture()), &blurShader);
		stateMachine.RenderTopState(finalTexture);
		finalTexture.display();

		window.draw(sf::Sprite(finalTexture.getTexture()), &crtShader);
	}

	// A crisp overlay, drawn after the CRT pass so it isn't warped by it.
	if (fpsCounter && context.settings.GetSettings().showFps)
	{
		fpsCounter->Render(window);
	}

	window.display();
}

Application::Application()
	// Members are listed in declaration order so the initialisation order is
	// obvious; `context` is last because it binds references to the rest.
	: window(sf::VideoMode::getDesktopMode(), "Tessera", sf::Style::None, sf::State::Windowed)
	, gameView({ VIRTUAL_RESOLUTION / 2.f, VIRTUAL_RESOLUTION })
	, settings(AppDataPath::Resolve(SaveFile::Settings))
	, highScores(AppDataPath::Resolve(SaveFile::Scores))
	, audioPlayer(soundBuffers)
	, context(
		stateMachine,
		window,
		fonts,
		music,
		soundBuffers,
		textures,
		shaders,
		audioPlayer,
		settings,
		highScores,
		gamepad,
		gamepadHaptics,
		localization)
{
	window.setMouseCursorVisible(true);
	window.setView(gameView);

	// Menu navigation gets a faint haptic tick for free once this is wired.
	gamepad.SetHaptics(&gamepadHaptics);

	const sf::Vector2u renderTextureSize
	{
		static_cast<unsigned int>(VIRTUAL_RESOLUTION.x),
		static_cast<unsigned int>(VIRTUAL_RESOLUTION.y)
	};

	if (!renderTexture.resize(renderTextureSize) ||
		!gameplayTexture.resize(renderTextureSize) ||
		!finalTexture.resize(renderTextureSize))
	{
		throw std::runtime_error("Failed to allocate render textures.");
	}

	shaders.Load(Assets::ShaderID::CRT, Assets::Paths::Shaders::CRT, sf::Shader::Type::Fragment);
	shaders.Load(Assets::ShaderID::Blur, Assets::Paths::Shaders::Blur, sf::Shader::Type::Fragment);
	shaders.Load(Assets::ShaderID::GhostTetromino, Assets::Paths::Shaders::GhostTetromino, sf::Shader::Type::Fragment);
	shaders.Load(Assets::ShaderID::NeonDilate, Assets::Paths::Shaders::NeonDilate, sf::Shader::Type::Fragment);
	shaders.Load(Assets::ShaderID::NeonBlur, Assets::Paths::Shaders::NeonBlur, sf::Shader::Type::Fragment);

	fonts.Load(Assets::FontID::Main, Assets::Paths::Fonts::Main);
	fpsCounter.emplace(fonts.Get(Assets::FontID::Main));

	textures.Load(Assets::TextureID::BlockSpritesheetWithOutline, Assets::Paths::Textures::BlockSpritesheetWithOutline);
	textures.Load(Assets::TextureID::BlockSpritesheetWithoutOutline, Assets::Paths::Textures::BlockSpritesheetWithoutOutline);
	textures.Load(Assets::TextureID::ButtonBackground, Assets::Paths::Textures::ButtonBackground);
	textures.Load(Assets::TextureID::MenuBackground, Assets::Paths::Textures::MenuBackground);
	textures.Load(Assets::TextureID::TitleBackground, Assets::Paths::Textures::TitleBackground);
	textures.Load(Assets::TextureID::PanelBackground, Assets::Paths::Textures::PanelBackground);
	textures.Load(Assets::TextureID::GameBackground, Assets::Paths::Textures::GameBackground);

	music.Load(Assets::MusicID::MainMenu, Assets::Paths::Music::MainMenu);
	music.Load(Assets::MusicID::Gameplay, Assets::Paths::Music::Gameplay);
	music.Load(Assets::MusicID::GameOver, Assets::Paths::Music::GameOver);

	soundBuffers.Load(Assets::SoundID::MenuItemSelected, Assets::Paths::Sounds::MenuItemSelected);
	soundBuffers.Load(Assets::SoundID::MenuItemPressed, Assets::Paths::Sounds::MenuItemPressed);
	soundBuffers.Load(Assets::SoundID::DropPiece, Assets::Paths::Sounds::DropPiece);
	soundBuffers.Load(Assets::SoundID::MovePiece, Assets::Paths::Sounds::MovePiece);
	soundBuffers.Load(Assets::SoundID::RotatePiece, Assets::Paths::Sounds::RotatePiece);
	soundBuffers.Load(Assets::SoundID::PieceHitWall, Assets::Paths::Sounds::PieceHitWall);
	soundBuffers.Load(Assets::SoundID::NextLevel, Assets::Paths::Sounds::NextLevel);
	soundBuffers.Load(Assets::SoundID::RowCleared, Assets::Paths::Sounds::RowCleared);

	if (!localization.Load(Assets::Paths::Data::LocalizationDir))
	{
		std::cerr << "WARNING: localization catalog not found at \""
			<< Assets::Paths::Data::LocalizationDir << "\" -- the UI will show raw text keys.\n";
	}

	settings.Load();
	settings.Apply(context);

	highScores.Load();

	stateMachine.PushState(std::make_unique<MainMenuState>(context));
	stateMachine.ApplyPendingChanges();
}

void Application::Run()
{
	sf::Clock deltaTimeClock;

	while (IsWindowOpen())
	{
		// The raw frame time drives the FPS readout; a clamped copy drives
		// gameplay so one stall can't teleport a piece several rows.
		const float frameSeconds = deltaTimeClock.restart().asSeconds();
		const float deltaTime = std::min(frameSeconds, MaxFrameTime);

		if (fpsCounter)
		{
			fpsCounter->Update(frameSeconds);
		}

		HandleInput();
		stateMachine.ApplyPendingChanges();

		Update(deltaTime);
		stateMachine.ApplyPendingChanges();

		Render();

		audioPlayer.RemoveStoppedSounds();
	}
}
