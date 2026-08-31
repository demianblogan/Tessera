#pragma once

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Shader.hpp>
#include <SFML/Graphics/Sprite.hpp>

#include "../audio/AudioBalance.h"
#include "../audio/AudioPlayer.h"
#include "../config/HapticSettings.h"
#include "../core/Context.h"
#include "../core/StateMachine.h"
#include "../display/DisplayManager.h"
#include "../input/GamepadManager.h"
#include "../input/gamepad/GamepadHaptics.h"
#include "../localization/LocalizationManager.h"
#include "../resources/ResourceManager.h"
#include "../resources/ShaderManager.h"
#include "../settings/SettingsManager.h"
#include "../statistics/HighScoreManager.h"
#include "../ui/FpsCounter.h"
#include "../ui/GlowingCursor.h"

#include <optional>

namespace sf
{
	class Event;
}

class Application
{
private:
	static constexpr sf::Vector2f VIRTUAL_RESOLUTION{ 1920.f, 1080.f };

	// Upper bound on the delta time handed to a single Update(). A stall
	// (window drag, minimize, debugger breakpoint, OS hiccup) would otherwise
	// deliver one huge delta and make the piece "teleport" several rows or a
	// timer expire instantly. Capping turns that frame into a brief hitch.
	static constexpr float MaxFrameTime = 0.1f;

	// How far a stick axis (SFML's -100..100 scale) must move to count as the
	// player switching to the gamepad.
	static constexpr float GamepadUsageThreshold = 40.f;

	Display::DisplayManager displayManager;
	sf::RenderWindow window;

	sf::RenderTexture renderTexture;
	sf::RenderTexture gameplayTexture;
	sf::RenderTexture finalTexture;
	ShaderManager shaders;

	// The 1920x1080 world the states render into (the render textures' view).
	// The window's own view is the letterboxed fit computed by DisplayManager.
	sf::View renderView;

	StateMachine stateMachine;

	FontManager fonts;
	MusicManager music;
	SoundBufferManager soundBuffers;
	TextureManager textures;
	SettingsManager settings;
	HighScoreManager highScores;

	AudioBalance balance;
	HapticSettings hapticSettings;
	AudioPlayer audioPlayer;
	GamepadManager gamepad;
	Haptics::GamepadHaptics gamepadHaptics;
	LocalizationManager localization;

	Context context;

	// Both emplaced once loading finishes (they need loaded assets).
	std::optional<UI::FpsCounter> fpsCounter;
	std::optional<UI::GlowingCursor> cursor;

	// The system cursor is always hidden; this tracks whether the mouse is the
	// input device in use, i.e. whether to draw our own cursor sprite.
	bool cursorVisible = true;

	[[nodiscard]] bool IsWindowOpen() const;

	// The two event kinds that act on the window itself rather than on any
	// particular state: a close request, and a resize (the borderless
	// full-screen window can't actually be resized today, but a windowed mode
	// is planned). Every other event is forwarded to the active state.
	void ApplyWindowLifecycleEvent(const sf::Event& event);

	void UpdateCursorVisibility(const sf::Event& event);
	void DrawCursor(sf::RenderTarget& target);
	void HandleInput();
	void Update(float deltaTime);
	void Render();

public:
	Application();
	void Run();
};
