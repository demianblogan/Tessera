#pragma once

#include <algorithm>
#include <cstdint>

#include <SFML/Graphics/Color.hpp>

// Small color maths shared by the animated menu pieces (title, ring). Alpha is
// left untouched unless a function name says otherwise.
namespace UI
{
	[[nodiscard]] inline std::uint8_t ToByte(float value) noexcept
	{
		return static_cast<std::uint8_t>(std::clamp(value, 0.f, 255.f));
	}

	[[nodiscard]] inline sf::Color Darken(sf::Color color, float factor) noexcept
	{
		return { ToByte(color.r * factor), ToByte(color.g * factor), ToByte(color.b * factor), color.a };
	}

	// factor may exceed 1 (SFML clamps on draw); alpha is kept.
	[[nodiscard]] inline sf::Color ScaleRgb(sf::Color color, float factor) noexcept
	{
		return { ToByte(color.r * factor), ToByte(color.g * factor), ToByte(color.b * factor), color.a };
	}

	[[nodiscard]] inline sf::Color MixToWhite(sf::Color color, float fraction) noexcept
	{
		return {
			ToByte(color.r + (255.f - color.r) * fraction),
			ToByte(color.g + (255.f - color.g) * fraction),
			ToByte(color.b + (255.f - color.b) * fraction),
			color.a };
	}

	// Standard (Rec. 601) luma weights, for a perceptual grey.
	inline constexpr float LumaWeightR = 0.30f;
	inline constexpr float LumaWeightG = 0.59f;
	inline constexpr float LumaWeightB = 0.11f;

	[[nodiscard]] inline sf::Color Desaturate(sf::Color color, float amount) noexcept
	{
		const float grey = LumaWeightR * color.r + LumaWeightG * color.g + LumaWeightB * color.b;
		return {
			ToByte(color.r + (grey - color.r) * amount),
			ToByte(color.g + (grey - color.g) * amount),
			ToByte(color.b + (grey - color.b) * amount),
			color.a };
	}
}
