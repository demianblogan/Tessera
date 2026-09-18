#include "Application.h"

#include <algorithm>
#include <cmath>
#include <optional>
#include <stdexcept>

#include <SFML/Window/Event.hpp>
#include <SFML/Window/Mouse.hpp>

#include <gameplay/KickDataFile.h>
#include <gameplay/PieceDataFile.h>
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
		// Refit the 1920x1080 render into the new window size (letterboxed).
		displayManager.FitView(window);
	}
}

void Application::UpdateCursorVisibility(const sf::Event& event)
{
	// Our cursor sprite shows only while the mouse is the device in use.
	// A key press or gamepad input hides it; moving the mouse brings it back.
	// The system cursor stays hidden throughout.
	bool needToShowCursor = isCursorVisible;

	if (event.is<sf::Event::MouseMoved>() || event.is<sf::Event::MouseButtonPressed>())
	{
		needToShowCursor = true;
	}
	else if (event.is<sf::Event::KeyPressed>() || event.is<sf::Event::JoystickButtonPressed>())
	{
		needToShowCursor = false;
	}
	else if (const auto* moved = event.getIf<sf::Event::JoystickMoved>())
	{
		if (std::abs(moved->position) > GamepadUsageThreshold)
			needToShowCursor = false;
	}

	isCursorVisible = needToShowCursor;
}

void Application::HandleInput()
{
	while (const std::optional<sf::Event> event = window.pollEvent())
	{
		gamepad.HandleEvent(*event);
		UpdateCursorVisibility(*event);
		ApplyWindowLifecycleEvent(*event);

		if (!window.isOpen())
			return;

		if (State* currentState = stateMachine.GetCurrentState())
			currentState->HandleEvent(*event);

		if (!window.isOpen())
			return;

		// A state just asked for a transition -- stop feeding this frame's
		// remaining events to a state that is about to be replaced.
		if (stateMachine.HasPendingChanges())
			return;
	}
}

void Application::Update(float deltaTime)
{
	context.totalTime += deltaTime;
	gamepad.Update();
	gamepadHaptics.Update(deltaTime);

	// Ticked unconditionally (not just while GameplayState is the active
	// state) so the gameplay playlist keeps advancing and the pause duck
	// keeps easing while the pause menu covers the game.
	musicPlayer.Update(deltaTime, settings.GetSettings().musicVolume);

	if (cursor.has_value())
		cursor->Update(deltaTime);

	if (State* currentState = stateMachine.GetCurrentState())
		currentState->Update(deltaTime);
}

void Application::DrawCursor(sf::RenderTarget& target)
{
	if (!cursor.has_value() || !isCursorVisible)
		return;

	const State* currentState = stateMachine.GetCurrentState();
	if (currentState != nullptr && !currentState->IsCursorVisible())
		return;

	// Window pixel -> virtual (1920x1080) coordinates, so the cursor lands in
	// the same space the states render in and picks up the CRT pass with them.
	const sf::Vector2f position = window.mapPixelToCoords(sf::Mouse::getPosition(window), window.getView());
	cursor->Render(target, position);
}

void Application::Render()
{
	sf::Shader& CRTShader = context.shaders.Get(Assets::ShaderID::CRT);

	window.clear();
	CRTShader.setUniform("time", context.totalTime);

	const bool isCRTFilterApplied = context.settings.GetSettings().isCRTFilterEnabled;

	renderTexture.clear();
	renderTexture.setView(renderView);
	stateMachine.RenderStates(renderTexture);
	DrawCursor(renderTexture);
	renderTexture.display();

	const sf::Sprite frame(renderTexture.getTexture());
	if (isCRTFilterApplied)
		window.draw(frame, &CRTShader);
	else
		window.draw(frame);

	// A crisp overlay, drawn after the CRT pass so its scanlines / aberration
	// don't touch the readout.
	if (FPSCounter.has_value() && context.settings.GetSettings().needToShowFPS)
		FPSCounter->Render(window);

	window.display();
}

Application::Application()
// Members are listed in declaration order so the initialisation order is
// obvious; `context` is last because it binds references to the rest.
	: renderView(sf::FloatRect({ 0.f, 0.f }, Display::VirtualSize))
	, settings(AppDataPath::Resolve(SaveFile::Settings))
	, highScores(AppDataPath::Resolve(SaveFile::Scores))
	, balance(Assets::Paths::Data::AudioBalance)
	, hapticSettings(Assets::Paths::Data::Haptics)
	, audioPlayer(soundBuffers, balance)
	, musicPlayer(music, balance)
	, context(
		stateMachine,
		window,
		fonts,
		music,
		soundBuffers,
		textures,
		shaders,
		audioPlayer,
		balance,
		musicPlayer,
		hapticSettings,
		displayManager,
		settings,
		highScores,
		gamepad,
		gamepadHaptics,
		localization)
{
	// The window is created here from the saved display settings (mode +
	// resolution). A fresh install has no resolution yet -- fill in the native
	// one. The window draws its own cursor (UI::GlowingCursor); the OS one
	// stays off.
	settings.Load();

	// Authored tetromino shapes and wall kicks, overriding the built-in SRS
	// layout / kick tables if present.
	PieceDataFile::Load(Assets::Paths::Data::Pieces);
	KickDataFile::Load(Assets::Paths::Data::SrsKicks);

	if (settings.GetSettings().display.resolution.x == 0u)
		settings.GetSettings().display.resolution = displayManager.GetDesktopResolution();

	displayManager.Apply(window, settings.GetSettings().display);

	// Menu navigation gets a faint haptic tick for free once this is wired.
	gamepad.SetHaptics(&gamepadHaptics);
	gamepad.SetHapticSettings(&hapticSettings);

	const sf::Vector2u renderTextureSize
	{
		static_cast<unsigned int>(Display::VirtualSize.x),
		static_cast<unsigned int>(Display::VirtualSize.y)
	};

	if (!renderTexture.resize(renderTextureSize))
		throw std::runtime_error("Failed to allocate render textures.");

	// Textures and shaders are loaded here, on the main thread: creating GPU
	// objects on a background thread deadlocks some drivers. They are small;
	// the loading screen then streams the audio and fonts in the background.
	namespace TexturePaths = Assets::Paths::Textures;
	textures.Load(Assets::TextureID::BlockSpritesheetWithOutline, TexturePaths::BlockSpritesheetWithOutline);
	textures.Load(Assets::TextureID::MenuBackground, TexturePaths::MenuBackground);
	textures.Load(Assets::TextureID::GameplayBackground, TexturePaths::GameplayBackground);
	textures.Load(Assets::TextureID::CompanyLogo, TexturePaths::CompanyLogo);
	textures.Load(Assets::TextureID::Cursor, TexturePaths::Cursor);
	textures.Load(Assets::TextureID::UiArrow, TexturePaths::UiArrow);
	textures.Load(Assets::TextureID::UiFrameCyan, TexturePaths::UiFrameCyan);
	textures.Load(Assets::TextureID::UiFrameBlue, TexturePaths::UiFrameBlue);
	textures.Load(Assets::TextureID::UiFrameGreen, TexturePaths::UiFrameGreen);
	textures.Load(Assets::TextureID::UiFramePurple, TexturePaths::UiFramePurple);
	textures.Load(Assets::TextureID::UiFrameBrown, TexturePaths::UiFrameBrown);
	textures.Load(Assets::TextureID::UiFrameRed, TexturePaths::UiFrameRed);
	textures.Load(Assets::TextureID::UiFrameWhiteRed, TexturePaths::UiFrameWhiteRed);
	textures.Load(Assets::TextureID::UiFrameWarning, TexturePaths::UiFrameWarning);
	textures.Load(Assets::TextureID::CarouselArrow, TexturePaths::CarouselArrow);
	textures.Load(Assets::TextureID::Checkbox, TexturePaths::Checkbox);

	// Left unsmoothed: the button-prompt icons are tiny and get scaled up a lot
	// for the Gamepad table -- nearest-neighbour keeps them crisp on 4K.
	textures.Load(Assets::TextureID::XboxGamepadLayout, TexturePaths::XboxGamepadLayout);
	textures.Load(Assets::TextureID::PlayStationGamepadLayout, TexturePaths::PlayStationGamepadLayout);

	namespace ShaderPaths = Assets::Paths::Shaders;
	shaders.Load(Assets::ShaderID::CRT, ShaderPaths::CRT, sf::Shader::Type::Fragment);
	shaders.Load(Assets::ShaderID::GhostTetromino, ShaderPaths::GhostTetromino, sf::Shader::Type::Fragment);
	shaders.Load(Assets::ShaderID::NeonDilate, ShaderPaths::NeonDilate, sf::Shader::Type::Fragment);
	shaders.Load(Assets::ShaderID::NeonBlur, ShaderPaths::NeonBlur, sf::Shader::Type::Fragment);
	shaders.Load(Assets::ShaderID::MenuAurora, ShaderPaths::MenuAurora, sf::Shader::Type::Fragment);
	shaders.Load(Assets::ShaderID::Mosaic, ShaderPaths::Mosaic, sf::Shader::Type::Fragment);

	const std::filesystem::path loadingFontPath = Assets::Paths::Fonts::Loading;
	fonts.Load(
		Assets::FontID::Loading,
		std::filesystem::exists(loadingFontPath) ? loadingFontPath : std::filesystem::path(Assets::Paths::Fonts::Main));

	// The shell track (loading screen -> splash -> menu) is opened here so the
	// loading screen can start it right away. openFromFile only reads the
	// stream header; decoding streams on sf::Music's own thread during play.
	music.Load(Assets::MusicID::MainMenu, Assets::Paths::Music::MainMenu);

	// A missing/invalid catalog just leaves the UI showing raw text keys --
	// see LocalizationManager for the fallback behaviour.
	static_cast<void>(localization.Load(Assets::Paths::Data::LocalizationDir, settings.GetSettings().language));

	settings.Apply(context);

	highScores.Load();

	stateMachine.PushState(std::make_unique<LoadingState>(
		context,
		[this]
		{
			FPSCounter.emplace(fonts.Get(Assets::FontID::Main));
			cursor.emplace(textures.Get(Assets::TextureID::Cursor));

			// The music tracks are loaded on the background thread, i.e. after
			// the first settings.Apply() ran with an empty music map -- so the
			// audio settings have to be applied again now they exist.
			settings.Apply(context);
		}));
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

		if (FPSCounter.has_value())
			FPSCounter->Update(frameSeconds);

		HandleInput();
		stateMachine.ApplyPendingChanges();

		// A settings panel may have asked for a new window mode / resolution;
		// recreate the window now, safely outside the event loop.
		if (displayManager.ApplyPendingMode(window))
			settings.Apply(context);

		Update(deltaTime);
		stateMachine.ApplyPendingChanges();

		Render();

		audioPlayer.RemoveStoppedSounds();
	}
}
