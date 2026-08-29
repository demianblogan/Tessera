#pragma once

#include <variant>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Sprite.hpp>

#include "Element.h"

namespace UI
{
	class Slider final : public Element
	{
	public:
		struct RectangleVisual
		{
			sf::Color fillColor = sf::Color::White;
			sf::Color outlineColor = sf::Color::Transparent;
			float outlineThickness = 0.f;
		};

		struct SpriteVisual
		{
			sf::Sprite sprite;
			bool tiled = false;
		};

		using Visual = std::variant<RectangleVisual, SpriteVisual>;

	private:
		sf::Vector2f preferredTrackSize;
		sf::Vector2f handleSize;

		float minimum = 0.f;
		float maximum = 1.f;
		float value = 0.f;
		float step = 0.f;

		Visual trackVisual;
		Visual fillVisual;
		Visual handleVisual;

		[[nodiscard]] float GetNormalizedValue() const;
		[[nodiscard]] sf::Vector2f GetTrackPosition() const;
		[[nodiscard]] sf::Vector2f GetTrackSize() const;
		[[nodiscard]] sf::Vector2f GetFillSize() const;
		[[nodiscard]] sf::Vector2f GetHandlePosition() const;

		void DrawVisual(sf::RenderTarget& target, const Visual& visual, sf::Vector2f position, sf::Vector2f size) const;

	public:
		Slider(
			sf::Vector2f trackSize,
			sf::Vector2f handleSize,
			float minimum,
			float maximum,
			float value,
			float step,
			Visual trackVisual,
			Visual fillVisual,
			Visual handleVisual
		);

		void SetValue(float value);
		[[nodiscard]] float GetValue() const;

		// Sets the value from a pointer position along the track (for mouse
		// click / drag). `point` is in the slider's own coordinate space.
		void SetValueFromPointer(sf::Vector2f point);

		void Increase();
		void Decrease();

		void SetRange(float minimum, float maximum);
		void SetStep(float step);

		[[nodiscard]] sf::Vector2f Measure() const override;

		[[nodiscard]] bool Contains(sf::Vector2f point) const;

		void Render(sf::RenderTarget& target) const override;
	};
}