#pragma once

#include <vector>

#include <SFML/Graphics/View.hpp>
#include <SFML/System/Vector2.hpp>

#include "DisplayMode.h"

namespace sf
{
	class RenderWindow;
}

// Owns the window's size / mode. It enumerates the resolutions the display
// supports, (re)creates the sf::RenderWindow for a chosen Mode, and computes
// the letterboxed view that fits the fixed 1920x1080 render inside whatever
// size the window ended up.
//
// Window recreation tears down and rebuilds the GL context -- only do it on an
// explicit Apply, never per-frame.
namespace Display
{
	class DisplayManager
	{
	public:
		static constexpr sf::Vector2f VirtualSize{ 1920.f, 1080.f };

		DisplayManager();

		// Ascending by pixel count; always contains the desktop resolution.
		[[nodiscard]] const std::vector<sf::Vector2u>& AvailableResolutions() const { return resolutions; }
		[[nodiscard]] sf::Vector2u DesktopResolution() const { return desktop; }

		// (Re)create `window` for `mode`, then fit its view. Hides the OS cursor.
		void Apply(sf::RenderWindow& window, const Mode& mode) const;

		// Recompute the letterboxed view for the window's current size (on resize).
		void FitView(sf::RenderWindow& window) const;

		[[nodiscard]] static sf::View LetterboxView(sf::Vector2u windowSize);

	private:
		std::vector<sf::Vector2u> resolutions;
		sf::Vector2u desktop;
	};
}
