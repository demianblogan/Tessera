#pragma once

#include <SFML/System/Vector2.hpp>

namespace Display
{
	enum class WindowMode
	{
		Fullscreen,
		Borderless,
		Window
	};

	// The player's chosen display configuration. `resolution` is ignored for
	// Borderless (which always uses the desktop resolution). {0, 0} means "not
	// chosen yet" -- the app fills it in with the desktop resolution on load.
	struct Mode
	{
		sf::Vector2u resolution{ 0u, 0u };
		WindowMode windowMode = WindowMode::Borderless;
	};

	[[nodiscard]] inline bool operator==(const Mode& lhs, const Mode& rhs) noexcept
	{
		return lhs.resolution == rhs.resolution && lhs.windowMode == rhs.windowMode;
	}
}
