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
	constexpr unsigned int MinWidth = 1280u;
	constexpr unsigned int MinHeight = 720u;
}

namespace Display
{
	DisplayManager::DisplayManager()
	{
		desktop = sf::VideoMode::getDesktopMode().size;

		std::vector<std::pair<unsigned int, unsigned int>> seen;
		const auto add = [&](sf::Vector2u size)
		{
			const std::pair<unsigned int, unsigned int> key{ size.x, size.y };
			if (std::find(seen.begin(), seen.end(), key) == seen.end())
			{
				seen.push_back(key);
				resolutions.push_back(size);
			}
		};

		for (const sf::VideoMode& videoMode : sf::VideoMode::getFullscreenModes())
		{
			const sf::Vector2u size = videoMode.size;
			if (size.x >= MinWidth && size.y >= MinHeight && size.x <= desktop.x && size.y <= desktop.y)
			{
				add(size);
			}
		}

		add(desktop);   // always offer the native resolution

		std::sort(resolutions.begin(), resolutions.end(),
			[](sf::Vector2u lhs, sf::Vector2u rhs)
			{
				return static_cast<std::uint64_t>(lhs.x) * lhs.y < static_cast<std::uint64_t>(rhs.x) * rhs.y;
			});
	}

	sf::View DisplayManager::LetterboxView(sf::Vector2u windowSize)
	{
		sf::View view;
		view.setCenter(VirtualSize / 2.f);
		view.setSize(VirtualSize);

		if (windowSize.x == 0u || windowSize.y == 0u)
		{
			return view;
		}

		const float windowAspect = static_cast<float>(windowSize.x) / static_cast<float>(windowSize.y);
		const float virtualAspect = VirtualSize.x / VirtualSize.y;

		sf::Vector2f viewport{ 1.f, 1.f };
		if (windowAspect > virtualAspect)
		{
			viewport.x = virtualAspect / windowAspect;
		}
		else
		{
			viewport.y = windowAspect / virtualAspect;
		}

		view.setViewport(sf::FloatRect(
			{ (1.f - viewport.x) * 0.5f, (1.f - viewport.y) * 0.5f }, viewport));
		return view;
	}

	void DisplayManager::FitView(sf::RenderWindow& window) const
	{
		window.setView(LetterboxView(window.getSize()));
	}

	bool DisplayManager::ApplyPending(sf::RenderWindow& window)
	{
		if (!pendingApply)
		{
			return false;
		}

		Apply(window, *pendingApply);
		pendingApply.reset();
		return true;
	}

	void DisplayManager::Apply(sf::RenderWindow& window, const Mode& mode) const
	{
		sf::Vector2u resolution = mode.resolution;
		if (resolution.x == 0u || resolution.y == 0u)
		{
			resolution = desktop;
		}

		switch (mode.windowMode)
		{
		case WindowMode::Fullscreen:
		{
			sf::VideoMode videoMode(resolution);
			if (!videoMode.isValid())
			{
				videoMode = sf::VideoMode::getDesktopMode();
			}
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

		window.setMouseCursorVisible(false);
		FitView(window);
	}
}
