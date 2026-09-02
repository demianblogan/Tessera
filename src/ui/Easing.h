#pragma once

#include <algorithm>
#include <cmath>

#include <SFML/System/Vector2.hpp>

// Small interpolation / easing maths shared by the animated UI pieces. `t` is
// clamped to [0, 1] by every easing curve here; Lerp is not clamped.
namespace UI::Easing
{
	[[nodiscard]] inline float Lerp(float a, float b, float t) noexcept
	{
		return a + (b - a) * t;
	}

	[[nodiscard]] inline sf::Vector2f Lerp(sf::Vector2f a, sf::Vector2f b, float t) noexcept
	{
		return { Lerp(a.x, b.x, t), Lerp(a.y, b.y, t) };
	}

	[[nodiscard]] inline float Clamp01(float t) noexcept
	{
		return std::clamp(t, 0.f, 1.f);
	}

	[[nodiscard]] inline float SmoothStep(float t) noexcept
	{
		t = Clamp01(t);
		return t * t * (3.f - 2.f * t);
	}

	[[nodiscard]] inline float EaseInCubic(float t) noexcept
	{
		t = Clamp01(t);
		return t * t * t;
	}

	[[nodiscard]] inline float EaseOutCubic(float t) noexcept
	{
		const float inv = 1.f - Clamp01(t);
		return 1.f - inv * inv * inv;
	}

	[[nodiscard]] inline float EaseOutQuad(float t) noexcept
	{
		const float inv = 1.f - Clamp01(t);
		return 1.f - inv * inv;
	}

	[[nodiscard]] inline float EaseOutBack(float t) noexcept
	{
		t = Clamp01(t);
		constexpr float c = 1.70158f;
		const float inv = t - 1.f;
		return 1.f + (c + 1.f) * inv * inv * inv + c * inv * inv;
	}

	[[nodiscard]] inline float EaseOutElastic(float t) noexcept
	{
		if (t <= 0.f) { return 0.f; }
		if (t >= 1.f) { return 1.f; }

		constexpr float period = 2.f * 3.14159265f / 3.f;
		return std::pow(2.f, -10.f * t) * std::sin((t * 10.f - 0.75f) * period) + 1.f;
	}
}
