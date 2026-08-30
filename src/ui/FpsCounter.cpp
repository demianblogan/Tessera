#include "FpsCounter.h"

#include <cmath>
#include <string>

#include <SFML/Graphics/RenderTarget.hpp>

namespace UI
{
	FpsCounter::FpsCounter(const sf::Font& font)
		: text(font, "FPS: --", 34)
	{
		text.setFillColor(sf::Color(255, 245, 170));
		text.setOutlineColor(sf::Color::Black);
		text.setOutlineThickness(3.f);
	}

	void FpsCounter::Update(float frameSeconds)
	{
		windowedTime += frameSeconds;
		windowedFrames++;

		if (windowedTime < RefreshInterval)
		{
			return;
		}

		const int fps = static_cast<int>(std::lround(windowedFrames / windowedTime));
		text.setString("FPS: " + std::to_string(fps));

		windowedTime = 0.f;
		windowedFrames = 0;
	}

	void FpsCounter::Render(sf::RenderTarget& target) const
	{
		sf::Text drawn = text;

		const sf::FloatRect bounds = drawn.getLocalBounds();
		drawn.setOrigin({ bounds.position.x + bounds.size.x, bounds.position.y });
		drawn.setPosition({ target.getView().getSize().x - Margin, Margin });

		target.draw(drawn);
	}
}
