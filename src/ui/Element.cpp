#include "Element.h"

#include <SFML/Graphics/RenderTarget.hpp>

namespace UI
{
	void Element::Arrange(sf::Vector2f position, sf::Vector2f size)
	{
		this->position = position;
		this->size = size;
	}

	void Element::Render(sf::RenderTarget& target, NeonGlow* /* glow */) const
	{
		// By default an element has no glow and just draws itself.
		Render(target);
	}

	sf::Vector2f Element::GetPosition() const
	{
		return position;
	}

	sf::Vector2f Element::GetSize() const
	{
		return size;
	}

	void Element::SetWidthAuto()
	{
		widthRule.mode = SizeMode::Auto;
	}

	void Element::SetWidthPixels(float width)
	{
		widthRule.mode = SizeMode::Pixels;
		widthRule.value = width;
	}

	void Element::SetWidthPercent(float percent)
	{
		widthRule.mode = SizeMode::Percent;
		widthRule.value = percent;
	}

	void Element::SetWidthFill()
	{
		widthRule.mode = SizeMode::Fill;
	}

	void Element::SetHeightAuto()
	{
		heightRule.mode = SizeMode::Auto;
	}

	void Element::SetHeightPixels(float height)
	{
		heightRule.mode = SizeMode::Pixels;
		heightRule.value = height;
	}

	void Element::SetHeightPercent(float percent)
	{
		heightRule.mode = SizeMode::Percent;
		heightRule.value = percent;
	}

	void Element::SetHeightFill()
	{
		heightRule.mode = SizeMode::Fill;
	}

	const Element::SizeRule& Element::GetWidthRule() const
	{
		return widthRule;
	}

	const Element::SizeRule& Element::GetHeightRule() const
	{
		return heightRule;
	}
}