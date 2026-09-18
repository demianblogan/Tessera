#pragma once

#include <optional>
#include <vector>

#include <SFML/Graphics/View.hpp>
#include <SFML/System/Vector2.hpp>

#include "DisplaySettings.h"

namespace sf
{
	class RenderWindow;
}

// Owns the window's size / mode. It enumerates the resolutions the display
// supports, (re)creates the sf::RenderWindow for a chosen Settings, and computes
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
		DisplayManager();

		// Ascending by pixel count; always contains the desktop resolution.
		[[nodiscard]] const std::vector<sf::Vector2u>& GetAvailableResolutions() const;
		[[nodiscard]] sf::Vector2u GetDesktopResolution() const;

		// (Re)create `window` for `mode`, then fit its view. Also re-hides the OS
		// cursor -- window.create() resets it to visible, and the game draws its
		// own cursor (UI::GlowingCursor), so this is the one place that has to
		// remember to turn the OS one back off.
		void Apply(sf::RenderWindow& window, const Settings& mode) const;

		// Queue a recreation to be done between frames (never mid event loop).
		void RequestApply(const Settings& mode);

		// If a mode is queued, apply it to `window` and clear the queue.
		// Returns true if it recreated the window.
		bool ApplyPendingMode(sf::RenderWindow& window);

		// Recompute the letterboxed view for the window's current size (on resize).
		void FitView(sf::RenderWindow& window) const;

		[[nodiscard]] static sf::View GetLetterboxView(sf::Vector2u windowSize);

	private:
		std::vector<sf::Vector2u> resolutions;
		sf::Vector2u desktop;
		std::optional<Settings> pendingModeForApply;
	};
}
