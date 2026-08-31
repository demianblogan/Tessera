#include "OptionRow.h"

#include <algorithm>
#include <cmath>
#include <string>

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/Angle.hpp>

namespace
{
	constexpr unsigned int LabelSize = 46;
	constexpr unsigned int ValueSize = 42;

	constexpr float ControlFraction = 0.46f;
	constexpr float ArrowScreenSize = 30.f;
	constexpr float CheckboxSize = 46.f;

	constexpr sf::IntRect CheckboxOn{ { 0, 0 }, { 28, 27 } };
	constexpr sf::IntRect CheckboxOff{ { 28, 0 }, { 28, 27 } };

	// Arrow press feedback, matching the main-menu ring arrows.
	constexpr float ArrowPressDuration = 0.22f;
	constexpr float ArrowPressDip = 0.24f;
	constexpr float ArrowPressShift = 7.f;
	constexpr sf::Color ArrowPressTint{ 255, 155, 70 };
	constexpr sf::Color ArrowPulseColour{ 255, 140, 45 };

	const sf::Color SelectedLabel{ 255, 255, 255 };
	const sf::Color IdleLabel{ 210, 216, 226 };
	const sf::Color DisabledLabel{ 120, 124, 132 };
	const sf::Color ValueColour{ 236, 242, 250 };
	const sf::Color ArrowLive{ 210, 216, 224 };
	const sf::Color ArrowDead{ 96, 102, 110 };
	const sf::Color TickColour{ 110, 235, 145 };
	const sf::Color CrossColour{ 235, 120, 120 };

	[[nodiscard]] float VerticalCentre(const sf::Text& text, float rowTop, float rowHeight)
	{
		const sf::FloatRect bounds = text.getLocalBounds();
		return rowTop + rowHeight * 0.5f - (bounds.position.y + bounds.size.y * 0.5f);
	}

	[[nodiscard]] float EaseOutCubic(float t) noexcept
	{
		const float inv = 1.f - std::clamp(t, 0.f, 1.f);
		return 1.f - inv * inv * inv;
	}

	[[nodiscard]] sf::Color WithAlpha(sf::Color colour, std::uint8_t a) noexcept
	{
		return { colour.r, colour.g, colour.b, a };
	}

	void RoundedLine(sf::RenderTarget& target, sf::Vector2f a, sf::Vector2f b, float thickness, sf::Color colour)
	{
		const sf::Vector2f delta = b - a;
		const float length = std::sqrt(delta.x * delta.x + delta.y * delta.y);

		sf::RectangleShape segment({ length, thickness });
		segment.setOrigin({ 0.f, thickness * 0.5f });
		segment.setPosition(a);
		segment.setRotation(sf::radians(std::atan2(delta.y, delta.x)));
		segment.setFillColor(colour);
		target.draw(segment);

		sf::CircleShape cap(thickness * 0.5f);
		cap.setOrigin({ thickness * 0.5f, thickness * 0.5f });
		cap.setFillColor(colour);
		cap.setPosition(a);
		target.draw(cap);
		cap.setPosition(b);
		target.draw(cap);
	}

	void CentredBar(sf::RenderTarget& target, sf::Vector2f centre, float length, float thickness,
		float degrees, sf::Color colour)
	{
		sf::RectangleShape bar({ length, thickness });
		bar.setOrigin(bar.getSize() * 0.5f);
		bar.setPosition(centre);
		bar.setRotation(sf::degrees(degrees));
		bar.setFillColor(colour);
		target.draw(bar);
	}
}

namespace UI
{
	// =====================================================================
	// OptionRow
	// =====================================================================

	OptionRow::OptionRow(const sf::Font& fontRef, const sf::String& label)
		: font(fontRef)
		, labelText(fontRef, label, LabelSize)
	{
	}

	void OptionRow::SetLayout(sf::Vector2f newLeft, float newWidth, float newHeight)
	{
		left = newLeft;
		width = newWidth;
		height = newHeight;
	}

	sf::FloatRect OptionRow::Bounds() const
	{
		return { left, { width, height } };
	}

	sf::FloatRect OptionRow::ControlArea() const
	{
		const float controlWidth = width * ControlFraction;
		return { { left.x + width - controlWidth, left.y }, { controlWidth, height } };
	}

	std::uint8_t OptionRow::Alpha(float panelAlpha, float extra) const
	{
		return static_cast<std::uint8_t>(std::clamp(panelAlpha * extra, 0.f, 1.f) * 255.f);
	}

	sf::Color OptionRow::LabelColour() const
	{
		if (!enabled)
		{
			return DisabledLabel;
		}
		const float t = highlight;
		return {
			static_cast<std::uint8_t>(IdleLabel.r + (SelectedLabel.r - IdleLabel.r) * t),
			static_cast<std::uint8_t>(IdleLabel.g + (SelectedLabel.g - IdleLabel.g) * t),
			static_cast<std::uint8_t>(IdleLabel.b + (SelectedLabel.b - IdleLabel.b) * t) };
	}

	void OptionRow::Update(float deltaTime)
	{
		const float target = (selected && enabled) ? 1.f : 0.f;
		highlight += (target - highlight) * std::min(1.f, deltaTime * 12.f);

		for (float& time : arrowPress)
		{
			time += deltaTime;
		}

		UpdateControl(deltaTime);
	}

	void OptionRow::Render(sf::RenderTarget& target, float panelAlpha) const
	{
		if (panelAlpha <= 0.01f)
		{
			return;
		}

		if (highlight > 0.01f)
		{
			sf::RectangleShape fill({ width, height });
			fill.setPosition(left);
			fill.setFillColor(sf::Color(255, 255, 255, Alpha(panelAlpha, 0.10f * highlight)));
			target.draw(fill);

			sf::RectangleShape bar({ 5.f, height });
			bar.setPosition(left);
			bar.setFillColor(WithAlpha(accent, Alpha(panelAlpha, highlight)));
			target.draw(bar);
		}

		const sf::Color colour = LabelColour();
		labelText.setFillColor(WithAlpha(colour, Alpha(panelAlpha)));
		labelText.setPosition({ left.x + 26.f, VerticalCentre(labelText, left.y, height) });
		target.draw(labelText);

		RenderControl(target, panelAlpha);
	}

	// --- arrows ---

	sf::Vector2f OptionRow::ArrowCentre(int side) const
	{
		const sf::FloatRect area = ControlArea();
		const float midY = area.position.y + area.size.y * 0.5f;
		const float x = area.position.x + area.size.x * (side < 0 ? 0.12f : 0.88f);
		return { x, midY };
	}

	sf::FloatRect OptionRow::ArrowBox(int side) const
	{
		const sf::Vector2f centre = ArrowCentre(side);
		const float half = ArrowScreenSize * 0.5f + 10.f;
		return { { centre.x - half, centre.y - half }, { 2.f * half, 2.f * half } };
	}

	void OptionRow::PressArrow(int side)
	{
		arrowPress[side < 0 ? 0 : 1] = 0.f;
	}

	int OptionRow::PickArrow(sf::Vector2f point, bool leftLive, bool rightLive)
	{
		hoveredArrow = 0;
		if (leftLive && ArrowBox(-1).contains(point))
		{
			hoveredArrow = -1;
		}
		else if (rightLive && ArrowBox(1).contains(point))
		{
			hoveredArrow = 1;
		}
		return hoveredArrow;
	}

	void OptionRow::DrawArrows(sf::RenderTarget& target, float panelAlpha, bool leftLive, bool rightLive) const
	{
		if (arrowTexture == nullptr)
		{
			return;
		}

		const float baseScale = ArrowScreenSize / static_cast<float>(std::max(1u, arrowTexture->getSize().y));

		const auto drawArrow = [&](int side, bool live)
		{
			const std::size_t index = side < 0 ? 0u : 1u;
			const float press = std::clamp(1.f - arrowPress[index] / ArrowPressDuration, 0.f, 1.f);
			const bool hovered = live && hoveredArrow == side;

			const sf::Vector2f centre = ArrowCentre(side);
			const sf::Vector2f drawCentre{ centre.x - static_cast<float>(side) * ArrowPressShift * press, centre.y };

			if (press > 0.f && live)
			{
				const float t = EaseOutCubic(1.f - press);
				const float radius = 8.f + t * 26.f;
				sf::CircleShape ring(radius);
				ring.setOrigin({ radius, radius });
				ring.setPosition(centre);
				ring.setFillColor(sf::Color::Transparent);
				ring.setOutlineThickness(4.f);
				ring.setOutlineColor(WithAlpha(ArrowPulseColour,
					static_cast<std::uint8_t>(press * press * 120.f * std::clamp(panelAlpha, 0.f, 1.f))));
				target.draw(ring, sf::RenderStates(sf::BlendAdd));
			}

			sf::Sprite arrow(*arrowTexture);
			arrow.setOrigin(sf::Vector2f(arrowTexture->getSize()) * 0.5f);
			const float scale = baseScale * (1.f - ArrowPressDip * press) * (hovered ? 1.18f : 1.f);
			arrow.setScale({ side < 0 ? -scale : scale, scale });   // texture points right
			arrow.setPosition(drawCentre);

			sf::Color base = live ? ArrowLive : ArrowDead;
			if (hovered)
			{
				base = sf::Color(255, 255, 255);
			}
			const sf::Color tinted{
				static_cast<std::uint8_t>(base.r + (ArrowPressTint.r - base.r) * press),
				static_cast<std::uint8_t>(base.g + (ArrowPressTint.g - base.g) * press),
				static_cast<std::uint8_t>(base.b + (ArrowPressTint.b - base.b) * press) };
			arrow.setColor(WithAlpha(tinted, Alpha(panelAlpha)));
			target.draw(arrow);
		};

		drawArrow(-1, leftLive);
		drawArrow(1, rightLive);
	}

	// =====================================================================
	// CarouselRow
	// =====================================================================

	CarouselRow::CarouselRow(const sf::Font& fontRef, const sf::String& label,
		std::vector<sf::String> options, std::size_t current, const sf::Texture& arrowTexture,
		std::function<void(std::size_t)> onChange)
		: OptionRow(fontRef, label)
		, options(std::move(options))
		, current(this->options.empty() ? 0 : std::min(current, this->options.size() - 1))
		, onChange(std::move(onChange))
		, valueText(fontRef, "", ValueSize)
	{
		UseArrows(arrowTexture);
	}

	void CarouselRow::SetCurrent(std::size_t index)
	{
		if (!options.empty())
		{
			current = std::min(index, options.size() - 1);
		}
	}

	void CarouselRow::Adjust(int direction)
	{
		if (!enabled || options.empty())
		{
			return;
		}

		PressArrow(direction);

		const std::size_t next = direction < 0
			? (current == 0 ? 0 : current - 1)
			: std::min(current + 1, options.size() - 1);

		if (next != current)
		{
			current = next;
			if (onChange)
			{
				onChange(current);
			}
		}
	}

	bool CarouselRow::HandlePointer(sf::Vector2f point, bool clicked)
	{
		if (!enabled || options.empty())
		{
			return false;
		}

		const int arrow = PickArrow(point, current > 0, current + 1 < options.size());
		if (clicked && arrow != 0)
		{
			Adjust(arrow);
		}
		return arrow != 0;
	}

	void CarouselRow::RenderControl(sf::RenderTarget& target, float panelAlpha) const
	{
		const sf::FloatRect area = ControlArea();
		const float midY = area.position.y + area.size.y * 0.5f;

		if (!options.empty())
		{
			valueText.setString(options[current]);
			const sf::FloatRect bounds = valueText.getLocalBounds();
			valueText.setOrigin({ bounds.position.x + bounds.size.x * 0.5f, bounds.position.y + bounds.size.y * 0.5f });
			valueText.setPosition({ area.position.x + area.size.x * 0.5f, midY });
			const sf::Color c = enabled ? ValueColour : DisabledLabel;
			valueText.setFillColor(WithAlpha(c, Alpha(panelAlpha)));
			target.draw(valueText);
		}

		DrawArrows(target, panelAlpha, enabled && current > 0,
			enabled && !options.empty() && current + 1 < options.size());
	}

	// =====================================================================
	// SliderRow
	// =====================================================================

	SliderRow::SliderRow(const sf::Font& fontRef, const sf::String& label, const sf::Texture& arrowTexture,
		int steps, int current, std::function<void(int)> onChange)
		: OptionRow(fontRef, label)
		, steps(std::max(1, steps))
		, current(std::clamp(current, 0, std::max(1, steps)))
		, onChange(std::move(onChange))
		, percentText(fontRef, "", 42u)
	{
		UseArrows(arrowTexture);
	}

	void SliderRow::SetCurrent(int value)
	{
		current = std::clamp(value, 0, steps);
	}

	void SliderRow::Set(int value)
	{
		const int clamped = std::clamp(value, 0, steps);
		if (clamped != current)
		{
			current = clamped;
			if (onChange)
			{
				onChange(current);
			}
		}
	}

	void SliderRow::Adjust(int direction)
	{
		if (!enabled)
		{
			return;
		}
		PressArrow(direction);
		Set(current + direction);
	}

	sf::FloatRect SliderRow::BarRect() const
	{
		const sf::FloatRect area = ControlArea();
		const float inset = area.size.x * 0.20f;   // clear of the arrows
		const float barHeight = 36.f;
		return { { area.position.x + inset, area.position.y + area.size.y * 0.5f - barHeight * 0.5f },
			{ area.size.x - 2.f * inset, barHeight } };
	}

	bool SliderRow::HandlePointer(sf::Vector2f point, bool clicked)
	{
		if (!enabled)
		{
			return false;
		}

		const int arrow = PickArrow(point, current > 0, current < steps);
		if (arrow != 0)
		{
			if (clicked)
			{
				Adjust(arrow);
			}
			return true;
		}

		// Click on the bar jumps to the nearest step.
		const sf::FloatRect bar = BarRect();
		const sf::FloatRect hit{ { bar.position.x, bar.position.y - 16.f }, { bar.size.x, bar.size.y + 32.f } };
		if (hit.contains(point))
		{
			if (clicked && bar.size.x > 0.f)
			{
				const float fraction = std::clamp((point.x - bar.position.x) / bar.size.x, 0.f, 1.f);
				Set(static_cast<int>(std::lround(fraction * static_cast<float>(steps))));
			}
			return true;
		}
		return false;
	}

	void SliderRow::RenderControl(sf::RenderTarget& target, float panelAlpha) const
	{
		const sf::FloatRect bar = BarRect();
		const float fraction = static_cast<float>(current) / static_cast<float>(steps);

		const std::uint8_t a = Alpha(panelAlpha);
		const sf::Color track = enabled ? sf::Color(48, 54, 64) : sf::Color(40, 44, 50);
		const sf::Color fillColour = enabled ? accent : sf::Color(90, 100, 108);

		sf::RectangleShape trackShape(bar.size);
		trackShape.setPosition(bar.position);
		trackShape.setFillColor(WithAlpha(track, a));
		trackShape.setOutlineThickness(1.5f);
		trackShape.setOutlineColor(WithAlpha(sf::Color(90, 98, 110), a));
		target.draw(trackShape);

		if (fraction > 0.f)
		{
			sf::RectangleShape fillShape({ bar.size.x * fraction, bar.size.y });
			fillShape.setPosition(bar.position);
			fillShape.setFillColor(WithAlpha(fillColour, a));
			target.draw(fillShape);
		}

		percentText.setString(std::to_string(current * (100 / steps)) + "%");
		const sf::FloatRect bounds = percentText.getLocalBounds();
		percentText.setOrigin({ bounds.position.x + bounds.size.x * 0.5f, bounds.position.y + bounds.size.y * 0.5f });
		percentText.setPosition({ bar.position.x + bar.size.x * 0.5f, bar.position.y + bar.size.y * 0.5f });
		percentText.setFillColor(WithAlpha(enabled ? sf::Color::White : DisabledLabel, a));
		percentText.setOutlineThickness(2.f);
		percentText.setOutlineColor(WithAlpha(sf::Color(10, 14, 20), a));
		target.draw(percentText);

		DrawArrows(target, panelAlpha, enabled && current > 0, enabled && current < steps);
	}

	// =====================================================================
	// ToggleRow
	// =====================================================================

	ToggleRow::ToggleRow(const sf::Font& fontRef, const sf::String& label,
		const sf::Texture& checkboxTexture, bool on, std::function<void(bool)> onChange)
		: OptionRow(fontRef, label)
		, on(on)
		, checkboxTexture(checkboxTexture)
		, onChange(std::move(onChange))
	{
	}

	void ToggleRow::SetOn(bool value)
	{
		on = value;
	}

	void ToggleRow::Set(bool value)
	{
		if (value != on)
		{
			on = value;
			if (onChange)
			{
				onChange(on);
			}
		}
	}

	void ToggleRow::Adjust(int direction)
	{
		if (enabled)
		{
			Set(direction > 0);
		}
	}

	void ToggleRow::Activate()
	{
		if (enabled)
		{
			Set(!on);
		}
	}

	sf::FloatRect ToggleRow::CheckboxBounds() const
	{
		const sf::FloatRect area = ControlArea();
		const sf::Vector2f centre{ area.position.x + area.size.x * 0.5f, area.position.y + area.size.y * 0.5f };
		return { { centre.x - CheckboxSize * 0.7f, centre.y - CheckboxSize * 0.7f },
			{ CheckboxSize * 1.4f, CheckboxSize * 1.4f } };
	}

	bool ToggleRow::HandlePointer(sf::Vector2f point, bool clicked)
	{
		if (!enabled || !CheckboxBounds().contains(point))
		{
			return false;
		}

		if (clicked)
		{
			Set(!on);
		}
		return true;
	}

	void ToggleRow::RenderControl(sf::RenderTarget& target, float panelAlpha) const
	{
		const sf::FloatRect area = ControlArea();
		const sf::Vector2f centre{ area.position.x + area.size.x * 0.5f, area.position.y + area.size.y * 0.5f };

		const bool live = selected && enabled;
		const float scale = CheckboxSize / static_cast<float>(CheckboxOn.size.y) * (live ? 1.08f : 1.f);

		sf::Sprite box(checkboxTexture);
		box.setTextureRect(on ? CheckboxOn : CheckboxOff);
		box.setOrigin(sf::Vector2f(CheckboxOn.size) * 0.5f);
		box.setScale({ scale, scale });
		box.setPosition(centre);
		box.setColor(WithAlpha(enabled ? (live ? sf::Color::White : sf::Color(210, 216, 224))
			: sf::Color(120, 124, 132), Alpha(panelAlpha)));
		target.draw(box);

		const float s = CheckboxSize * (live ? 1.08f : 1.f);
		const float thickness = std::max(3.f, s * 0.13f);
		const sf::Color symbol = WithAlpha(
			enabled ? (on ? TickColour : CrossColour) : sf::Color(120, 124, 132), Alpha(panelAlpha));

		if (on)
		{
			const sf::Vector2f p0{ centre.x - s * 0.24f, centre.y + s * 0.02f };
			const sf::Vector2f p1{ centre.x - s * 0.06f, centre.y + s * 0.20f };
			const sf::Vector2f p2{ centre.x + s * 0.26f, centre.y - s * 0.20f };
			RoundedLine(target, p0, p1, thickness, symbol);
			RoundedLine(target, p1, p2, thickness, symbol);
		}
		else
		{
			CentredBar(target, centre, s * 0.62f, thickness, 45.f, symbol);
			CentredBar(target, centre, s * 0.62f, thickness, -45.f, symbol);
		}
	}
}
