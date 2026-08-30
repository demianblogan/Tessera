#include "MenuGrid.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

#include <SFML/Graphics/PrimitiveType.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/VertexArray.hpp>

namespace
{
	constexpr float HorizonY = 560.f;
	constexpr float BottomY = 1080.f;
	constexpr float VanishX = 960.f;

	constexpr int RowCount = 26;
	constexpr float RowBunch = 2.2f;     // > 1 packs the rows toward the horizon
	constexpr float ScrollSpeed = 0.16f; // loops per second

	constexpr int VerticalCount = 21;    // odd, so one line sits dead centre
	constexpr float VerticalHalfSpread = 2500.f;   // half-width of the grid at the bottom edge

	constexpr sf::Color GridColour{ 236, 46, 158 };     // neon magenta
	constexpr sf::Color HorizonColour{ 130, 225, 255 }; // cyan accent on the horizon line
	constexpr float BaseAlpha = 0.42f;

	[[nodiscard]] float Frac(float value) noexcept
	{
		return value - std::floor(value);
	}

	[[nodiscard]] float SmoothStep(float edge0, float edge1, float x) noexcept
	{
		const float t = std::clamp((x - edge0) / (edge1 - edge0), 0.f, 1.f);
		return t * t * (3.f - 2.f * t);
	}

	// Screen y for a row at loop position u in [0,1): 0 at the horizon, 1 at
	// the bottom, bunched toward the horizon.
	[[nodiscard]] float RowScreenY(float u) noexcept
	{
		return HorizonY + (BottomY - HorizonY) * std::pow(u, RowBunch);
	}

	void AppendLine(sf::VertexArray& lines, sf::Vector2f a, sf::Vector2f b, sf::Color colourA, sf::Color colourB)
	{
		lines.append({ a, colourA });
		lines.append({ b, colourB });
	}

	[[nodiscard]] sf::Color WithAlpha(sf::Color colour, float alpha) noexcept
	{
		colour.a = static_cast<std::uint8_t>(std::clamp(alpha, 0.f, 1.f) * 255.f);
		return colour;
	}
}

namespace UI
{
	void MenuGrid::Update(float deltaTime)
	{
		scroll = Frac(scroll + ScrollSpeed * deltaTime);
	}

	void MenuGrid::Render(sf::RenderTarget& target) const
	{
		sf::VertexArray lines(sf::PrimitiveType::Lines);

		// Vertical lines: straight rays from the vanishing point out to the
		// bottom edge, fading in as they come forward.
		for (int i = 0; i < VerticalCount; ++i)
		{
			const float f = static_cast<float>(i) / static_cast<float>(VerticalCount - 1) * 2.f - 1.f;   // -1..1
			const sf::Vector2f top{ VanishX, HorizonY };
			const sf::Vector2f bottom{ VanishX + f * VerticalHalfSpread, BottomY };
			AppendLine(lines, top, bottom, WithAlpha(GridColour, 0.f), WithAlpha(GridColour, BaseAlpha));
		}

		// Horizontal rows, scrolling toward the viewer.
		for (int k = 0; k < RowCount; ++k)
		{
			const float u = Frac(static_cast<float>(k) / static_cast<float>(RowCount) + scroll);
			const float y = RowScreenY(u);
			const float depth = (y - HorizonY) / (BottomY - HorizonY);   // 0 at horizon, 1 at bottom

			const float fade = SmoothStep(0.f, 0.10f, u) * (1.f - SmoothStep(0.86f, 1.f, u));
			if (fade <= 0.f)
			{
				continue;
			}

			const float halfWidth = VerticalHalfSpread * depth;
			const sf::Color colour = WithAlpha(GridColour, BaseAlpha * fade);
			AppendLine(lines, { VanishX - halfWidth, y }, { VanishX + halfWidth, y }, colour, colour);
		}

		// The horizon line itself, a touch brighter and cyan.
		AppendLine(lines, { VanishX - VerticalHalfSpread * 0.06f, HorizonY }, { VanishX + VerticalHalfSpread * 0.06f, HorizonY },
			WithAlpha(HorizonColour, BaseAlpha * 0.9f), WithAlpha(HorizonColour, BaseAlpha * 0.9f));

		target.draw(lines);
	}
}
