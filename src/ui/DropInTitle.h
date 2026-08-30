#pragma once

#include <vector>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/System/String.hpp>
#include <SFML/System/Vector2.hpp>

class NeonGlow;

namespace sf
{
	class Font;
	class RenderTarget;
}

namespace UI
{
	// The main-menu title, animated once on entry: each letter drops in from
	// above in turn, squashes on impact and springs back to shape. After the
	// last letter settles the whole thing just sits there as static text.
	class DropInTitle
	{
	public:
		DropInTitle(const sf::Font& font, const sf::String& text, unsigned int characterSize);

		// Screen point the finished word is centred on.
		void SetCenter(sf::Vector2f center);

		void Update(float deltaTime);

		// If `glow` is given, each letter gets a pulsing neon bloom in its own
		// colour, drawn behind the crisp text.
		void Render(sf::RenderTarget& target, NeonGlow* glow = nullptr) const;

		[[nodiscard]] bool IsFinished() const;

	private:
		struct Glyph
		{
			sf::Text text;
			sf::Color colour;      // resting fill colour
			float offsetX = 0.f;   // resting position, relative to the word centre
			float offsetY = 0.f;
			float startDelay = 0.f;
			float elapsed = 0.f;
		};

		std::vector<Glyph> glyphs;
		sf::Vector2f center;
	};
}
