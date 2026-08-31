#include "OptionRow.h"

#include <algorithm>

#include <SFML/Graphics/ConvexShape.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

namespace
{
	constexpr unsigned int LabelSize = 30;
	constexpr unsigned int ValueSize = 28;

	constexpr float ControlFraction = 0.46f;   // right part of the row for the control
	constexpr float ArrowHalf = 11.f;
	constexpr float TogglePadding = 16.f;

	const sf::Color SelectedLabel{ 255, 255, 255 };
	const sf::Color IdleLabel{ 206, 212, 222 };
	const sf::Color DisabledLabel{ 120, 124, 132 };
	const sf::Color ValueColour{ 235, 240, 248 };
	const sf::Color AccentColour{ 120, 210, 255 };
	const sf::Color ArrowLive{ 235, 240, 248 };
	const sf::Color ArrowDead{ 96, 100, 108 };

	[[nodiscard]] float VerticalCentre(const sf::Text& text, float rowTop, float rowHeight)
	{
		const sf::FloatRect bounds = text.getLocalBounds();
		return rowTop + rowHeight * 0.5f - (bounds.position.y + bounds.size.y * 0.5f);
	}

	void DrawTriangle(sf::RenderTarget& target, sf::Vector2f centre, int side, sf::Color colour)
	{
		sf::ConvexShape triangle(3);
		const float dir = static_cast<float>(side);
		triangle.setPoint(0, { centre.x + dir * ArrowHalf, centre.y });
		triangle.setPoint(1, { centre.x - dir * ArrowHalf, centre.y - ArrowHalf });
		triangle.setPoint(2, { centre.x - dir * ArrowHalf, centre.y + ArrowHalf });
		triangle.setFillColor(colour);
		target.draw(triangle);
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
			bar.setFillColor(sf::Color(AccentColour.r, AccentColour.g, AccentColour.b, Alpha(panelAlpha, highlight)));
			target.draw(bar);
		}

		const sf::Color colour = LabelColour();
		labelText.setFillColor(sf::Color(colour.r, colour.g, colour.b, Alpha(panelAlpha)));
		labelText.setPosition({ left.x + 24.f, VerticalCentre(labelText, left.y, height) });
		target.draw(labelText);

		RenderControl(target, panelAlpha);
	}

	// =====================================================================
	// CarouselRow
	// =====================================================================

	CarouselRow::CarouselRow(const sf::Font& fontRef, const sf::String& label,
		std::vector<sf::String> options, std::size_t current, std::function<void(std::size_t)> onChange)
		: OptionRow(fontRef, label)
		, options(std::move(options))
		, current(this->options.empty() ? 0 : std::min(current, this->options.size() - 1))
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

		if (next != current)
		{
			current = next;
			if (onChange)
			{
				onChange(current);
			}
		}
	}

	sf::FloatRect CarouselRow::ArrowBox(int side) const
	{
		const sf::FloatRect area = ControlArea();
		const float x = side < 0 ? area.position.x : area.position.x + area.size.x;
		return { { x - ArrowHalf - 6.f, area.position.y }, { 2.f * ArrowHalf + 12.f, area.size.y } };
	}

	bool CarouselRow::HandlePointer(sf::Vector2f point, bool clicked)
	{
		if (!enabled)
		{
			return false;
		}

		if (ArrowBox(-1).contains(point))
		{
			if (clicked) { Adjust(-1); }
			return true;
		}
		if (ArrowBox(1).contains(point))
		{
			if (clicked) { Adjust(1); }
			return true;
		}
		return false;
	}

	void CarouselRow::RenderControl(sf::RenderTarget& target, float panelAlpha) const
	{
		const sf::FloatRect area = ControlArea();
		const float midY = area.position.y + area.size.y * 0.5f;

		const bool canLeft = enabled && current > 0;
		const bool canRight = enabled && !options.empty() && current + 1 < options.size();

		DrawTriangle(target, { area.position.x + ArrowHalf, midY }, -1,
			sf::Color((canLeft ? ArrowLive : ArrowDead).r, (canLeft ? ArrowLive : ArrowDead).g,
				(canLeft ? ArrowLive : ArrowDead).b, Alpha(panelAlpha)));
		DrawTriangle(target, { area.position.x + area.size.x - ArrowHalf, midY }, 1,
			sf::Color((canRight ? ArrowLive : ArrowDead).r, (canRight ? ArrowLive : ArrowDead).g,
				(canRight ? ArrowLive : ArrowDead).b, Alpha(panelAlpha)));

		if (!options.empty())
		{
			valueText.setString(options[current]);
			const sf::FloatRect bounds = valueText.getLocalBounds();
			valueText.setOrigin({ bounds.position.x + bounds.size.x * 0.5f, bounds.position.y + bounds.size.y * 0.5f });
			valueText.setPosition({ area.position.x + area.size.x * 0.5f, midY });
			const sf::Color c = enabled ? ValueColour : DisabledLabel;
			valueText.setFillColor(sf::Color(c.r, c.g, c.b, Alpha(panelAlpha)));
			target.draw(valueText);
		}
	}

	// =====================================================================
	// ToggleRow
	// =====================================================================

	ToggleRow::ToggleRow(const sf::Font& fontRef, const sf::String& label,
		const sf::String& onLabel, const sf::String& offLabel, bool on,
		std::function<void(bool)> onChange)
		: OptionRow(fontRef, label)
		, on(on)
		, onLabel(onLabel)
		, offLabel(offLabel)
		, onChange(std::move(onChange))
		, onText(fontRef, "", ValueSize)
		, offText(fontRef, "", ValueSize)
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

	sf::FloatRect ToggleRow::OptionBox(bool onSide) const
	{
		const sf::FloatRect area = ControlArea();
		const float half = area.size.x * 0.5f;
		const float x = onSide ? area.position.x + half : area.position.x;
		return { { x, area.position.y }, { half, area.size.y } };
	}

	bool ToggleRow::HandlePointer(sf::Vector2f point, bool clicked)
	{
		if (!enabled)
		{
			return false;
		}

		if (OptionBox(true).contains(point))
		{
			if (clicked) { Set(true); }
			return true;
		}
		if (OptionBox(false).contains(point))
		{
			if (clicked) { Set(false); }
			return true;
		}
		return false;
	}

	void ToggleRow::RenderControl(sf::RenderTarget& target, float panelAlpha) const
	{
		const auto drawOption = [&](sf::Text& text, const sf::String& label, bool active, bool onSide)
		{
			text.setString(label);
			const sf::FloatRect bounds = text.getLocalBounds();
			text.setOrigin({ bounds.position.x + bounds.size.x * 0.5f, bounds.position.y + bounds.size.y * 0.5f });

			const sf::FloatRect box = OptionBox(onSide);
			const sf::Vector2f centre{ box.position.x + box.size.x * 0.5f, box.position.y + box.size.y * 0.5f };
			text.setPosition(centre);

			if (active && enabled)
			{
				sf::RectangleShape pill({ bounds.size.x + 2.f * TogglePadding, box.size.y * 0.62f });
				pill.setOrigin(pill.getSize() * 0.5f);
				pill.setPosition(centre);
				pill.setFillColor(sf::Color(AccentColour.r, AccentColour.g, AccentColour.b, Alpha(panelAlpha, 0.22f)));
				pill.setOutlineThickness(2.f);
				pill.setOutlineColor(sf::Color(AccentColour.r, AccentColour.g, AccentColour.b, Alpha(panelAlpha)));
				target.draw(pill);
			}

			const sf::Color c = !enabled ? DisabledLabel : (active ? sf::Color::White : sf::Color(140, 146, 156));
			text.setFillColor(sf::Color(c.r, c.g, c.b, Alpha(panelAlpha)));
			target.draw(text);
		};

		drawOption(offText, offLabel, !on, false);
		drawOption(onText, onLabel, on, true);
	}
}
