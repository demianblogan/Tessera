#include "OptionRow.h"

#include <algorithm>
#include <cmath>

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>

namespace
{
	constexpr unsigned int LabelSize = 46;
	constexpr unsigned int ValueSize = 42;

	constexpr float ControlFraction = 0.46f;   // right part of the row for the control
	constexpr float ArrowScreenSize = 30.f;    // on-screen size of the carousel arrow sprite
	constexpr float CheckboxSize = 44.f;       // on-screen size of the toggle checkbox

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
	const sf::Color AccentColour{ 120, 210, 255 };
	const sf::Color ArrowLive{ 210, 216, 224 };   // slightly dimmed; hover brings it to full
	const sf::Color ArrowDead{ 96, 102, 110 };

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
}

namespace UI
{
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
			bar.setFillColor(WithAlpha(AccentColour, Alpha(panelAlpha, highlight)));
			target.draw(bar);
		}

		const sf::Color colour = LabelColour();
		labelText.setFillColor(WithAlpha(colour, Alpha(panelAlpha)));
		labelText.setPosition({ left.x + 26.f, VerticalCentre(labelText, left.y, height) });
		target.draw(labelText);

		RenderControl(target, panelAlpha);
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
		, arrowTexture(arrowTexture)
		, onChange(std::move(onChange))
		, valueText(fontRef, "", ValueSize)
	{
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

		const std::size_t next = direction < 0
			? (current == 0 ? 0 : current - 1)
			: std::min(current + 1, options.size() - 1);

		pressTime[direction < 0 ? 0 : 1] = 0.f;

		if (next != current)
		{
			current = next;
			if (onChange)
			{
				onChange(current);
			}
		}
	}

	void CarouselRow::UpdateControl(float deltaTime)
	{
		for (float& time : pressTime)
		{
			time += deltaTime;
		}
	}

	sf::Vector2f CarouselRow::ArrowCentre(int side) const
	{
		// Fixed columns: the arrows line up with a ToggleRow's Off / On options
		// (the left and right quarter of the control area).
		const sf::FloatRect area = ControlArea();
		const float midY = area.position.y + area.size.y * 0.5f;
		const float x = area.position.x + area.size.x * (side < 0 ? 0.25f : 0.75f);
		return { x, midY };
	}

	sf::FloatRect CarouselRow::ArrowBox(int side) const
	{
		const sf::Vector2f centre = ArrowCentre(side);
		const float half = ArrowScreenSize * 0.5f + 8.f;
		return { { centre.x - half, centre.y - half }, { 2.f * half, 2.f * half } };
	}

	bool CarouselRow::HandlePointer(sf::Vector2f point, bool clicked)
	{
		hoveredArrow = 0;
		if (!enabled || options.empty())
		{
			return false;
		}

		if (current > 0 && ArrowBox(-1).contains(point))
		{
			hoveredArrow = -1;
		}
		else if (current + 1 < options.size() && ArrowBox(1).contains(point))
		{
			hoveredArrow = 1;
		}

		if (clicked && hoveredArrow != 0)
		{
			Adjust(hoveredArrow);
		}
		return hoveredArrow != 0;
	}

	void CarouselRow::RenderControl(sf::RenderTarget& target, float panelAlpha) const
	{
		const sf::FloatRect area = ControlArea();
		const float midY = area.position.y + area.size.y * 0.5f;

		// -- Value --
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

		// -- Arrows --
		const float baseScale = ArrowScreenSize / static_cast<float>(std::max(1u, arrowTexture.getSize().y));

		const auto drawArrow = [&](int side, bool live)
		{
			const std::size_t index = side < 0 ? 0u : 1u;
			const float press = std::clamp(1.f - pressTime[index] / ArrowPressDuration, 0.f, 1.f);
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

			sf::Sprite arrow(arrowTexture);
			arrow.setOrigin(sf::Vector2f(arrowTexture.getSize()) * 0.5f);
			const float scale = baseScale * (1.f - ArrowPressDip * press) * (hovered ? 1.18f : 1.f);
			// Texture points right; the left arrow is mirrored.
			arrow.setScale({ side < 0 ? -scale : scale, scale });
			arrow.setPosition(drawCentre);

			sf::Color base = live ? ArrowLive : ArrowDead;
			if (hovered)
			{
				base = sf::Color(255, 255, 255);   // full-bright on hover
			}
			const sf::Color tinted{
				static_cast<std::uint8_t>(base.r + (ArrowPressTint.r - base.r) * press),
				static_cast<std::uint8_t>(base.g + (ArrowPressTint.g - base.g) * press),
				static_cast<std::uint8_t>(base.b + (ArrowPressTint.b - base.b) * press) };
			arrow.setColor(WithAlpha(tinted, Alpha(panelAlpha)));
			target.draw(arrow);
		};

		drawArrow(-1, enabled && current > 0);
		drawArrow(1, enabled && !options.empty() && current + 1 < options.size());
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
		const float scale = CheckboxSize / 26.f * (live ? 1.1f : 1.f);

		sf::Sprite box(checkboxTexture);
		box.setTextureRect(on ? sf::IntRect{ { 0, 0 }, { 26, 26 } } : sf::IntRect{ { 28, 0 }, { 26, 26 } });
		box.setOrigin({ 13.f, 13.f });
		box.setScale({ scale, scale });
		box.setPosition(centre);
		box.setColor(WithAlpha(enabled ? (live ? sf::Color::White : sf::Color(210, 216, 224))
			: sf::Color(120, 124, 132), Alpha(panelAlpha)));
		target.draw(box);
	}
}
