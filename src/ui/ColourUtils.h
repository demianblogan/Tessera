#pragma once

#include <algorithm>
#include <cstdint>

#include <SFML/Graphics/Color.hpp>

// Small colour maths shared by the animated menu pieces (title, ring). Alpha is
// left untouched unless a function name says otherwise.
namespace UI
{
	[[nodiscard]] inline std::uint8_t ToByte(float value) noexcept
	{
		return static_cast<std::uint8_t>(std::clamp(value, 0.f, 255.f));
	}

	[[nodiscard]] inline sf::Color Darken(sf::Color colour, float factor) noexcept
	{
		return { ToByte(colour.r * factor), ToByte(colour.g * factor), ToByte(colour.b * factor), colour.a };
	}

	// factor may exceed 1 (SFML clamps on draw); alpha is kept.
	[[nodiscard]] inline sf::Color ScaleRgb(sf::Color colour, float factor) noexcept
	{
		return { ToByte(colour.r * factor), ToByte(colour.g * factor), ToByte(colour.b * factor), colour.a };
	}

	[[nodiscard]] inline sf::Color MixToWhite(sf::Color colour, float t) noexcept
	{
		return {
			ToByte(colour.r + (255.f - colour.r) * t),
			ToByte(colour.g + (255.f - colour.g) * t),
			ToByte(colour.b + (255.f - colour.b) * t),
			colour.a };
	}

	[[nodiscard]] inline sf::Color Desaturate(sf::Color colour, float amount) noexcept
	{
		const float grey = 0.30f * colour.r + 0.59f * colour.g + 0.11f * colour.b;
		return {
			ToByte(colour.r + (grey - colour.r) * amount),
			ToByte(colour.g + (grey - colour.g) * amount),
			ToByte(colour.b + (grey - colour.b) * amount),
			colour.a };
	}
}
