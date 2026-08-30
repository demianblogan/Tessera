#include "Application.h"

#include <algorithm>
#include <cmath>
#include <optional>
#include <iostream>
#include <stdexcept>

#include <SFML/Window/Event.hpp>

#include <states/LoadingState.h>
#include <utils/AppDataPath.h>

#include <filesystem>

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

	const bool applyCrt = currentState == nullptr || currentState->UsesCrtEffect();

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

		if (applyCrt)
		{
			window.draw(sf::Sprite(renderTexture.getTexture()), &crtShader);
		}
		else
		{
			window.draw(sf::Sprite(renderTexture.getTexture()));
		}
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

	// Only what the loading screen itself needs before the first frame: the
	// CRT/Blur compositing shaders and the pixel font for its label. Every
	// other asset is loaded on a background thread by LoadingState.
	shaders.Load(Assets::ShaderID::CRT, Assets::Paths::Shaders::CRT, sf::Shader::Type::Fragment);
	shaders.Load(Assets::ShaderID::Blur, Assets::Paths::Shaders::Blur, sf::Shader::Type::Fragment);

	const std::filesystem::path pixelFontPath = Assets::Paths::Fonts::Pixel;
	fonts.Load(
		Assets::FontID::Pixel,
		std::filesystem::exists(pixelFontPath) ? pixelFontPath : std::filesystem::path(Assets::Paths::Fonts::Main));

	if (!localization.Load(Assets::Paths::Data::LocalizationDir))
	{
		std::cerr << "WARNING: localization catalog not found at \""
			<< Assets::Paths::Data::LocalizationDir << "\" -- the UI will show raw text keys.\n";
	}

	settings.Load();
	settings.Apply(context);

	highScores.Load();

	stateMachine.PushState(std::make_unique<LoadingState>(
		context,
		[this] { fpsCounter.emplace(fonts.Get(Assets::FontID::Main)); }));
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
