#pragma once

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Text.hpp>

namespace sf
{
	class Font;
	class RenderTarget;
}

namespace UI
{
	// A frame-rate readout pinned to the top-right of whatever it's drawn onto.
	// Averages over a short window so the number doesn't jitter every frame.
	class FpsCounter
	{
	public:
		explicit FpsCounter(const sf::Font& font);

		// Feed the real (unclamped) seconds the last frame took.
		void Update(float frameSeconds);

		// Draws at the top-right of the target's current view.
		void Render(sf::RenderTarget& target) const;

	private:
		static constexpr float RefreshInterval = 0.5f;
		static constexpr float Margin = 24.f;
		static constexpr unsigned int CharacterSize = 34;
		static constexpr float OutlineThickness = 3.f;
		static constexpr sf::Color TextColor{ 255, 245, 170 };

		sf::Text text;
		float windowedTime = 0.f;
		int windowedFrames = 0;
	};
}
