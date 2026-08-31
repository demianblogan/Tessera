#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/System/String.hpp>
#include <SFML/System/Vector2.hpp>

namespace sf
{
	class Font;
	class RenderTarget;
	class Texture;
}

namespace UI
{
	// One row of a settings panel: a left-aligned label and a control on the
	// right. Rows that use left / right arrows (carousel, slider) get the arrow
	// rendering, hover and press feedback from this base.
	class OptionRow
	{
	public:
		OptionRow(const sf::Font& font, const sf::String& label);
		virtual ~OptionRow() = default;

		void SetLayout(sf::Vector2f left, float width, float height);
		void SetEnabled(bool enabled) { this->enabled = enabled; }
		void SetSelected(bool selected) { this->selected = selected; }

		[[nodiscard]] bool IsEnabled() const { return enabled; }
		[[nodiscard]] sf::FloatRect Bounds() const;

		// Keyboard / pad: left or right (-1 / +1). Confirm flips a toggle.
		virtual void Adjust(int direction) = 0;
		virtual void Activate() {}

		// Mouse. Returns true if the point landed on an interactive part.
		virtual bool HandlePointer(sf::Vector2f point, bool clicked) = 0;

		void Update(float deltaTime);
		void Render(sf::RenderTarget& target, float panelAlpha) const;

	protected:
		[[nodiscard]] sf::FloatRect ControlArea() const;
		[[nodiscard]] std::uint8_t Alpha(float panelAlpha, float extra = 1.f) const;
		[[nodiscard]] sf::Color LabelColour() const;

		virtual void RenderControl(sf::RenderTarget& target, float panelAlpha) const = 0;
		virtual void UpdateControl(float /*deltaTime*/) {}

		// --- Left / right arrows (opt-in) ---
		void UseArrows(const sf::Texture& texture) { arrowTexture = &texture; }
		[[nodiscard]] sf::Vector2f ArrowCentre(int side) const;   // -1 left, +1 right
		[[nodiscard]] sf::FloatRect ArrowBox(int side) const;
		void PressArrow(int side);
		// Updates the hover state and returns -1 / +1 for the arrow under `point`
		// (0 for none). `leftLive` / `rightLive` gate the ends.
		int PickArrow(sf::Vector2f point, bool leftLive, bool rightLive);
		void DrawArrows(sf::RenderTarget& target, float panelAlpha, bool leftLive, bool rightLive) const;

		const sf::Font& font;
		mutable sf::Text labelText;

		sf::Vector2f left{ 0.f, 0.f };
		float width = 0.f;
		float height = 0.f;

		bool enabled = true;
		bool selected = false;
		float highlight = 0.f;

	private:
		const sf::Texture* arrowTexture = nullptr;
		float arrowPress[2] = { 1000.f, 1000.f };
		int hoveredArrow = 0;
	};

	// Label + [ <  value  > ]. Not a loop: the arrows grey out at the ends.
	class CarouselRow final : public OptionRow
	{
	public:
		CarouselRow(const sf::Font& font, const sf::String& label,
			std::vector<sf::String> options, std::size_t current,
			const sf::Texture& arrowTexture, std::function<void(std::size_t)> onChange);

		void Adjust(int direction) override;
		bool HandlePointer(sf::Vector2f point, bool clicked) override;

		void SetCurrent(std::size_t index);
		[[nodiscard]] std::size_t Current() const { return current; }

	protected:
		void RenderControl(sf::RenderTarget& target, float panelAlpha) const override;

	private:
		std::vector<sf::String> options;
		std::size_t current = 0;
		std::function<void(std::size_t)> onChange;
		mutable sf::Text valueText;
	};

	// Label + [ <  |=====fill====|  > ] with a percent read-out on the bar.
	class SliderRow final : public OptionRow
	{
	public:
		SliderRow(const sf::Font& font, const sf::String& label, const sf::Texture& arrowTexture,
			int steps, int current, std::function<void(int)> onChange);

		void Adjust(int direction) override;
		bool HandlePointer(sf::Vector2f point, bool clicked) override;

		void SetCurrent(int value);
		[[nodiscard]] int Current() const { return current; }

	protected:
		void RenderControl(sf::RenderTarget& target, float panelAlpha) const override;

	private:
		[[nodiscard]] sf::FloatRect BarRect() const;
		void Set(int value);

		int steps = 10;
		int current = 0;
		std::function<void(int)> onChange;
		mutable sf::Text percentText;
	};

	// Label + a checkbox (a tick when on, a cross when off, drawn over a
	// two-frame sprite).
	class ToggleRow final : public OptionRow
	{
	public:
		ToggleRow(const sf::Font& font, const sf::String& label,
			const sf::Texture& checkboxTexture, bool on, std::function<void(bool)> onChange);

		void Adjust(int direction) override;
		void Activate() override;
		bool HandlePointer(sf::Vector2f point, bool clicked) override;

		void SetOn(bool on);
		[[nodiscard]] bool IsOn() const { return on; }

	protected:
		void RenderControl(sf::RenderTarget& target, float panelAlpha) const override;

	private:
		void Set(bool value);
		[[nodiscard]] sf::FloatRect CheckboxBounds() const;

		bool on = false;
		const sf::Texture& checkboxTexture;
		std::function<void(bool)> onChange;
	};
}
