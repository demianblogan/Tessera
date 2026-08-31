#pragma once

#include <cstddef>
#include <vector>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Rect.hpp>
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
	// A single line of "menu" text drawn the way the main-menu ring entries are:
	// a dark outline, a vertical gradient fill in one hue, a soft drop shadow,
	// and a gentle per-letter idle wave. DrawGlow adds the additive neon bloom.
	// Shared by the ring, the transition header and the plain menu buttons.
	class MenuLabel
	{
	public:
		MenuLabel(const sf::Font& font, unsigned int characterSize);

		void SetText(const sf::String& text);
		void Update(float deltaTime);   // advances the idle-wave phase

		[[nodiscard]] sf::Vector2f InkSize() const { return inkSize; }
		[[nodiscard]] float InkCentreY() const { return inkCentreY; }

		// Axis-aligned bounds of the ink when drawn at `centre` / `scale`
		// (ignores the small wave), for hit-testing.
		[[nodiscard]] sf::FloatRect Bounds(sf::Vector2f centre, float scale) const;

		// Crisp draw. `whiten` mixes the whole label toward white (a flash);
		// `alpha` scales opacity.
		void Draw(sf::RenderTarget& target, sf::Vector2f centre, float scale, sf::Color colour,
			float alpha = 1.f, float whiten = 0.f) const;

		// Additive bloom over the label's silhouette, in a fixed-size box so
		// NeonGlow never re-sizes mid-animation.
		void DrawGlow(sf::RenderTarget& target, NeonGlow& glow, sf::Vector2f centre, float scale,
			sf::Color tint) const;

	private:
		struct Glyph
		{
			char32_t codepoint = 0;
			float penX = 0.f;   // pen origin, relative to the string centre
		};

		[[nodiscard]] float WaveOffset(std::size_t index) const;

		const sf::Font& font;
		unsigned int characterSize;

		std::vector<Glyph> glyphs;
		sf::Vector2f inkSize;
		float inkCentreY = 0.f;
		sf::Vector2f glowBoxSize;

		float waveTime = 0.f;
	};
}
