#include "DropInTitle.h"

#include <algorithm>
#include <cmath>

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

namespace
{
	constexpr float StaggerDelay = 0.09f;    // gap between successive letters starting
	constexpr float FallDuration = 0.32f;
	constexpr float SettleDuration = 0.55f;
	constexpr float DropDistance = 750.f;    // how far above the resting spot a letter starts
	constexpr float SquashY = 0.60f;         // vertical scale at the moment of impact
	constexpr float StretchX = 1.35f;        // horizontal scale at the moment of impact

	[[nodiscard]] float Lerp(float a, float b, float t) noexcept
	{
		return a + (b - a) * t;
	}

	// Overshooting spring: 0 at t=0, 1 at t=1, wobbling past 1 on the way.
	[[nodiscard]] float EaseOutElastic(float t) noexcept
	{
		if (t <= 0.f) return 0.f;
		if (t >= 1.f) return 1.f;

		constexpr float period = 2.f * 3.14159265f / 3.f;
		return std::pow(2.f, -10.f * t) * std::sin((t * 10.f - 0.75f) * period) + 1.f;
	}

	struct GlyphPose
	{
		float y = 0.f;
		float scaleX = 1.f;
		float scaleY = 1.f;
	};

	[[nodiscard]] GlyphPose PoseAt(float local) noexcept
	{
		if (local <= 0.f)
		{
			return { -DropDistance, 1.f, 1.f };
		}

		if (local < FallDuration)
		{
			const float p = local / FallDuration;
			return { -DropDistance * (1.f - p * p), 1.f, 1.f };
		}

		const float settle = std::clamp((local - FallDuration) / SettleDuration, 0.f, 1.f);
		const float e = EaseOutElastic(settle);
		return { 0.f, Lerp(StretchX, 1.f, e), Lerp(SquashY, 1.f, e) };
	}
}

namespace UI
{
	DropInTitle::DropInTitle(const sf::Font& font, const sf::String& text, unsigned int characterSize)
	{
		// Vertical extent of the whole word, for keeping the letters on one line.
		const sf::FloatRect wordBounds = sf::Text(font, text, characterSize).getLocalBounds();
		const float wordCentreY = wordBounds.position.y + wordBounds.size.y * 0.5f;

		// Walk the pen across the string using the font's own advances/kerning
		// (sf::Text::findCharacterPos is deprecated in this SFML build).
		struct Placed { char32_t codepoint; float centreX; };
		std::vector<Placed> placed;
		float penX = 0.f;
		char32_t previous = 0;

		for (std::size_t i = 0; i < text.getSize(); ++i)
		{
			const char32_t codepoint = text[i];
			if (previous != 0)
			{
				penX += font.getKerning(previous, codepoint, characterSize);
			}

			const float advance = font.getGlyph(codepoint, characterSize, false).advance;
			if (codepoint != U' ')
			{
				placed.push_back({ codepoint, penX + advance * 0.5f });
			}

			penX += advance;
			previous = codepoint;
		}

		const float wordCentreX = penX * 0.5f;

		for (std::size_t i = 0; i < placed.size(); ++i)
		{
			Glyph glyph{ sf::Text(font, sf::String(placed[i].codepoint), characterSize), 0.f, 0.f, 0.f, 0.f };
			const sf::FloatRect gb = glyph.text.getLocalBounds();
			glyph.text.setOrigin({ gb.position.x + gb.size.x * 0.5f, gb.position.y + gb.size.y * 0.5f });

			glyph.offsetX = placed[i].centreX - wordCentreX;
			// The glyph's own ink centre, measured against the whole word's, keeps
			// the letters on a common line once they land.
			glyph.offsetY = (gb.position.y + gb.size.y * 0.5f) - wordCentreY;
			glyph.startDelay = static_cast<float>(i) * StaggerDelay;

			glyphs.push_back(std::move(glyph));
		}
	}

	void DropInTitle::SetCenter(sf::Vector2f newCenter)
	{
		center = newCenter;
	}

	void DropInTitle::Update(float deltaTime)
	{
		for (Glyph& glyph : glyphs)
		{
			glyph.elapsed += deltaTime;
		}
	}

	void DropInTitle::Render(sf::RenderTarget& target) const
	{
		for (const Glyph& glyph : glyphs)
		{
			const GlyphPose pose = PoseAt(glyph.elapsed - glyph.startDelay);

			sf::Text drawn = glyph.text;
			drawn.setPosition({ center.x + glyph.offsetX, center.y + glyph.offsetY + pose.y });
			drawn.setScale({ pose.scaleX, pose.scaleY });
			target.draw(drawn);
		}
	}

	bool DropInTitle::IsFinished() const
	{
		for (const Glyph& glyph : glyphs)
		{
			if (glyph.elapsed - glyph.startDelay < FallDuration + SettleDuration)
			{
				return false;
			}
		}

		return true;
	}
}
