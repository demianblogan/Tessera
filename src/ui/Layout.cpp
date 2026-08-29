#include "Layout.h"

#include <algorithm>

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

namespace UI
{
	Layout::Layout(Orientation orientation)
		: orientation(orientation)
	{
		// No code
	}

	void Layout::Add(std::unique_ptr<Element> child)
	{
		children.push_back(std::move(child));
	}

	const std::vector<std::unique_ptr<Element>>& Layout::GetChildren() const
	{
		return children;
	}

	void Layout::SetGap(float gap)
	{
		this->gap = gap;
	}

	void Layout::SetPadding(Padding padding)
	{
		this->padding = padding;
	}

	void Layout::SetHorizontalAlignment(Alignment alignment)
	{
		horizontalAlignment = alignment;
	}

	void Layout::SetVerticalAlignment(Alignment alignment)
	{
		verticalAlignment = alignment;
	}

	void Layout::SetBackgroundColor(sf::Color color)
	{
		backgroundColor = color;
	}

	sf::Vector2f Layout::Measure() const
	{
		float width = 0.f;
		float height = 0.f;

		if (orientation == Orientation::Vertical)
		{
			for (const auto& child : children)
			{
				const sf::Vector2f size = child->Measure();
				width = std::max(width, size.x);
				height += size.y;
			}

			if (!children.empty())
			{
				height += gap * static_cast<float>(children.size() - 1);
			}
		}
		else
		{
			for (const auto& child : children)
			{
				const sf::Vector2f size = child->Measure();
				width += size.x;
				height = std::max(height, size.y);
			}

			if (!children.empty())
			{
				width += gap * static_cast<float>(children.size() - 1);
			}
		}

		width += padding.left + padding.right;
		height += padding.top + padding.bottom;

		return { width, height };
	}

	void Layout::Arrange(sf::Vector2f position, sf::Vector2f size)
	{
		Element::Arrange(position, size);

		const float availableWidth = size.x - padding.left - padding.right;
		const float availableHeight = size.y - padding.top - padding.bottom;

		float fixedWidth = 0.f;
		float fixedHeight = 0.f;

		int fillWidthCount = 0;
		int fillHeightCount = 0;

		for (const auto& child : children)
		{
			const auto& widthRule = child->GetWidthRule();
			const auto& heightRule = child->GetHeightRule();
			const sf::Vector2f measured = child->Measure();

			if (widthRule.mode == SizeMode::Fill)
			{
				fillWidthCount++;
			}
			else
			{
				switch (widthRule.mode)
				{
				case SizeMode::Pixels:
					fixedWidth += widthRule.value;
					break;

				case SizeMode::Percent:
					fixedWidth += availableWidth * widthRule.value;
					break;

				default:
					fixedWidth += measured.x;
					break;
				}
			}

			if (heightRule.mode == SizeMode::Fill)
			{
				fillHeightCount++;
			}
			else
			{
				switch (heightRule.mode)
				{
				case SizeMode::Pixels:
					fixedHeight += heightRule.value;
					break;

				case SizeMode::Percent:
					fixedHeight += availableHeight * heightRule.value;
					break;

				default:
					fixedHeight += measured.y;
					break;
				}
			}
		}

		const float totalGap = gap * static_cast<float>(std::max(0, static_cast<int>(children.size()) - 1));

		const float remainingWidth = availableWidth - fixedWidth - totalGap;
		const float remainingHeight = availableHeight - fixedHeight - totalGap;

		const float fillWidth = fillWidthCount > 0 ? remainingWidth / static_cast<float>(fillWidthCount) : 0.f;
		const float fillHeight = fillHeightCount > 0 ? remainingHeight / static_cast<float>(fillHeightCount) : 0.f;

		sf::Vector2f cursor =
		{
			position.x + padding.left,
			position.y + padding.top
		};

		if (orientation == Orientation::Vertical)
		{
			for (auto& child : children)
			{
				float childWidth = ResolveChildWidth(*child, availableWidth, fillWidth);
				float childHeight = ResolveChildHeight(*child, availableHeight, fillHeight);

				sf::Vector2f childPosition = cursor;

				switch (horizontalAlignment)
				{
				case Alignment::Start:
					break;

				case Alignment::Center:
					childPosition.x += (availableWidth - childWidth) / 2.f;
					break;

				case Alignment::End:
					childPosition.x += availableWidth - childWidth;
					break;

				case Alignment::Stretch:
					childWidth = availableWidth;
					break;
				}

				child->Arrange(childPosition, { childWidth, childHeight });

				cursor.y += childHeight + gap;
			}
		}
		else
		{
			for (auto& child : children)
			{
				float childWidth = ResolveChildWidth(*child, availableWidth, fillWidth);
				float childHeight = ResolveChildHeight(*child, availableHeight, fillHeight);

				sf::Vector2f childPosition = cursor;

				switch (verticalAlignment)
				{
				case Alignment::Center:
					childPosition.y += (availableHeight - childHeight) / 2.f;
					break;

				case Alignment::End:
					childPosition.y += availableHeight - childHeight;
					break;

				case Alignment::Stretch:
					childHeight = availableHeight;
					break;
				}

				child->Arrange(childPosition, { childWidth,	childHeight });

				cursor.x += childWidth + gap;
			}
		}
	}

	float Layout::ResolveChildWidth(const Element& child, float availableWidth, float fillWidth) const
	{
		const auto& rule = child.GetWidthRule();

		switch (rule.mode)
		{
		case SizeMode::Pixels:
			return rule.value;

		case SizeMode::Percent:
			return availableWidth * rule.value;

		case SizeMode::Fill:
			return fillWidth;

		default:
			return child.Measure().x;
		}
	}

	float Layout::ResolveChildHeight(const Element& child, float availableHeight, float fillHeight) const
	{
		const auto& rule = child.GetHeightRule();

		switch (rule.mode)
		{
		case SizeMode::Pixels:
			return rule.value;

		case SizeMode::Percent:
			return availableHeight * rule.value;

		case SizeMode::Fill:
			return fillHeight;

		default:
			return child.Measure().y;
		}
	}

	void Layout::Render(sf::RenderTarget& target, NeonGlow* glow) const
	{
		if (backgroundColor != sf::Color::Transparent)
		{
			sf::RectangleShape rect(size);

			rect.setPosition(position);
			rect.setFillColor(backgroundColor);

			target.draw(rect);
		}

		for (const auto& child : children)
		{
			child->Render(target, glow);
		}
	}

	void Layout::Render(sf::RenderTarget& target) const
	{
		Render(target, nullptr);
	}
}