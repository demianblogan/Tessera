#include "MenuButtonColumn.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <utility>

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

#include "ColorUtils.h"
#include "../utils/Easing.h"
#include "TetrominoPalette.h"

namespace
{
	// Fly-in: buttons start here (bottom center, just off screen) and curve up
	// to their resting left-column slots, one after another.
	constexpr sf::Vector2f SpawnPoint{ 960.f, 1120.f };
	constexpr float FlyDuration = 0.42f;
	constexpr float FlyStagger = 0.07f;
	constexpr float FlyStartScale = 0.82f;

	constexpr float ExitDuration = 0.26f;
	constexpr float ExitDropY = 1180.f;

	constexpr float Pi = std::numbers::pi_v<float>;
	constexpr float PressDuration = 0.18f;
	constexpr float PressPunch = 0.12f;
	constexpr float PressFlash = 0.55f;

	constexpr float SelectedScale = 1.05f;
	constexpr float GlowIntensity = 0.55f;
	constexpr float GlowBreathSpeed = 2.0f;
	constexpr float GlowBreathBase = 0.85f;
	constexpr float GlowBreathAmplitude = 0.15f;

	// GetPose() -- during the fly-in, alpha ramps to full faster than the
	// position eases in, so a button is already visible partway along its path.
	constexpr float IntroAlphaRampScale = 1.8f;

	// Update() -- a button's swoosh fires this far into its own staggered
	// fly-in window (as a fraction of FlyDuration).
	constexpr float SwooshTriggerFraction = 0.08f;

	// SetCompact() target look for the non-active buttons.
	constexpr float CompactScale = 0.72f;
	constexpr float CompactAlpha = 0.32f;
	constexpr float CompactSpeed = 5.f;   // 1 / seconds to reach the compact state

	constexpr float IdleDim = 0.6f;   // an unselected enabled button, vs the selected one

	// PoseOf() -- fly-in Bezier control point: pulled most of the way toward
	// the resting slot horizontally, and dropped a bit below it vertically, so
	// the path curves up and in rather than arriving in a straight line.
	constexpr float FlyControlBlend = 0.25f;
	constexpr float FlyControlYOffset = 100.f;

	// Sentinel meaning "no intro animation": AppearInstantly() jumps introTime
	// straight past the end so Update() never advances it further.
	constexpr float NoIntroSentinel = 1e7f;

	using Easing::EaseInCubic;
	using Easing::EaseOutCubic;
	using Easing::Lerp;

	[[nodiscard]] sf::Vector2f QuadBezier(sf::Vector2f start, sf::Vector2f control, sf::Vector2f end, float progress) noexcept
	{
		const float inv = 1.f - progress;
		return inv * inv * start + 2.f * inv * progress * control + progress * progress * end;
	}
}

namespace UI
{
	MenuButtonColumn::MenuButtonColumn(const sf::Font& font, unsigned int characterSize,
		sf::Shader& dilateShader, sf::Shader& blurShader)
		: glow(dilateShader, blurShader)
		, font(font)
		, characterSize(characterSize)
	{
	}

	void MenuButtonColumn::AddButton(const sf::String& text, std::function<void()> onActivate, bool isEnabled,
		std::optional<sf::Color> color)
	{
		MenuLabel label(font, characterSize);
		label.SetText(text);
		buttons.push_back(Button{
			std::move(label), std::move(onActivate), isEnabled, color.value_or(sf::Color::White), {} });
	}

	void MenuButtonColumn::SetButtonText(std::size_t index, const sf::String& text)
	{
		if (index < buttons.size())
		{
			buttons[index].label.SetText(text);

			// The resting slot's left edge is topLeft.x; its draw *center* is
			// offset by half the ink width (see Begin()), so a new string with a
			// different width needs that offset recomputed too, or every row
			// drifts sideways by a different amount once translated.
			buttons[index].restCenter.x = topLeft.x + buttons[index].label.GetInkSize().x * 0.5f;
		}
	}

	void MenuButtonColumn::SetLayout(sf::Vector2f newTopLeft, float newRowGap)
	{
		topLeft = newTopLeft;
		rowGap = newRowGap;
	}

	void MenuButtonColumn::SetSelectionChangedCallback(std::function<void(std::size_t, int)> callback)
	{
		onSelectionChanged = std::move(callback);
	}

	void MenuButtonColumn::SetSwooshCallback(std::function<void(std::size_t)> callback)
	{
		onSwoosh = std::move(callback);
	}

	bool MenuButtonColumn::IsAnyEnabled() const
	{
		return std::any_of(buttons.begin(), buttons.end(), [](const Button& button) { return button.isEnabled; });
	}

	void MenuButtonColumn::SetRenderShift(sf::Vector2f shift)
	{
		renderShift = shift;
	}

	void MenuButtonColumn::SetRenderDim(float dim)
	{
		renderDim = dim;
	}

	void MenuButtonColumn::SetSelectionHighlight(bool isSelectionHighlightEnabled)
	{
		this->isSelectionHighlightEnabled = isSelectionHighlightEnabled;
	}

	std::size_t MenuButtonColumn::GetSelectedIndex() const
	{
		return selectedIndex;
	}

	std::size_t MenuButtonColumn::GetButtonCount() const
	{
		return buttons.size();
	}

	void MenuButtonColumn::Begin()
	{
		hasStarted = true;
		introTime = 0.f;
		exitTime = -1.f;
		swooshFired.assign(buttons.size(), 0);

		// Resting slot: left edge at topLeft.x, so the draw center is offset by
		// half the (unscaled) text width.
		sf::Vector2f maxGlowBox{ 0.f, 0.f };
		for (std::size_t i = 0; i < buttons.size(); ++i)
		{
			buttons[i].restCenter = {
				topLeft.x + buttons[i].label.GetInkSize().x * 0.5f,
				topLeft.y + static_cast<float>(i) * rowGap };
			maxGlowBox.x = std::max(maxGlowBox.x, buttons[i].label.GetGlowBox().x);
			maxGlowBox.y = std::max(maxGlowBox.y, buttons[i].label.GetGlowBox().y);
		}

		// One glow box for the whole column, so NeonGlow never re-sizes when the
		// selection moves between buttons of different widths.
		for (Button& button : buttons)
		{
			button.label.SetGlowBoxSize(maxGlowBox);
		}

		// Start focused on the first enabled button.
		for (std::size_t i = 0; i < buttons.size(); ++i)
		{
			if (buttons[i].isEnabled)
			{
				selectedIndex = i;
				break;
			}
		}
	}

	void MenuButtonColumn::AppearInstantly()
	{
		Begin();
		introTime = NoIntroSentinel;   // past the end of the fly-in: settled, no animation
		std::fill(swooshFired.begin(), swooshFired.end(), static_cast<char>(1));
	}

	sf::Vector2f MenuButtonColumn::GetEntryCenter(std::size_t index) const
	{
		if (index >= buttons.size())
		{
			return {};
		}
		return buttons[index].restCenter + renderShift;
	}

	float MenuButtonColumn::GetEntryHeight(std::size_t index) const
	{
		if (index >= buttons.size())
		{
			return 0.f;
		}
		return buttons[index].label.GetInkSize().y;
	}

	void MenuButtonColumn::PlayExit()
	{
		if (exitTime < 0.f)
		{
			exitTime = 0.f;
		}
	}

	bool MenuButtonColumn::IsIntroDone() const
	{
		if (!hasStarted)
		{
			return false;
		}
		const float total = static_cast<float>(buttons.size()) * FlyStagger + FlyDuration;
		return introTime >= total;
	}

	bool MenuButtonColumn::IsExitDone() const
	{
		return exitTime >= ExitDuration;
	}

	void MenuButtonColumn::MoveSelection(int direction)
	{
		if (!IsAnyEnabled() || buttons.empty())
		{
			return;
		}

		const int count = static_cast<int>(buttons.size());
		int index = static_cast<int>(selectedIndex);
		for (int step = 0; step < count; ++step)
		{
			index = (index + direction + count) % count;
			if (buttons[static_cast<std::size_t>(index)].isEnabled)
			{
				break;
			}
		}

		const auto next = static_cast<std::size_t>(index);
		if (next != selectedIndex)
		{
			selectedIndex = next;
			if (onSelectionChanged)
			{
				onSelectionChanged(selectedIndex, direction);
			}
		}
	}

	void MenuButtonColumn::SelectPrevious() { MoveSelection(-1); }
	void MenuButtonColumn::SelectNext() { MoveSelection(1); }

	void MenuButtonColumn::Activate()
	{
		if (selectedIndex >= buttons.size())
		{
			return;
		}

		Button& button = buttons[selectedIndex];
		if (button.isEnabled && button.activate)
		{
			pressTime = 0.f;
			button.activate();
		}
	}

	void MenuButtonColumn::PointerMoved(sf::Vector2f point)
	{
		if (!IsIntroDone() || exitTime >= 0.f)
		{
			return;
		}

		for (std::size_t i = 0; i < buttons.size(); ++i)
		{
			if (buttons[i].isEnabled && buttons[i].label.GetBounds(buttons[i].restCenter + renderShift, 1.f).contains(point))
			{
				if (i != selectedIndex)
				{
					const int direction = i > selectedIndex ? 1 : -1;
					selectedIndex = i;
					if (onSelectionChanged)
					{
						onSelectionChanged(selectedIndex, direction);
					}
				}
				return;
			}
		}
	}

	MenuButtonColumn::PointerHit MenuButtonColumn::PointerPressed(sf::Vector2f point)
	{
		if (!IsIntroDone() || exitTime >= 0.f)
		{
			return PointerHit::None;
		}

		for (std::size_t i = 0; i < buttons.size(); ++i)
		{
			if (buttons[i].isEnabled && buttons[i].label.GetBounds(buttons[i].restCenter + renderShift, 1.f).contains(point))
			{
				selectedIndex = i;
				Activate();
				return PointerHit::Activated;
			}
		}

		return PointerHit::None;
	}

	void MenuButtonColumn::SetCompact(bool isCompact, std::size_t activeIndex)
	{
		this->isCompact = isCompact;
		// Keep the active index while un-compacting, so the closing category
		// stays full-size for the whole transition instead of another button
		// jumping to full when the index is reset.
		if (isCompact)
		{
			compactActive = activeIndex;
		}
	}

	void MenuButtonColumn::Update(float deltaTime)
	{
		animTime += deltaTime;
		pressTime += deltaTime;
		glow.Update(deltaTime);

		const bool settled = IsIntroDone() && exitTime < 0.f;
		for (std::size_t i = 0; i < buttons.size(); ++i)
		{
			buttons[i].label.Update(deltaTime);
			// Only the selected, settled button carries the idle wave.
			buttons[i].label.SetWaveEnabled(isSelectionHighlightEnabled && settled && buttons[i].isEnabled && i == selectedIndex);
		}

		if (hasStarted && introTime < NoIntroSentinel)
		{
			introTime += deltaTime;
		}
		if (exitTime >= 0.f)
		{
			exitTime += deltaTime;
		}

		// Fire the swoosh as each button launches into the fly-in.
		if (hasStarted && !IsIntroDone() && onSwoosh)
		{
			for (std::size_t i = 0; i < buttons.size() && i < swooshFired.size(); ++i)
			{
				if (!swooshFired[i]
					&& (introTime - static_cast<float>(i) * FlyStagger) / FlyDuration >= SwooshTriggerFraction)
				{
					swooshFired[i] = 1;
					onSwoosh(i);
				}
			}
		}

		const float target = isCompact ? 1.f : 0.f;
		compactFraction = std::clamp(
			compactFraction + (target - compactFraction) * std::min(1.f, deltaTime * CompactSpeed), 0.f, 1.f);
	}

	MenuButtonColumn::Pose MenuButtonColumn::GetPose(std::size_t index) const
	{
		const Button& button = buttons[index];
		Pose pose{ button.restCenter, 1.f, 1.f };

		if (exitTime >= 0.f)
		{
			const float exitEase = EaseInCubic(exitTime / ExitDuration);
			pose.center = Lerp(button.restCenter, { button.restCenter.x, ExitDropY }, exitEase);
			pose.alpha = 1.f - std::clamp(exitTime / ExitDuration, 0.f, 1.f);
			return pose;
		}

		if (hasStarted && !IsIntroDone())
		{
			const float introLocal = std::clamp(
				(introTime - static_cast<float>(index) * FlyStagger) / FlyDuration, 0.f, 1.f);
			const float introEase = EaseOutCubic(introLocal);
			const sf::Vector2f control{ Lerp(SpawnPoint.x, button.restCenter.x, FlyControlBlend), button.restCenter.y + FlyControlYOffset };
			pose.center = QuadBezier(SpawnPoint, control, button.restCenter, introEase);
			pose.alpha = std::clamp(introLocal * IntroAlphaRampScale, 0.f, 1.f);
			pose.scale = Lerp(FlyStartScale, 1.f, introEase);
			return pose;
		}

		// Settled. The selected button sits a touch larger; when a category is
		// open every other button shrinks and dims.
		if (index == selectedIndex)
		{
			pose.scale = SelectedScale;
		}

		if (compactFraction > 0.f && index != compactActive)
		{
			pose.scale *= Lerp(1.f, CompactScale, compactFraction);
			pose.alpha *= Lerp(1.f, CompactAlpha, compactFraction);
		}

		return pose;
	}

	void MenuButtonColumn::Render(sf::RenderTarget& target) const
	{
		if (!hasStarted)
		{
			return;
		}

		const bool settled = IsIntroDone() && exitTime < 0.f;

		const float press = pressTime < PressDuration
			? std::sin((1.f - pressTime / PressDuration) * Pi)
			: 0.f;
		const float breath = GlowBreathBase + GlowBreathAmplitude * std::sin(animTime * GlowBreathSpeed);

		for (std::size_t i = 0; i < buttons.size(); ++i)
		{
			const Button& button = buttons[i];
			Pose pose = GetPose(i);
			pose.center += renderShift;
			pose.alpha *= renderDim;
			if (pose.alpha <= 0.f)
			{
				continue;
			}

			const bool isSelected = isSelectionHighlightEnabled && settled && button.isEnabled && i == selectedIndex;

			sf::Color color = UI::DisabledEntryColor;
			if (button.isEnabled)
			{
				color = isSelected ? button.color : UI::ScaleRgb(button.color, IdleDim);
			}

			float scale = pose.scale;
			float whiten = 0.f;
			if (isSelected)
			{
				scale *= 1.f + PressPunch * press;
				whiten = PressFlash * press;

				const sf::Color tint = UI::ScaleRgb(button.color, GlowIntensity * breath * pose.alpha);
				button.label.DrawGlow(target, glow, pose.center, scale, tint);
			}

			button.label.Draw(target, pose.center, scale, color, pose.alpha, whiten);
		}
	}
}
