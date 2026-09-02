#pragma once

#include <SFML/System/String.hpp>
#include <SFML/Window/Keyboard.hpp>

namespace Input
{
	// A short, always-English label for a physical key, independent of the
	// active keyboard layout. sf::Keyboard::getDescription() would return the
	// layout-local character (on a Cyrillic layout the top-left letter key comes
	// back as a Cyrillic letter); this maps the scancode -- a physical position
	// -- straight to its US-English name.
	[[nodiscard]] sf::String KeyName(sf::Keyboard::Scancode key);
}
