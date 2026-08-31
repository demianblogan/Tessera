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
}

namespace UI
{
	// One row of a settings panel: a left-aligned label and a control on the
	// right. Concrete rows below add a left/right carousel or an on/off toggle.
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
		// The right-hand area the control is laid out in.
		[[nodiscard]] sf::FloatRect ControlArea() const;
		[[nodiscard]] std::uint8_t Alpha(float panelAlpha, float extra = 1.f) const;
		[[nodiscard]] sf::Color LabelColour() const;

		virtual void RenderControl(sf::RenderTarget& target, float panelAlpha) const = 0;

		const sf::Font& font;
		mutable sf::Text labelText;

		sf::Vector2f left{ 0.f, 0.f };
		float width = 0.f;
		float height = 0.f;

		bool enabled = true;
		bool selected = false;
		float highlight = 0.f;   // eases toward 1 while selected
	};

	// Label + [ <  value  > ]. Not a loop: the arrows grey out at the ends.
	class CarouselRow final : public OptionRow
	{
	public:
		CarouselRow(const sf::Font& font, const sf::String& label,
			std::vector<sf::String> options, std::size_t current,
			std::function<void(std::size_t)> onChange);

		void Adjust(int direction) override;
		bool HandlePointer(sf::Vector2f point, bool clicked) override;

		void SetCurrent(std::size_t index);
		[[nodiscard]] std::size_t Current() const { return current; }

	protected:
		void RenderControl(sf::RenderTarget& target, float panelAlpha) const override;

	private:
		[[nodiscard]] sf::FloatRect ArrowBox(int side) const;   // -1 left, +1 right

		std::vector<sf::String> options;
		std::size_t current = 0;
		std::function<void(std::size_t)> onChange;
		mutable sf::Text valueText;
	};

	// Label + [ On | Off ].
	class ToggleRow final : public OptionRow
	{
	public:
		ToggleRow(const sf::Font& font, const sf::String& label,
			const sf::String& onLabel, const sf::String& offLabel, bool on,
			std::function<void(bool)> onChange);

		void Adjust(int direction) override;
		void Activate() override;
		bool HandlePointer(sf::Vector2f point, bool clicked) override;

		void SetOn(bool on);
		[[nodiscard]] bool IsOn() const { return on; }

	protected:
		void RenderControl(sf::RenderTarget& target, float panelAlpha) const override;

	private:
		void Set(bool value);
		[[nodiscard]] sf::FloatRect OptionBox(bool onSide) const;

		bool on = false;
		sf::String onLabel;
		sf::String offLabel;
		std::function<void(bool)> onChange;
		mutable sf::Text onText;
		mutable sf::Text offText;
	};
}
