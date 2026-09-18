#pragma once

#include <cstddef>

#include <SFML/System/Vector2.hpp>

namespace Display
{
	// The fixed design resolution every scene renders at; the window/view are
	// then letterboxed to fit whatever size the player's window ends up being.
	// Single source of truth -- everything that needs "the virtual canvas size"
	// (DisplayManager's letterboxing, Application's render texture, and the
	// handful of UI effects that scatter particles across the whole screen)
	// reads it from here instead of re-typing the literal.
	inline constexpr sf::Vector2f VirtualSize{ 1920.f, 1080.f };

	enum class WindowMode
	{
		Fullscreen,
		Borderless,
		Window
	};

	inline constexpr std::size_t WindowModeCount = 3;

	// The player's chosen display configuration. `resolution` is ignored for
	// Borderless (which always uses the desktop resolution). {0, 0} means "not
	// chosen yet" -- the app fills it in with the desktop resolution on load.
	struct Settings
	{
		sf::Vector2u resolution{ 0u, 0u };
		WindowMode windowMode = WindowMode::Fullscreen;
	};

	[[nodiscard]] inline bool operator==(const Settings& lhs, const Settings& rhs) noexcept
	{
		return lhs.resolution == rhs.resolution && lhs.windowMode == rhs.windowMode;
	}
}
