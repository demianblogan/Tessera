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

		// When false, the per-letter idle wave is suppressed and the label draws
		// flat (used for the unselected buttons in a column).
		void SetWaveEnabled(bool isWaveEnabled);

		// Pin the glow box to a fixed size (e.g. the widest button in a column),
		// so NeonGlow does not re-size its buffers when the glow moves between
		// labels of different widths. Zero restores the per-label size.
		void SetGlowBoxSize(sf::Vector2f size);

		[[nodiscard]] sf::Vector2f GetInkSize() const;
		[[nodiscard]] float GetInkCenterY() const;

		// Axis-aligned bounds of the ink when drawn at `center` / `scale`
		// (ignores the small wave), for hit-testing.
		[[nodiscard]] sf::FloatRect GetBounds(sf::Vector2f center, float scale) const;

		// Crisp draw. `whiten` mixes the whole label toward white (a flash);
		// `alpha` scales opacity.
		void Draw(sf::RenderTarget& target, sf::Vector2f center, float scale, sf::Color color,
			float alpha = 1.f, float whiten = 0.f) const;

		// Additive bloom over the label's silhouette, in a fixed-size box so
		// NeonGlow never re-sizes mid-animation.
		void DrawGlow(sf::RenderTarget& target, NeonGlow& glow, sf::Vector2f center, float scale,
			sf::Color tint) const;

		// The glow box currently in use (fixed if one was set, else derived from
		// the text). Lets a column pick one size for all its buttons.
		[[nodiscard]] sf::Vector2f GetGlowBox() const;

	private:
		struct Glyph
		{
			char32_t codepoint = 0;
			float penX = 0.f;   // pen origin, relative to the string center
		};

		[[nodiscard]] float GetWaveOffset(std::size_t index) const;

		const sf::Font* font;
		unsigned int characterSize;

		std::vector<Glyph> glyphs;
		sf::Vector2f inkSize;
		float inkCenterY = 0.f;
		sf::Vector2f autoGlowBoxSize;     // derived from the text
		sf::Vector2f fixedGlowBoxSize;    // {0,0} => use autoGlowBoxSize

		float waveTime = 0.f;
		bool isWaveEnabled = true;
	};
}
