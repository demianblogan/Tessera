#include "MenuButtonColumn.h"

#include <algorithm>
#include <cmath>
#include <utility>

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

#include "ColourUtils.h"
#include "TetrominoPalette.h"

namespace
{
	// Fly-in: buttons start here (bottom centre, just off screen) and curve up
	// to their resting left-column slots, one after another.
	constexpr sf::Vector2f SpawnPoint{ 960.f, 1120.f };
	constexpr float FlyDuration = 0.42f;
	constexpr float FlyStagger = 0.07f;
	constexpr float FlyStartScale = 0.82f;

	constexpr float ExitDuration = 0.26f;
	constexpr float ExitDropY = 1180.f;

	constexpr float Pi = 3.14159265f;
	constexpr float PressDuration = 0.18f;
	constexpr float PressPunch = 0.12f;
	constexpr float PressFlash = 0.55f;

	constexpr float SelectedScale = 1.05f;
	constexpr float GlowIntensity = 0.55f;
	constexpr float GlowBreathSpeed = 2.0f;

	// SetCompact() target look for the non-active buttons.
	constexpr float CompactScale = 0.72f;
	constexpr float CompactAlpha = 0.32f;
	constexpr float CompactSpeed = 5.f;   // 1 / seconds to reach the compact state

	constexpr float IdleDim = 0.6f;   // an unselected enabled button, vs the selected one

	[[nodiscard]] float EaseOutCubic(float t) noexcept
	{
		const float inv = 1.f - std::clamp(t, 0.f, 1.f);
		return 1.f - inv * inv * inv;
	}

	[[nodiscard]] float EaseInCubic(float t) noexcept
	{
		t = std::clamp(t, 0.f, 1.f);
		return t * t * t;
	}

	[[nodiscard]] float Lerp(float a, float b, float t) noexcept
	{
		return a + (b - a) * t;
	}

	[[nodiscard]] sf::Vector2f Lerp(sf::Vector2f a, sf::Vector2f b, float t) noexcept
	{
		return { Lerp(a.x, b.x, t), Lerp(a.y, b.y, t) };
	}

	[[nodiscard]] sf::Vector2f QuadBezier(sf::Vector2f a, sf::Vector2f c, sf::Vector2f b, float t) noexcept
	{
		const float inv = 1.f - t;
		return inv * inv * a + 2.f * inv * t * c + t * t * b;
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

	void MenuButtonColumn::AddButton(const sf::String& text, std::function<void()> onActivate, bool enabled,
		std::optional<sf::Color> colour)
	{
		MenuLabel label(font, characterSize);
		label.SetText(text);
		buttons.push_back(Button{
			std::move(label), std::move(onActivate), enabled, colour.value_or(sf::Color::White), {} });
	}

	void MenuButtonColumn::SetLayout(sf::Vector2f newTopLeft, float newRowGap)
	{
		topLeft = newTopLeft;
		rowGap = newRowGap;
	}

	void MenuButtonColumn::SetSelectionChangedCallback(std::function<void(std::size_t)> callback)
	{
		onSelectionChanged = std::move(callback);
	}

	void MenuButtonColumn::SetSwooshCallback(std::function<void(std::size_t)> callback)
	{
		onSwoosh = std::move(callback);
	}

	bool MenuButtonColumn::AnyEnabled() const
	{
		return std::any_of(buttons.begin(), buttons.end(), [](const Button& b) { return b.enabled; });
	}

	void MenuButtonColumn::Begin()
	{
		started = true;
		introTime = 0.f;
		exitTime = -1.f;
		swooshFired.assign(buttons.size(), 0);

		// Resting slot: left edge at topLeft.x, so the draw centre is offset by
		// half the (unscaled) text width.
		sf::Vector2f maxGlowBox{ 0.f, 0.f };
		for (std::size_t i = 0; i < buttons.size(); ++i)
		{
			buttons[i].restCentre = {
				topLeft.x + buttons[i].label.InkSize().x * 0.5f,
				topLeft.y + static_cast<float>(i) * rowGap };
			maxGlowBox.x = std::max(maxGlowBox.x, buttons[i].label.GlowBox().x);
			maxGlowBox.y = std::max(maxGlowBox.y, buttons[i].label.GlowBox().y);
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
			if (buttons[i].enabled)
			{
				selectedIndex = i;
				break;
			}
		}
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
		if (!started)
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
		if (!AnyEnabled() || buttons.empty())
		{
			return;
		}

		const int count = static_cast<int>(buttons.size());
		int index = static_cast<int>(selectedIndex);
		for (int step = 0; step < count; ++step)
		{
			index = (index + direction + count) % count;
			if (buttons[static_cast<std::size_t>(index)].enabled)
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
				onSelectionChanged(selectedIndex);
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
		if (button.enabled && button.activate)
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
			if (buttons[i].enabled && buttons[i].label.Bounds(buttons[i].restCentre, 1.f).contains(point))
			{
				if (i != selectedIndex)
				{
					selectedIndex = i;
					if (onSelectionChanged)
					{
						onSelectionChanged(selectedIndex);
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
			if (buttons[i].enabled && buttons[i].label.Bounds(buttons[i].restCentre, 1.f).contains(point))
			{
				selectedIndex = i;
				Activate();
				return PointerHit::Activated;
			}
		}

		return PointerHit::None;
	}

	void MenuButtonColumn::SetCompact(bool nowCompact, std::size_t activeIndex)
	{
		compact = nowCompact;
		// Keep the active index while un-compacting, so the closing category
		// stays full-size for the whole transition instead of another button
		// jumping to full when the index is reset.
		if (nowCompact)
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
			buttons[i].label.SetWaveEnabled(settled && buttons[i].enabled && i == selectedIndex);
		}

		if (started && introTime < 1e6f)
		{
			introTime += deltaTime;
		}
		if (exitTime >= 0.f)
		{
			exitTime += deltaTime;
		}

		// Fire the swoosh as each button launches into the fly-in.
		if (started && !IsIntroDone() && onSwoosh)
		{
			for (std::size_t i = 0; i < buttons.size() && i < swooshFired.size(); ++i)
			{
				if (!swooshFired[i]
					&& (introTime - static_cast<float>(i) * FlyStagger) / FlyDuration >= 0.08f)
				{
					swooshFired[i] = 1;
					onSwoosh(i);
				}
			}
		}

		const float target = compact ? 1.f : 0.f;
		compactT = std::clamp(compactT + (target - compactT) * std::min(1.f, deltaTime * CompactSpeed), 0.f, 1.f);
	}

	MenuButtonColumn::Pose MenuButtonColumn::PoseOf(std::size_t index) const
	{
		const Button& button = buttons[index];
		Pose pose{ button.restCentre, 1.f, 1.f };

		if (exitTime >= 0.f)
		{
			const float e = EaseInCubic(exitTime / ExitDuration);
			pose.centre = Lerp(button.restCentre, { button.restCentre.x, ExitDropY }, e);
			pose.alpha = 1.f - std::clamp(exitTime / ExitDuration, 0.f, 1.f);
			return pose;
		}

		if (started && !IsIntroDone())
		{
			const float local = std::clamp(
				(introTime - static_cast<float>(index) * FlyStagger) / FlyDuration, 0.f, 1.f);
			const float e = EaseOutCubic(local);
			const sf::Vector2f control{ Lerp(SpawnPoint.x, button.restCentre.x, 0.25f), button.restCentre.y + 100.f };
			pose.centre = QuadBezier(SpawnPoint, control, button.restCentre, e);
			pose.alpha = std::clamp(local * 1.8f, 0.f, 1.f);
			pose.scale = Lerp(FlyStartScale, 1.f, e);
			return pose;
		}

		// Settled. The selected button sits a touch larger; when a category is
		// open every other button shrinks and dims.
		if (index == selectedIndex)
		{
			pose.scale = SelectedScale;
		}

		if (compactT > 0.f && index != compactActive)
		{
			pose.scale *= Lerp(1.f, CompactScale, compactT);
			pose.alpha *= Lerp(1.f, CompactAlpha, compactT);
		}

		return pose;
	}

	void MenuButtonColumn::Render(sf::RenderTarget& target) const
	{
		if (!started)
		{
			return;
		}

		const bool settled = IsIntroDone() && exitTime < 0.f;

		const float press = pressTime < PressDuration
			? std::sin((1.f - pressTime / PressDuration) * Pi)
			: 0.f;
		const float breath = 0.85f + 0.15f * std::sin(animTime * GlowBreathSpeed);

		for (std::size_t i = 0; i < buttons.size(); ++i)
		{
			const Button& button = buttons[i];
			const Pose pose = PoseOf(i);
			if (pose.alpha <= 0.f)
			{
				continue;
			}

			const bool isSelected = settled && button.enabled && i == selectedIndex;

			sf::Color colour = UI::DisabledEntryColour;
			if (button.enabled)
			{
				colour = isSelected ? button.colour : UI::ScaleRgb(button.colour, IdleDim);
			}

			float scale = pose.scale;
			float whiten = 0.f;
			if (isSelected)
			{
				scale *= 1.f + PressPunch * press;
				whiten = PressFlash * press;

				const sf::Color tint = UI::ScaleRgb(button.colour, GlowIntensity * breath * pose.alpha);
				button.label.DrawGlow(target, glow, pose.centre, scale, tint);
			}

			button.label.Draw(target, pose.centre, scale, colour, pose.alpha, whiten);
		}
	}
}
