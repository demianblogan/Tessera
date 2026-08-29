#include "Slider.h"

#include <algorithm>
#include <cmath>

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

namespace UI
{
	Slider::Slider(
		sf::Vector2f trackSize,
		sf::Vector2f handleSize,
		float minimum,
		float maximum,
		float value,
		float step,
		Visual trackVisual,
		Visual fillVisual,
		Visual handleVisual
	)
		: preferredTrackSize(trackSize)
		, handleSize(handleSize)
		, minimum(std::min(minimum, maximum))
		, maximum(std::max(minimum, maximum))
		, step(std::max(0.f, step))
		, trackVisual(std::move(trackVisual))
		, fillVisual(std::move(fillVisual))
		, handleVisual(std::move(handleVisual))
	{
		SetValue(value);
		SetWidthPixels(trackSize.x);
		SetHeightPixels(std::max(trackSize.y, handleSize.y));
	}

	void Slider::SetValue(float value)
	{
		value = std::clamp(value, minimum, maximum);

		if (step > 0.f)
		{
			value = minimum + std::round((value - minimum) / step) * step;
			value = std::clamp(value, minimum, maximum);
		}

		this->value = value;
	}

	float Slider::GetValue() const
	{
		return value;
	}

	void Slider::SetValueFromPointer(sf::Vector2f point)
	{
		if (size.x <= 0.f)
		{
			return;
		}

		const float normalized = std::clamp((point.x - position.x) / size.x, 0.f, 1.f);
		SetValue(minimum + normalized * (maximum - minimum));
	}

	void Slider::Increase()
	{
		if (step > 0.f)
		{
			SetValue(value + step);
		}
	}

	void Slider::Decrease()
	{
		if (step > 0.f)
		{
			SetValue(value - step);
		}
	}

	void Slider::SetRange(float minimum, float maximum)
	{
		this->minimum = std::min(minimum, maximum);
		this->maximum = std::max(minimum, maximum);
		SetValue(value);
	}

	void Slider::SetStep(float step)
	{
		this->step = std::max(0.f, step);
		SetValue(value);
	}

	sf::Vector2f Slider::Measure() const
	{
		return
		{
			preferredTrackSize.x,
			std::max(preferredTrackSize.y, handleSize.y)
		};
	}

	bool Slider::Contains(sf::Vector2f point) const
	{
		return sf::FloatRect(position, size).contains(point);
	}

	float Slider::GetNormalizedValue() const
	{
		const float range = maximum - minimum;

		if (range <= 0.f)
		{
			return 0.f;
		}

		return (value - minimum) / range;
	}

	sf::Vector2f Slider::GetTrackPosition() const
	{
		return
		{
			position.x,
			position.y + (size.y - preferredTrackSize.y) / 2.f
		};
	}

	sf::Vector2f Slider::GetTrackSize() const
	{
		return { size.x, preferredTrackSize.y };
	}

	sf::Vector2f Slider::GetFillSize() const
	{
		return { size.x * GetNormalizedValue(),	preferredTrackSize.y };
	}

	sf::Vector2f Slider::GetHandlePosition() const
	{
		const float usableWidth = size.x - handleSize.x;

		return
		{
			position.x + usableWidth * GetNormalizedValue(),
			position.y + (size.y - handleSize.y) / 2.f
		};
	}

	void Slider::Render(sf::RenderTarget& target) const
	{
		DrawVisual(target, trackVisual, GetTrackPosition(), GetTrackSize());
		DrawVisual(target, fillVisual, GetTrackPosition(), GetFillSize());
		DrawVisual(target, handleVisual, GetHandlePosition(), handleSize);
	}

	void Slider::DrawVisual(sf::RenderTarget& target, const Visual& visual, sf::Vector2f position, sf::Vector2f size) const
	{
		if (std::holds_alternative<RectangleVisual>(visual))
		{
			const RectangleVisual& rectangle = std::get<RectangleVisual>(visual);
			sf::RectangleShape rect(size);

			rect.setPosition(position);
			rect.setFillColor(rectangle.fillColor);
			rect.setOutlineColor(rectangle.outlineColor);
			rect.setOutlineThickness(rectangle.outlineThickness);

			target.draw(rect);

			return;
		}

		const SpriteVisual& spriteVisual = std::get<SpriteVisual>(visual);
		const sf::Sprite& originalSprite = spriteVisual.sprite;
		const sf::FloatRect bounds = originalSprite.getLocalBounds();

		if (bounds.size.x <= 0.f || bounds.size.y <= 0.f)
		{
			return;
		}

		if (!spriteVisual.tiled)
		{
			sf::Sprite sprite = originalSprite;

			sprite.setPosition(position);
			sprite.setScale({ size.x / bounds.size.x, size.y / bounds.size.y });

			target.draw(sprite);

			return;
		}

		float currentX = 0.f;

		while (currentX < size.x)
		{
			const float segmentWidth = std::min(bounds.size.x, size.x - currentX);

			sf::Sprite sprite = originalSprite;
			sprite.setPosition({ position.x + currentX,	position.y });

			sf::IntRect textureRect = sprite.getTextureRect();
			textureRect.size.x = static_cast<int>(segmentWidth);

			sprite.setTextureRect(textureRect);
			sprite.setScale({ 1.f, size.y / bounds.size.y });

			target.draw(sprite);

			currentX += segmentWidth;
		}
	}
}