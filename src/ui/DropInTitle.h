#pragma once

#include <cstddef>
#include <cstdint>
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
	// above in turn, squashes on impact and springs back, then eases into a
	// gentle idle wave in both position and glow. Each letter carries one of
	// the tetromino colours, a dark outline and a vertical gradient fill, with
	// a pulsing neon bloom behind it. Landing throws a white flash, a shockwave
	// ring, a brief chromatic split and a small kick to the whole word. An
	// optional dim reflection sits below.
	class DropInTitle
	{
	public:
		DropInTitle(const sf::Font& font, const sf::String& text, unsigned int characterSize);

		void SetCenter(sf::Vector2f center);
		void SetReflectionEnabled(bool enabled);

		void Update(float deltaTime);
		void Render(sf::RenderTarget& target, NeonGlow* glow = nullptr) const;

		[[nodiscard]] bool IsFinished() const;

	private:
		struct Glyph
		{
			sf::Text text;         // font + size + character; re-coloured per draw
			char32_t codepoint = 0;
			sf::Color colour;      // resting fill colour
			float offsetX = 0.f;   // resting position, relative to the word centre
			float offsetY = 0.f;
			float startDelay = 0.f;
			float elapsed = 0.f;
		};

		struct Pose
		{
			bool visible = false;
			float y = 0.f;
			float scaleX = 1.f;
			float scaleY = 1.f;
			float rotationDegrees = 0.f;
			float glowStrength = 1.f;   // multiplies the bloom tint
			float flash = 0.f;          // 0..1 white-hot on impact
			float aberration = 0.f;     // 0..1 chromatic split on impact
			float shock = -1.f;         // 0..1 shockwave progress, <0 = inactive
			bool falling = false;
			float local = 0.f;          // seconds since this glyph started
		};

		[[nodiscard]] Pose EvaluateGlyph(std::size_t index) const;
		[[nodiscard]] sf::Vector2f RestingPosition(std::size_t index, const Pose& pose) const;

		void DrawGradientLetter(sf::RenderTarget& target, std::size_t index, const Pose& pose,
			sf::Vector2f drawPosition, float scaleSignY, std::uint8_t alpha) const;

		const sf::Font& font;
		unsigned int characterSize;

		std::vector<Glyph> glyphs;
		sf::Vector2f center;
		bool reflectionEnabled = true;

		// Small vertical spring: every landing nudges the whole word.
		float kickOffset = 0.f;
		float kickVelocity = 0.f;
	};
}
