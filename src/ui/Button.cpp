#include "Button.h"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

namespace UI
{
	Button::Button(sf::Vector2f size)
		: preferredSize(size)
	{
		SetWidthPixels(size.x);
		SetHeightPixels(size.y);
	}

	Button::Button(sf::Vector2f size, std::unique_ptr<Label> label)
		: Button(size)
	{
		this->label = std::move(label);
	}

	Button::Button(const sf::Sprite& sprite)
		: backgroundSprite(sprite)
	{
		const sf::FloatRect bounds = sprite.getGlobalBounds();
		preferredSize = bounds.size;

		SetWidthPixels(bounds.size.x);
		SetHeightPixels(bounds.size.y);
	}

	Button::Button(const sf::Sprite& sprite, std::unique_ptr<Label> label)
		: Button(sprite)
	{
		this->label = std::move(label);
	}

	void Button::SetLabel(std::unique_ptr<Label> label)
	{
		this->label = std::move(label);
	}

	Label* Button::GetLabel()
	{
		return label.get();
	}

	const Label* Button::GetLabel() const
	{
		return label.get();
	}

	void Button::SetTextAlignment(TextAlignment alignment)
	{
		textAlignment = alignment;
	}

	void Button::SetPadding(Padding padding)
	{
		this->padding = padding;
	}

	void Button::SetSelected(bool selected)
	{
		if (!enabled)
		{
			return;
		}

		this->selected = selected;
		UpdateVisualState();
	}

	bool Button::IsSelected() const
	{
		return selected;
	}

	void Button::SetEnabled(bool enabled)
	{
		this->enabled = enabled;

		if (!enabled)
		{
			selected = false;
		}

		UpdateVisualState();
	}

	bool Button::IsEnabled() const
	{
		return enabled;
	}

	void Button::SetNormalStyle(Style style)
	{
		normalStyle = std::move(style);
		UpdateVisualState();
	}

	void Button::SetSelectedStyle(Style style)
	{
		selectedStyle = std::move(style);
		UpdateVisualState();
	}

	void Button::SetDisabledStyle(Style style)
	{
		disabledStyle = std::move(style);
		UpdateVisualState();
	}

	void Button::SetPreferredSize(sf::Vector2f size)
	{
		preferredSize = size;
	}

	sf::Vector2f Button::Measure() const
	{
		if (preferredSize.x > 0.f || preferredSize.y > 0.f)
		{
			return preferredSize;
		}

		if (label != nullptr)
		{
			const sf::Vector2f labelSize = label->Measure();

			return
			{
				labelSize.x + padding.left + padding.right,
				labelSize.y + padding.top + padding.bottom
			};
		}

		return { 0.f, 0.f };
	}

	void Button::Arrange(sf::Vector2f position, sf::Vector2f size)
	{
		Element::Arrange(position, size);

		if (label == nullptr)
		{
			return;
		}

		const sf::Vector2f labelSize = label->Measure();
		sf::Vector2f labelPosition = position;

		switch (textAlignment)
		{
		case TextAlignment::Left:
			labelPosition.x += padding.left;
			break;

		case TextAlignment::Center:
			labelPosition.x += (size.x - labelSize.x) / 2.f;
			break;

		case TextAlignment::Right:
			labelPosition.x += size.x - labelSize.x - padding.right;
			break;
		}

		labelPosition.y += (size.y - labelSize.y) / 2.f;

		label->Arrange(labelPosition, labelSize);
	}

	bool Button::Contains(sf::Vector2f point) const
	{
		return sf::FloatRect(position, size).contains(point);
	}

	void Button::Render(sf::RenderTarget& target, sf::Shader* glowShader, float time) const
	{
		// =====================================================
		// Glow
		// =====================================================

		if (selected && glowShader)
		{
			const float glowPadding = 2.f;

			sf::RectangleShape glowRect;

			glowRect.setPosition(
				{
					position.x - glowPadding,
					position.y - glowPadding
				}
			);

			glowRect.setSize(
				{
					size.x + glowPadding * 2.f,
					size.y + glowPadding * 2.f
				}
			);

			glowRect.setFillColor(sf::Color(120, 220, 255, 110));

			glowShader->setUniform("time", time);

			sf::RenderStates glowStates;
			glowStates.shader = glowShader;
			glowStates.blendMode = sf::BlendAdd;

			target.draw(glowRect, glowStates);
		}

		// =====================================================
		// Normal button
		// =====================================================

		if (backgroundSprite)
		{
			sf::Sprite sprite = *backgroundSprite;

			const sf::FloatRect bounds = sprite.getLocalBounds();

			if (bounds.size.x > 0.f && bounds.size.y > 0.f)
			{
				sprite.setPosition(position);
				sprite.setScale(
					{
						size.x / bounds.size.x,
						size.y / bounds.size.y
					}
				);
				sprite.setColor(backgroundColor);

				target.draw(sprite);
			}
		}
		else
		{
			sf::RectangleShape rect(size);

			rect.setPosition(position);
			rect.setFillColor(backgroundColor);
			rect.setOutlineColor(outlineColor);
			rect.setOutlineThickness(outlineThickness);

			target.draw(rect);
		}

		if (label)
		{
			label->Render(target);
		}
	}

	void Button::Render(sf::RenderTarget& target) const
	{
		Render(target, nullptr, 0.f);
	}

	void Button::ApplyStyle(const Style& style)
	{
		if (style.backgroundColor)
		{
			backgroundColor = *style.backgroundColor;
		}

		if (style.outlineColor)
		{
			outlineColor = *style.outlineColor;
		}

		if (style.outlineThickness)
		{
			outlineThickness = *style.outlineThickness;
		}

		if (label && style.textColor)
		{
			label->SetFillColor(*style.textColor);
		}
	}

	void Button::UpdateVisualState()
	{
		ApplyStyle(normalStyle);

		if (!enabled)
		{
			ApplyStyle(disabledStyle);
			return;
		}

		if (selected)
		{
			ApplyStyle(selectedStyle);
		}
	}
}