#include "DropInTitle.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/System/Angle.hpp>

#include "../rendering/NeonGlow.h"

namespace
{
	constexpr float StaggerDelay = 0.09f;    // gap between successive letters starting
	constexpr float FallDuration = 0.32f;
	constexpr float SettleDuration = 0.55f;
	constexpr float DropDistance = 750.f;    // how far above the resting spot a letter starts
	constexpr float SquashY = 0.60f;         // vertical scale at the moment of impact
	constexpr float StretchX = 1.35f;        // horizontal scale at the moment of impact

	// White-hot flash on impact, decaying back to the letter's colour.
	constexpr float FlashDuration = 0.16f;

	constexpr float OutlineThickness = 4.f;

	// One per letter, cycled: the seven classic tetromino colours, matching
	// the loading-bar blocks.
	constexpr std::array<sf::Color, 7> LetterPalette{
		sf::Color{ 0, 240, 240 },    // I - cyan
		sf::Color{ 245, 220, 40 },   // O - yellow
		sf::Color{ 180, 60, 240 },   // T - purple
		sf::Color{ 60, 230, 90 },    // S - green
		sf::Color{ 240, 60, 70 },    // Z - red
		sf::Color{ 70, 110, 240 },   // J - blue
		sf::Color{ 245, 160, 40 },   // L - orange
	};

	[[nodiscard]] sf::Color Darken(sf::Color colour, float factor) noexcept
	{
		return sf::Color{
			static_cast<std::uint8_t>(static_cast<float>(colour.r) * factor),
			static_cast<std::uint8_t>(static_cast<float>(colour.g) * factor),
			static_cast<std::uint8_t>(static_cast<float>(colour.b) * factor) };
	}

	[[nodiscard]] sf::Color MixToWhite(sf::Color colour, float t) noexcept
	{
		return sf::Color{
			static_cast<std::uint8_t>(colour.r + (255 - colour.r) * t),
			static_cast<std::uint8_t>(colour.g + (255 - colour.g) * t),
			static_cast<std::uint8_t>(colour.b + (255 - colour.b) * t) };
	}

	// Gentle idle motion once a letter has settled: a travelling vertical
	// bob with a touch of sway, easing in so there is no jump.
	constexpr float WaveAmplitude = 7.f;
	constexpr float WaveSpeed = 2.3f;
	constexpr float WavePhaseStep = 0.7f;    // radians of offset between neighbouring letters
	constexpr float WaveSwayDegrees = 1.6f;
	constexpr float WaveRampDuration = 0.7f;

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
		float rotationDegrees = 0.f;
	};

	[[nodiscard]] GlyphPose PoseAt(float local, std::size_t index) noexcept
	{
		if (local <= 0.f)
		{
			return { -DropDistance, 1.f, 1.f, 0.f };
		}

		if (local < FallDuration)
		{
			const float p = local / FallDuration;
			return { -DropDistance * (1.f - p * p), 1.f, 1.f, 0.f };
		}

		const float settle = std::clamp((local - FallDuration) / SettleDuration, 0.f, 1.f);
		const float e = EaseOutElastic(settle);

		GlyphPose pose{ 0.f, Lerp(StretchX, 1.f, e), Lerp(SquashY, 1.f, e), 0.f };

		const float sinceSettled = local - FallDuration - SettleDuration;
		if (sinceSettled > 0.f)
		{
			const float ramp = std::clamp(sinceSettled / WaveRampDuration, 0.f, 1.f);
			const float phase = local * WaveSpeed + static_cast<float>(index) * WavePhaseStep;
			pose.y += ramp * WaveAmplitude * std::sin(phase);
			pose.rotationDegrees += ramp * WaveSwayDegrees * std::sin(phase * 0.5f);
		}

		return pose;
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
			const sf::Color colour = LetterPalette[i % LetterPalette.size()];

			Glyph glyph{ sf::Text(font, sf::String(placed[i].codepoint), characterSize), colour, 0.f, 0.f, 0.f, 0.f };
			const sf::FloatRect gb = glyph.text.getLocalBounds();
			glyph.text.setOrigin({ gb.position.x + gb.size.x * 0.5f, gb.position.y + gb.size.y * 0.5f });
			glyph.text.setFillColor(colour);
			glyph.text.setOutlineColor(Darken(colour, 0.28f));
			glyph.text.setOutlineThickness(OutlineThickness);

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

	void DropInTitle::Render(sf::RenderTarget& target, NeonGlow* glow) const
	{
		// Each letter as it looks right now (transform + flash colour).
		std::vector<sf::Text> shaped;
		shaped.reserve(glyphs.size());

		for (std::size_t i = 0; i < glyphs.size(); ++i)
		{
			const Glyph& glyph = glyphs[i];
			const float local = glyph.elapsed - glyph.startDelay;
			const GlyphPose pose = PoseAt(local, i);

			sf::Color fill = glyph.colour;
			const float impact = local - FallDuration;
			if (impact >= 0.f && impact < FlashDuration)
			{
				const float f = 1.f - impact / FlashDuration;
				fill = MixToWhite(glyph.colour, f * f);
			}

			sf::Text drawn = glyph.text;
			drawn.setPosition({ center.x + glyph.offsetX, center.y + glyph.offsetY + pose.y });
			drawn.setScale({ pose.scaleX, pose.scaleY });
			drawn.setRotation(sf::degrees(pose.rotationDegrees));
			drawn.setFillColor(fill);
			shaped.push_back(std::move(drawn));
		}

		if (glow != nullptr)
		{
			for (std::size_t i = 0; i < shaped.size(); ++i)
			{
				if (glyphs[i].elapsed - glyphs[i].startDelay <= 0.f)
				{
					continue;
				}

				const sf::FloatRect bounds = shaped[i].getGlobalBounds();
				const float slack = 14.f;
				const sf::FloatRect area{
					{ bounds.position.x - slack, bounds.position.y - slack },
					{ bounds.size.x + slack * 2.f, bounds.size.y + slack * 2.f } };

				const sf::Text& letter = shaped[i];
				glow->Draw(target, area,
					[&letter](sf::RenderTarget& buffer, const sf::RenderStates& states)
					{
						sf::Text silhouette = letter;
						silhouette.setFillColor(sf::Color::White);
						silhouette.setOutlineColor(sf::Color::White);
						buffer.draw(silhouette, states);
					},
					shaped[i].getFillColor());
			}
		}

		for (const sf::Text& letter : shaped)
		{
			target.draw(letter);
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
