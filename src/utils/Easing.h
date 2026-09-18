#pragma once

#include <algorithm>
#include <cmath>
#include <numbers>

#include <SFML/System/Vector2.hpp>

// Small interpolation / easing maths shared by every animated piece, both the
// menu chrome (ui/) and the gameplay presentation (rendering/). `t` is
// clamped to [0, 1] by every easing curve here; Lerp is not clamped.
//
// All of these answer the same question -- "given how far through an
// animation I am (t, 0..1), how far through the *value* should I be?" -- just
// with different curves. Lerp is the value itself (plug the curve's output
// back into Lerp to actually move/fade/scale something); the rest reshape t
// before it gets there, which is what makes a move feel snappy, floaty, or
// springy instead of perfectly linear (robotic, and the eye reads linear
// motion as slowing down near the end because it's judged relative to
// distance left, not time left).
namespace Easing
{
	// Linear interpolation: t=0 gives a, t=1 gives b, anywhere between is a
	// straight blend. Not clamped -- t outside [0, 1] overshoots past a/b,
	// which some callers below rely on (see EaseOutBack). This is the
	// workhorse: every other function here only computes a *replacement* t to
	// feed into this, whether the thing being blended is a color channel
	// (GameplayHUD::MixColor, BoardCallouts::Brighten), a position or size
	// (BoardRenderer's next-queue slide and hold-swap flight), or a UI scale
	// (LanguagePickerState's hover highlight).
	[[nodiscard]] constexpr float Lerp(float a, float b, float t) noexcept
	{
		return a + (b - a) * t;
	}

	// Lerp, component-wise, for on-screen positions (e.g. BoardRenderer's
	// piece-flight center, easing a piece from its old position to its new one).
	[[nodiscard]] constexpr sf::Vector2f Lerp(sf::Vector2f a, sf::Vector2f b, float t) noexcept
	{
		return { Lerp(a.x, b.x, t), Lerp(a.y, b.y, t) };
	}

	// Plain clamp to [0, 1]. Used both to sanitise a raw t before an easing
	// curve (every curve below calls this internally) and on its own wherever
	// a fraction just needs pinning in range without reshaping, e.g.
	// BoardCallouts fading a callout's alpha out over its last portion of life.
	[[nodiscard]] constexpr float Clamp01(float t) noexcept
	{
		return std::clamp(t, 0.f, 1.f);
	}

	// Slow -> fast -> slow (an S-curve): zero velocity at both t=0 and t=1, so
	// nothing starts or stops with a visible jolt. The general-purpose "make
	// this fade/slide feel gentle" curve -- ConfirmDialog and PauseMenuScreen's
	// appear/leave fades, CarouselMenu and OptionsScreen's cross-fades,
	// GameOverState's panel intro all use this when there's no reason to
	// favour a snappy start or end over the other.
	[[nodiscard]] constexpr float SmoothStep(float t) noexcept
	{
		t = Clamp01(t);
		return t * t * (3.f - 2.f * t);
	}

	// Slow start, accelerating hard into t=1 (t^3 -- t=0.5 is only an eighth
	// of the way there). Reads as something building up speed: MenuButtonColumn
	// uses it for a column's exit slide (drifting off, then whipping away),
	// LanguagePickerState for the falling intro letters (they start slow, as
	// if just released, and pick up speed like real gravity), ConfirmDialog
	// and PlayTransitionState similarly for elements leaving the screen.
	[[nodiscard]] constexpr float EaseInCubic(float t) noexcept
	{
		t = Clamp01(t);
		return t * t * t;
	}

	// The mirror of EaseInCubic: starts fast, decelerates hard into t=1 (a
	// "soft landing" -- most of the motion happens in the first third). This
	// is the single most-used curve in the project: GameplayHUD's stat-panel
	// flash/pulse decay, BoardRenderer's next-queue slide and hold-swap/hard-
	// drop flights, MenuButtonColumn's fly-in, LoadingState's progress-bar
	// fill, CarouselMenu's rotation settle, BoardCallouts' rise-and-flash --
	// anywhere something needs to arrive at its resting position/value and
	// visibly settle there rather than stopping abruptly.
	[[nodiscard]] constexpr float EaseOutCubic(float t) noexcept
	{
		const float inv = 1.f - Clamp01(t);
		return 1.f - inv * inv * inv;
	}

	// Same shape as EaseOutCubic (fast start, decelerating finish) but gentler
	// -- t^2 instead of t^3, so it settles less abruptly. DropInTitle uses this
	// for the title letters' idle wave motion, where EaseOutCubic's sharper
	// deceleration would look too mechanical for a continuous, breathing motion.
	[[nodiscard]] constexpr float EaseOutQuad(float t) noexcept
	{
		const float inv = 1.f - Clamp01(t);
		return 1.f - inv * inv;
	}

	// Overshoots past 1 partway through, then settles back down to exactly 1 --
	// a springy "pop" rather than a smooth arrival, like a UI element that's
	// slightly too enthusiastic about landing. GameOverState's button press
	// punch and BoardCallouts' text pop-in (a callout scales down from
	// oversized, overshoots slightly small, then springs back to full size)
	// both lean on this for that "landed with a bit of bounce" feel that a
	// pure ease-out can't produce.
	[[nodiscard]] constexpr float EaseOutBack(float t) noexcept
	{
		t = Clamp01(t);
		constexpr float c = 1.70158f;
		const float inv = t - 1.f;
		return 1.f + (c + 1.f) * inv * inv * inv + c * inv * inv;
	}

	// A full decaying spring: oscillates past 1 and back several times, each
	// swing smaller than the last, before settling. The showiest curve here --
	// DropInTitle uses it for a falling title letter's impact, so it visibly
	// wobbles on landing instead of just stopping, selling the weight of the
	// drop.
	//
	// Not constexpr, unlike every curve above: it calls std::pow/std::sin,
	// which the standard doesn't guarantee as constexpr (and MSVC's <cmath>
	// doesn't implement them as such) -- marking it constexpr here would claim
	// a guarantee the toolchain can't back up.
	[[nodiscard]] inline float EaseOutElastic(float t) noexcept
	{
		if (t <= 0.f)
		{
			return 0.f;
		}

		if (t >= 1.f)
		{
			return 1.f;
		}

		constexpr float period = 2.f * std::numbers::pi_v<float> / 3.f;
		return std::pow(2.f, -10.f * t) * std::sin((t * 10.f - 0.75f) * period) + 1.f;
	}
}
