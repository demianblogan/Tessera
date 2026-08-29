#include "Panel.h"

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Texture.hpp>

namespace UI
{
	Panel::Panel(const sf::Sprite& sprite)
		: backgroundSprite(sprite)
	{
		// No code
	}

	void Panel::SetPadding(sf::Vector2f padding)
	{
		this->padding = padding;
	}

	void Panel::SetChild(std::unique_ptr<Element> child)
	{
		this->child = std::move(child);
	}

	sf::Vector2f Panel::Measure() const
	{
		return size;
	}

	void Panel::Arrange(sf::Vector2f position, sf::Vector2f size)
	{
		Element::Arrange(position, size);

		frame = NineSliceFrame::ForWidget(backgroundSprite.getTexture(), { position, size });

		if (child == nullptr)
		{
			return;
		}

		child->Arrange(
			{
				position.x + padding.x,
				position.y + padding.y
			},
			{
				size.x - padding.x * 2.f,
				size.y - padding.y * 2.f
			}
		);
	}

	void Panel::Render(sf::RenderTarget& target) const
	{
		if (frame)
		{
			frame->Draw(target);
		}
		else
		{
			sf::Sprite sprite = backgroundSprite;
			const sf::FloatRect bounds = sprite.getLocalBounds();

			sprite.setPosition(position);
			sprite.setScale({ size.x / bounds.size.x, size.y / bounds.size.y });

			target.draw(sprite);
		}

		if (child)
		{
			child->Render(target);
		}
	}
}