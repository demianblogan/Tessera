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

#include "../utils/Easing.h"

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
	constexpr sf::Color ArrowPulseColor{ 255, 140, 45 };
	constexpr float ArrowPulseRadiusStart = 8.f;
	constexpr float ArrowPulseRadiusScale = 26.f;
	constexpr float ArrowPulseOutlineThickness = 4.f;
	constexpr float ArrowPulseAlphaScale = 120.f;
	constexpr float ArrowHoverScale = 1.18f;
	constexpr float ArrowHitPadding = 10.f;
	constexpr float ArrowCenterInsetLeft = 0.12f;    // fraction of the control area's width
	constexpr float ArrowCenterInsetRight = 0.88f;

	// Shared left/right content inset from the row's frame -- the label on the
	// left (OptionRow::Render) and the keycap on the right (KeyBindRow) both
	// sit this far in.
	constexpr float RowContentInset = 26.f;

	// ToggleRow::CheckboxBounds -- the hit box is larger than the drawn
	// checkbox by this half-extent scale.
	constexpr float CheckboxHitHalfScale = 0.7f;

	// ToggleRow::RenderControl -- the checkbox (and its tick/cross symbol)
	// grows slightly while selected.
	constexpr float CheckboxSelectedScale = 1.08f;

	// ToggleRow::RenderControl -- tick/cross geometry, as fractions of the
	// symbol size `s`.
	constexpr float TickP0X = 0.24f;
	constexpr float TickP0Y = 0.02f;
	constexpr float TickP1X = 0.06f;
	constexpr float TickP1Y = 0.20f;
	constexpr float TickP2X = 0.26f;
	constexpr float TickP2Y = 0.20f;
	constexpr float CrossBarLength = 0.62f;
	constexpr float CrossBarAngle = 45.f;

	// ToggleRow::RenderControl -- tick/cross symbol line thickness, scaled off
	// the checkbox size `s` but never thinner than this floor.
	constexpr float SymbolThicknessMin = 3.f;
	constexpr float SymbolThicknessFraction = 0.13f;

	const sf::Color SelectedLabel{ 255, 255, 255 };
	const sf::Color IdleLabel{ 210, 216, 226 };
	const sf::Color DisabledLabel{ 120, 124, 132 };
	const sf::Color ValueColor{ 236, 242, 250 };
	const sf::Color ArrowLive{ 210, 216, 224 };
	const sf::Color ArrowDead{ 96, 102, 110 };
	const sf::Color TickColor{ 110, 235, 145 };
	const sf::Color CrossColor{ 235, 120, 120 };

	[[nodiscard]] float VerticalCenter(const sf::Text& text, float rowTop, float rowHeight)
	{
		const sf::FloatRect bounds = text.getLocalBounds();
		return rowTop + rowHeight * 0.5f - (bounds.position.y + bounds.size.y * 0.5f);
	}

	using Easing::EaseOutCubic;

	[[nodiscard]] sf::Color WithAlpha(sf::Color color, std::uint8_t alpha) noexcept
	{
		return { color.r, color.g, color.b, alpha };
	}

	void RoundedLine(sf::RenderTarget& target, sf::Vector2f start, sf::Vector2f end, float thickness, sf::Color color)
	{
		const sf::Vector2f delta = end - start;
		const float length = std::sqrt(delta.x * delta.x + delta.y * delta.y);

		sf::RectangleShape segment({ length, thickness });
		segment.setOrigin({ 0.f, thickness * 0.5f });
		segment.setPosition(start);
		segment.setRotation(sf::radians(std::atan2(delta.y, delta.x)));
		segment.setFillColor(color);
		target.draw(segment);

		sf::CircleShape cap(thickness * 0.5f);
		cap.setOrigin({ thickness * 0.5f, thickness * 0.5f });
		cap.setFillColor(color);
		cap.setPosition(start);
		target.draw(cap);
		cap.setPosition(end);
		target.draw(cap);
	}

	void CenteredBar(sf::RenderTarget& target, sf::Vector2f center, float length, float thickness,
		float degrees, sf::Color color)
	{
		sf::RectangleShape bar({ length, thickness });
		bar.setOrigin(bar.getSize() * 0.5f);
		bar.setPosition(center);
		bar.setRotation(sf::degrees(degrees));
		bar.setFillColor(color);
		target.draw(bar);
	}

	[[nodiscard]] sf::Color MixColor(sf::Color colorA, sf::Color colorB, float mixFactor) noexcept
	{
		mixFactor = std::clamp(mixFactor, 0.f, 1.f);
		return
		{
			static_cast<std::uint8_t>(colorA.r + (colorB.r - colorA.r) * mixFactor),
			static_cast<std::uint8_t>(colorA.g + (colorB.g - colorA.g) * mixFactor),
			static_cast<std::uint8_t>(colorA.b + (colorB.b - colorA.b) * mixFactor)
		};
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
	{}

	void OptionRow::SetLayout(sf::Vector2f newLeft, float newWidth, float newHeight)
	{
		left = newLeft;
		width = newWidth;
		height = newHeight;
	}

	void OptionRow::SetEnabled(bool isEnabled)
	{
		this->isEnabled = isEnabled;
	}

	void OptionRow::SetSelected(bool isSelected)
	{
		this->isSelected = isSelected;
	}

	void OptionRow::SetAccent(sf::Color color)
	{
		accent = color;
	}

	bool OptionRow::IsEnabled() const
	{
		return isEnabled;
	}

	sf::FloatRect OptionRow::GetBounds() const
	{
		return { left, { width, height } };
	}

	void OptionRow::Activate()
	{}

	int OptionRow::GetHoveredArrow() const
	{
		return hoveredArrow;
	}

	sf::FloatRect OptionRow::GetControlArea() const
	{
		const float controlWidth = width * ControlFraction;
		return
		{
			{ left.x + width - controlWidth, left.y },
			{ controlWidth, height }
		};
	}

	std::uint8_t OptionRow::GetAlpha(float panelAlpha, float extra) const
	{
		return static_cast<std::uint8_t>(std::clamp(panelAlpha * extra, 0.f, 1.f) * 255.f);
	}

	void OptionRow::UpdateControl(float /*deltaTime*/)
	{}

	void OptionRow::UseArrows(const sf::Texture& texture)
	{
		arrowTexture = &texture;
	}

	sf::Color OptionRow::GetLabelColor() const
	{
		if (!isEnabled)
			return DisabledLabel;

		return
		{
			static_cast<std::uint8_t>(IdleLabel.r + (SelectedLabel.r - IdleLabel.r) * highlight),
			static_cast<std::uint8_t>(IdleLabel.g + (SelectedLabel.g - IdleLabel.g) * highlight),
			static_cast<std::uint8_t>(IdleLabel.b + (SelectedLabel.b - IdleLabel.b) * highlight)
		};
	}

	void OptionRow::Update(float deltaTime)
	{
		const float target = (isSelected && isEnabled) ? 1.f : 0.f;
		highlight += (target - highlight) * std::min(1.f, deltaTime * 12.f);

		for (float& time : arrowPress)
			time += deltaTime;

		UpdateControl(deltaTime);
	}

	void OptionRow::Render(sf::RenderTarget& target, float panelAlpha) const
	{
		if (panelAlpha <= 0.01f)
			return;

		if (highlight > 0.01f)
		{
			sf::RectangleShape fill({ width, height });
			fill.setPosition(left);
			fill.setFillColor(sf::Color(255, 255, 255, GetAlpha(panelAlpha, 0.10f * highlight)));
			target.draw(fill);

			sf::RectangleShape bar({ 5.f, height });
			bar.setPosition(left);
			bar.setFillColor(WithAlpha(accent, GetAlpha(panelAlpha, highlight)));
			target.draw(bar);
		}

		const sf::Color color = GetLabelColor();
		labelText.setFillColor(WithAlpha(color, GetAlpha(panelAlpha)));
		labelText.setPosition({ left.x + RowContentInset, VerticalCenter(labelText, left.y, height) });
		target.draw(labelText);

		RenderControl(target, panelAlpha);
	}

	// --- arrows ---

	sf::Vector2f OptionRow::GetArrowCenter(int side) const
	{
		const sf::FloatRect area = GetControlArea();
		const float midY = area.position.y + area.size.y * 0.5f;
		const float x = area.position.x + area.size.x * (side < 0 ? ArrowCenterInsetLeft : ArrowCenterInsetRight);
		return { x, midY };
	}

	sf::FloatRect OptionRow::GetArrowBox(int side) const
	{
		const sf::Vector2f center = GetArrowCenter(side);
		const float half = ArrowScreenSize * 0.5f + ArrowHitPadding;
		return { { center.x - half, center.y - half }, { 2.f * half, 2.f * half } };
	}

	void OptionRow::PressArrow(int side)
	{
		arrowPress[side < 0 ? 0 : 1] = 0.f;
	}

	int OptionRow::PickArrow(sf::Vector2f point, bool isLeftLive, bool isRightLive)
	{
		hoveredArrow = 0;
		if (isLeftLive && GetArrowBox(-1).contains(point))
			hoveredArrow = -1;
		else if (isRightLive && GetArrowBox(1).contains(point))
			hoveredArrow = 1;

		return hoveredArrow;
	}

	void OptionRow::DrawArrows(sf::RenderTarget& target, float panelAlpha, bool isLeftLive, bool isRightLive) const
	{
		if (arrowTexture == nullptr)
		{
			return;
		}

		const float baseScale = ArrowScreenSize / static_cast<float>(std::max(1u, arrowTexture->getSize().y));

		const auto drawArrow = [&](int side, bool isLive)
			{
				const std::size_t index = side < 0 ? 0u : 1u;
				const float press = std::clamp(1.f - arrowPress[index] / ArrowPressDuration, 0.f, 1.f);
				const bool isHovered = isLive && hoveredArrow == side;

				const sf::Vector2f center = GetArrowCenter(side);
				const sf::Vector2f drawCenter{ center.x - static_cast<float>(side) * ArrowPressShift * press, center.y };

				if (press > 0.f && isLive)
				{
					const float t = EaseOutCubic(1.f - press);
					const float radius = ArrowPulseRadiusStart + t * ArrowPulseRadiusScale;
					sf::CircleShape ring(radius);
					ring.setOrigin({ radius, radius });
					ring.setPosition(center);
					ring.setFillColor(sf::Color::Transparent);
					ring.setOutlineThickness(ArrowPulseOutlineThickness);
					ring.setOutlineColor(WithAlpha(ArrowPulseColor,
						static_cast<std::uint8_t>(press * press * ArrowPulseAlphaScale * std::clamp(panelAlpha, 0.f, 1.f))));
					target.draw(ring, sf::RenderStates(sf::BlendAdd));
				}

				sf::Sprite arrow(*arrowTexture);
				arrow.setOrigin(sf::Vector2f(arrowTexture->getSize()) * 0.5f);
				const float scale = baseScale * (1.f - ArrowPressDip * press) * (isHovered ? ArrowHoverScale : 1.f);
				arrow.setScale({ side < 0 ? -scale : scale, scale });   // texture points right
				arrow.setPosition(drawCenter);

				sf::Color base = isLive ? ArrowLive : ArrowDead;
				if (isHovered)
				{
					base = sf::Color(255, 255, 255);
				}
				const sf::Color tinted{
					static_cast<std::uint8_t>(base.r + (ArrowPressTint.r - base.r) * press),
					static_cast<std::uint8_t>(base.g + (ArrowPressTint.g - base.g) * press),
					static_cast<std::uint8_t>(base.b + (ArrowPressTint.b - base.b) * press) };
				arrow.setColor(WithAlpha(tinted, GetAlpha(panelAlpha)));
				target.draw(arrow);
			};

		drawArrow(-1, isLeftLive);
		drawArrow(1, isRightLive);
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
			current = std::min(index, options.size() - 1);
	}

	std::size_t CarouselRow::GetCurrent() const
	{
		return current;
	}

	void CarouselRow::Adjust(int direction)
	{
		if (!isEnabled || options.empty())
			return;

		PressArrow(direction);

		const std::size_t next = direction < 0 ? (current == 0 ? 0 : current - 1) : std::min(current + 1, options.size() - 1);

		if (next != current)
		{
			current = next;
			if (onChange)
				onChange(current);
		}
	}

	bool CarouselRow::HandlePointer(sf::Vector2f point, bool wasClicked)
	{
		if (!isEnabled || options.empty())
			return false;

		const int arrow = PickArrow(point, current > 0, current + 1 < options.size());
		if (wasClicked && arrow != 0)
			Adjust(arrow);

		return arrow != 0;
	}

	void CarouselRow::RenderControl(sf::RenderTarget& target, float panelAlpha) const
	{
		const sf::FloatRect area = GetControlArea();
		const float midY = area.position.y + area.size.y * 0.5f;

		if (!options.empty())
		{
			valueText.setString(options[current]);
			const sf::FloatRect bounds = valueText.getLocalBounds();
			valueText.setOrigin({ bounds.position.x + bounds.size.x * 0.5f, bounds.position.y + bounds.size.y * 0.5f });
			valueText.setPosition({ area.position.x + area.size.x * 0.5f, midY });
			const sf::Color c = isEnabled ? ValueColor : DisabledLabel;
			valueText.setFillColor(WithAlpha(c, GetAlpha(panelAlpha)));
			target.draw(valueText);
		}

		DrawArrows(target, panelAlpha, isEnabled && current > 0,
			isEnabled && !options.empty() && current + 1 < options.size());
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

	int SliderRow::GetCurrent() const
	{
		return current;
	}

	void SliderRow::Set(int value)
	{
		const int clampedValue = std::clamp(value, 0, steps);
		if (clampedValue != current)
		{
			current = clampedValue;
			if (onChange)
				onChange(current);
		}
	}

	void SliderRow::Adjust(int direction)
	{
		if (!isEnabled)
			return;

		PressArrow(direction);
		Set(current + direction);
	}

	sf::FloatRect SliderRow::GetBarRect() const
	{
		const sf::FloatRect area = GetControlArea();
		const float inset = area.size.x * 0.20f;   // clear of the arrows
		const float barHeight = 36.f;

		return
		{
			{ area.position.x + inset, area.position.y + area.size.y * 0.5f - barHeight * 0.5f },
			{ area.size.x - 2.f * inset, barHeight }
		};
	}

	bool SliderRow::HandlePointer(sf::Vector2f point, bool wasClicked)
	{
		if (!isEnabled)
			return false;

		const int arrow = PickArrow(point, current > 0, current < steps);
		if (arrow != 0)
		{
			if (wasClicked)
				Adjust(arrow);
			return true;
		}

		// Click on the bar jumps to the nearest step.
		const sf::FloatRect bar = GetBarRect();
		const sf::FloatRect hit{ { bar.position.x, bar.position.y - 16.f }, { bar.size.x, bar.size.y + 32.f } };
		if (hit.contains(point))
		{
			if (wasClicked && bar.size.x > 0.f)
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
		const sf::FloatRect bar = GetBarRect();
		const float fraction = static_cast<float>(current) / static_cast<float>(steps);

		const std::uint8_t a = GetAlpha(panelAlpha);
		const sf::Color track = isEnabled ? sf::Color(48, 54, 64) : sf::Color(40, 44, 50);
		const sf::Color fillColor = isEnabled ? accent : sf::Color(90, 100, 108);

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
			fillShape.setFillColor(WithAlpha(fillColor, a));
			target.draw(fillShape);
		}

		percentText.setString(std::to_string(current * (100 / steps)) + "%");
		const sf::FloatRect bounds = percentText.getLocalBounds();
		percentText.setOrigin({ bounds.position.x + bounds.size.x * 0.5f, bounds.position.y + bounds.size.y * 0.5f });
		percentText.setPosition({ bar.position.x + bar.size.x * 0.5f, bar.position.y + bar.size.y * 0.5f });
		percentText.setFillColor(WithAlpha(isEnabled ? sf::Color::White : DisabledLabel, a));
		percentText.setOutlineThickness(2.f);
		percentText.setOutlineColor(WithAlpha(sf::Color(10, 14, 20), a));
		target.draw(percentText);

		DrawArrows(target, panelAlpha, isEnabled && current > 0, isEnabled && current < steps);
	}

	// =====================================================================
	// ToggleRow
	// =====================================================================

	ToggleRow::ToggleRow(const sf::Font& fontRef, const sf::String& label,
		const sf::Texture& checkboxTexture, bool isOn, std::function<void(bool)> onChange)
		: OptionRow(fontRef, label)
		, isOn(isOn)
		, checkboxTexture(checkboxTexture)
		, onChange(std::move(onChange))
	{}

	void ToggleRow::SetOn(bool isOn)
	{
		this->isOn = isOn;
	}

	bool ToggleRow::IsOn() const
	{
		return isOn;
	}

	void ToggleRow::Set(bool newIsOn)
	{
		if (newIsOn != isOn)
		{
			isOn = newIsOn;
			if (onChange)
				onChange(isOn);
		}
	}

	void ToggleRow::Adjust(int direction)
	{
		if (isEnabled)
			Set(direction > 0);
	}

	void ToggleRow::Activate()
	{
		if (isEnabled)
			Set(!isOn);
	}

	sf::FloatRect ToggleRow::GetCheckboxBounds() const
	{
		const sf::FloatRect area = GetControlArea();
		const sf::Vector2f center{ area.position.x + area.size.x * 0.5f, area.position.y + area.size.y * 0.5f };

		return
		{
			{ center.x - CheckboxSize * CheckboxHitHalfScale, center.y - CheckboxSize * CheckboxHitHalfScale },
			{ CheckboxSize * 2.f * CheckboxHitHalfScale, CheckboxSize * 2.f * CheckboxHitHalfScale }
		};
	}

	bool ToggleRow::HandlePointer(sf::Vector2f point, bool wasClicked)
	{
		if (!isEnabled || !GetCheckboxBounds().contains(point))
			return false;

		if (wasClicked)
			Set(!isOn);

		return true;
	}

	void ToggleRow::RenderControl(sf::RenderTarget& target, float panelAlpha) const
	{
		const sf::FloatRect area = GetControlArea();
		const sf::Vector2f center{ area.position.x + area.size.x * 0.5f, area.position.y + area.size.y * 0.5f };

		const bool isLive = isSelected && isEnabled;
		const float scale = CheckboxSize / static_cast<float>(CheckboxOn.size.y) * (isLive ? CheckboxSelectedScale : 1.f);

		sf::Sprite box(checkboxTexture);
		box.setTextureRect(isOn ? CheckboxOn : CheckboxOff);
		box.setOrigin(sf::Vector2f(CheckboxOn.size) * 0.5f);
		box.setScale({ scale, scale });
		box.setPosition(center);
		box.setColor(WithAlpha(isEnabled ? (isLive ? sf::Color::White : sf::Color(210, 216, 224))
			: DisabledLabel, GetAlpha(panelAlpha)));
		target.draw(box);

		const float s = CheckboxSize * (isLive ? CheckboxSelectedScale : 1.f);
		const float thickness = std::max(SymbolThicknessMin, s * SymbolThicknessFraction);
		const sf::Color symbol =
			WithAlpha(isEnabled ? (isOn ? TickColor : CrossColor) : DisabledLabel, GetAlpha(panelAlpha));

		if (isOn)
		{
			const sf::Vector2f p0{ center.x - s * TickP0X, center.y + s * TickP0Y };
			const sf::Vector2f p1{ center.x - s * TickP1X, center.y + s * TickP1Y };
			const sf::Vector2f p2{ center.x + s * TickP2X, center.y - s * TickP2Y };
			RoundedLine(target, p0, p1, thickness, symbol);
			RoundedLine(target, p1, p2, thickness, symbol);
		}
		else
		{
			CenteredBar(target, center, s * CrossBarLength, thickness, CrossBarAngle, symbol);
			CenteredBar(target, center, s * CrossBarLength, thickness, -CrossBarAngle, symbol);
		}
	}

	// =====================================================================
	// KeyBindRow
	// =====================================================================

	namespace
	{
		constexpr float BlinkSpeed = 9.f;
		constexpr float FlashDuration = 0.5f;
		constexpr float KeycapMinWidth = 150.f;
		constexpr float KeycapTextPadding = 54.f;

		constexpr float KeyTextAccentMix = 0.4f;    // capturing-state text tint toward the accent
		constexpr float KeyFillFlashMix = 0.45f;    // how far the fill is pulled toward the flash color
		constexpr float KeyTextFlashMix = 0.7f;     // how far the text is pulled toward the flash color

		// KeyBindRow::RenderControl -- while capturing, the keycap's alpha
		// oscillates between these two bounds instead of sitting fully opaque.
		constexpr float CapturingBlinkAlphaBase = 0.55f;
		constexpr float CapturingBlinkAlphaAmplitude = 0.45f;

		// KeyBindRow::RenderControl -- keycap outline thickness, thicker while
		// capturing to draw the eye.
		constexpr float CapturingOutlineThickness = 3.5f;
		constexpr float IdleOutlineThickness = 2.f;
	}

	KeyBindRow::KeyBindRow(const sf::Font& fontRef, const sf::String& label, const sf::String& keyLabel)
		: OptionRow(fontRef, label)
		, keyText(fontRef, keyLabel, ValueSize)
	{}

	void KeyBindRow::Adjust(int /*direction*/)
	{
		// No-op: KeyBindRow has no left/right control, only Activate() (via the
		// panel driving capture start/end) and pointer clicks.
	}

	void KeyBindRow::SetKeyLabel(const sf::String& text)
	{
		keyText.setString(text);
	}

	void KeyBindRow::SetCapturing(bool isCapturing)
	{
		this->isCapturing = isCapturing;
		blink = 0.f;
	}

	void KeyBindRow::Flash(sf::Color color)
	{
		flashColor = color;
		flashTime = 0.f;
	}

	bool KeyBindRow::IsCapturing() const
	{
		return isCapturing;
	}

	void KeyBindRow::UpdateControl(float deltaTime)
	{
		blink += deltaTime;
		flashTime += deltaTime;
	}

	bool KeyBindRow::HandlePointer(sf::Vector2f point, bool)
	{
		return isEnabled && GetBounds().contains(point);
	}

	void KeyBindRow::RenderControl(sf::RenderTarget& target, float panelAlpha) const
	{
		const sf::FloatRect textBounds = keyText.getLocalBounds();
		const float boxWidth = std::max(KeycapMinWidth, textBounds.size.x + KeycapTextPadding);
		const float boxHeight = height * 0.6f;

		// Right-align the keycap so its inset from the frame mirrors the label's
		// inset on the left (see OptionRow::Render, RowContentInset).
		const sf::Vector2f center{ left.x + width - RowContentInset - boxWidth * 0.5f, left.y + height * 0.5f };

		const float flashK = std::clamp(1.f - flashTime / FlashDuration, 0.f, 1.f);
		const float blinkK = isCapturing
			? CapturingBlinkAlphaBase + CapturingBlinkAlphaAmplitude * std::sin(blink * BlinkSpeed)
			: 1.f;
		const std::uint8_t a = GetAlpha(panelAlpha, blinkK);

		sf::Color outline = (isCapturing || isSelected) ? accent : sf::Color(120, 130, 145);
		outline = MixColor(outline, flashColor, flashK);
		sf::Color fill = MixColor(sf::Color(28, 32, 40), flashColor, flashK * KeyFillFlashMix);

		sf::RectangleShape box({ boxWidth, boxHeight });
		box.setOrigin(box.getSize() * 0.5f);
		box.setPosition(center);
		box.setFillColor(WithAlpha(fill, a));
		box.setOutlineThickness(isCapturing ? CapturingOutlineThickness : IdleOutlineThickness);
		box.setOutlineColor(WithAlpha(outline, a));
		target.draw(box);

		keyText.setOrigin(
			{ textBounds.position.x + textBounds.size.x * 0.5f,
			textBounds.position.y + textBounds.size.y * 0.5f });
		keyText.setPosition(center);
		const sf::Color textColor = !isEnabled ? DisabledLabel
			: isCapturing ? MixColor(sf::Color::White, accent, KeyTextAccentMix)
			: sf::Color::White;
		keyText.setFillColor(WithAlpha(MixColor(textColor, flashColor, flashK * KeyTextFlashMix), a));
		target.draw(keyText);
	}
}
