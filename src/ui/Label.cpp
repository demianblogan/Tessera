#include "Label.h"

#include <SFML/Graphics/RenderTarget.hpp>

#include "TextLayout.h"

namespace UI
{
	Label::Label(const sf::Font& font, const sf::String& string, unsigned int characterSize)
		: text(font, string, characterSize)
	{
		// No code
	}

	void Label::ApplyFit()
	{
		text.setScale({ 1.f, 1.f });

		if (maxWidth > 0.f)
		{
			TextLayout::FitWidth(text, maxWidth);
		}
	}

	void Label::SetString(const sf::String& string)
	{
		text.setString(string);
		ApplyFit();
	}

	void Label::SetFillColor(sf::Color color)
	{
		text.setFillColor(color);
	}

	void Label::SetMaxWidth(float maxWidth)
	{
		this->maxWidth = maxWidth;
		ApplyFit();
	}

	sf::String Label::GetString() const
	{
		return text.getString();
	}

	float Label::GetVisualHeight() const
	{
		return static_cast<float>(text.getCharacterSize()) * text.getScale().y;
	}

	sf::Vector2f Label::Measure() const
	{
		const sf::FloatRect bounds = text.getLocalBounds();
		const sf::Vector2f scale = text.getScale();

		return { bounds.size.x * scale.x, bounds.size.y * scale.y };
	}

	void Label::Arrange(sf::Vector2f position, sf::Vector2f size)
	{
		Element::Arrange(position, size);

		const sf::FloatRect bounds = text.getLocalBounds();
		text.setPosition({ position.x, position.y - bounds.position.y * text.getScale().y });
	}

	void Label::Render(sf::RenderTarget& target) const
	{
		target.draw(text);
	}
}
