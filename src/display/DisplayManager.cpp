#include "DisplayManager.h"

#include <algorithm>
#include <cstdint>
#include <utility>

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/VideoMode.hpp>

namespace
{
	constexpr const char* WindowTitle = "Tessera";

	// Below this the UI (fixed 1920x1080, letterboxed) would be unreadably small.
	constexpr unsigned int MinDisplayWidth = 1280u;
	constexpr unsigned int MinDisplayHeight = 720u;
}

namespace Display
{
	DisplayManager::DisplayManager()
	{
		desktop = sf::VideoMode::getDesktopMode().size;

		// getFullscreenModes() can list the same resolution more than once (at
		// different refresh rates / bit depths), and `desktop` can duplicate one
		// of those entries -- this tracks which resolutions already made it into
		// `resolutions` so each one is offered only once.
		std::vector<std::pair<unsigned int, unsigned int>> addedResolutionKeys;

		const auto addResolutionIfNew = [this, &addedResolutionKeys](sf::Vector2u size)
		{
			const std::pair<unsigned int, unsigned int> resolutionKey{ size.x, size.y };

			if (std::find(addedResolutionKeys.begin(), addedResolutionKeys.end(), resolutionKey) == addedResolutionKeys.end())
			{
				addedResolutionKeys.push_back(resolutionKey);
				resolutions.push_back(size);
			}
		};

		for (const sf::VideoMode& videoMode : sf::VideoMode::getFullscreenModes())
		{
			const sf::Vector2u size = videoMode.size;

			if (size.x >= MinDisplayWidth && size.y >= MinDisplayHeight &&
				size.x <= desktop.x && size.y <= desktop.y)
			{
				addResolutionIfNew(size);
			}
		}

		addResolutionIfNew(desktop); // always offer the native resolution

		std::sort(resolutions.begin(), resolutions.end(),
			[](sf::Vector2u lhs, sf::Vector2u rhs)
			{
				return static_cast<std::uint64_t>(lhs.x) * lhs.y < static_cast<std::uint64_t>(rhs.x) * rhs.y;
			});
	}

	const std::vector<sf::Vector2u>& DisplayManager::GetAvailableResolutions() const
	{
		return resolutions;
	}

	sf::Vector2u DisplayManager::GetDesktopResolution() const
	{
		return desktop;
	}

	void DisplayManager::RequestApply(const Settings& mode)
	{
		pendingModeForApply = mode;
	}

	sf::View DisplayManager::GetLetterboxView(sf::Vector2u windowSize)
	{
		sf::View view;
		view.setCenter(VirtualSize / 2.f);
		view.setSize(VirtualSize);

		if (windowSize.x == 0u || windowSize.y == 0u)
			return view;

		const float windowAspectRatio = static_cast<float>(windowSize.x) / static_cast<float>(windowSize.y);
		const float virtualAspectRatio = VirtualSize.x / VirtualSize.y;

		sf::Vector2f viewport{ 1.f, 1.f };
		if (windowAspectRatio > virtualAspectRatio)
			viewport.x = virtualAspectRatio / windowAspectRatio;
		else
			viewport.y = windowAspectRatio / virtualAspectRatio;

		const sf::FloatRect letterboxRect({ (1.f - viewport.x) * 0.5f, (1.f - viewport.y) * 0.5f }, viewport);
		view.setViewport(letterboxRect);

		return view;
	}

	void DisplayManager::FitView(sf::RenderWindow& window) const
	{
		window.setView(GetLetterboxView(window.getSize()));
	}

	bool DisplayManager::ApplyPendingMode(sf::RenderWindow& window)
	{
		if (!pendingModeForApply.has_value())
			return false;

		Apply(window, *pendingModeForApply);
		pendingModeForApply.reset();

		return true;
	}

	void DisplayManager::Apply(sf::RenderWindow& window, const Settings& mode) const
	{
		sf::Vector2u resolution = mode.resolution;

		if (resolution.x == 0u || resolution.y == 0u)
			resolution = desktop;

		switch (mode.windowMode)
		{
		case WindowMode::Fullscreen:
		{
			sf::VideoMode videoMode(resolution);
			if (!videoMode.isValid())
				videoMode = sf::VideoMode::getDesktopMode();

			window.create(videoMode, WindowTitle, sf::Style::Default, sf::State::Fullscreen);
			break;
		}

		case WindowMode::Borderless:
			window.create(sf::VideoMode(desktop), WindowTitle, sf::Style::None, sf::State::Windowed);
			break;

		case WindowMode::Window:
			window.create(sf::VideoMode(resolution), WindowTitle, sf::Style::Default, sf::State::Windowed);
			break;
		}

		// window.create() above always resets the OS cursor to visible, so it has
		// to be hidden again here every time -- the game draws its own cursor
		// (UI::GlowingCursor) and the system one must stay off.
		window.setMouseCursorVisible(false);

		FitView(window);
	}
}
