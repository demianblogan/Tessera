#pragma once

#include <SFML/Graphics/Text.hpp>

#include "Element.h"

namespace UI
{
	class Label final : public Element
	{
	private:
		sf::Text text;
		float maxWidth = 0.f;   // 0 = unconstrained

		void ApplyFit();

	public:
		Label(const sf::Font& font, const sf::String& string, unsigned int characterSize);

		void SetString(const sf::String& string);
		void SetFillColor(sf::Color color);

		// Caps the rendered width: the text scales down (never up, never past a
		// readable floor) to stay within `maxWidth`. 0 removes the cap.
		void SetMaxWidth(float maxWidth);

		[[nodiscard]] sf::String GetString() const;

		[[nodiscard]] sf::Vector2f Measure() const override;
		void Arrange(sf::Vector2f position, sf::Vector2f size) override;

		void Render(sf::RenderTarget& target) const override;
	};
}
