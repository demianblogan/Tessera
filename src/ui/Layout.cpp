#include "Layout.h"

#include <algorithm>

#include <SFML/Graphics/RenderTarget.hpp>

namespace UI
{
	namespace
	{
		// Reads the "along the layout" (main) and "across the layout" (cross)
		// components of a vector, and rebuilds one from those, so Measure and
		// Arrange can be written once instead of once per orientation.
		struct Axes
		{
			bool horizontal = false;

			[[nodiscard]] float Main(sf::Vector2f v) const { return horizontal ? v.x : v.y; }
			[[nodiscard]] float Cross(sf::Vector2f v) const { return horizontal ? v.y : v.x; }

			[[nodiscard]] sf::Vector2f Make(float main, float cross) const
			{
				return horizontal ? sf::Vector2f{ main, cross } : sf::Vector2f{ cross, main };
			}
		};

		[[nodiscard]] float ResolveSize(const Element::SizeRule& rule, float measured, float available, float fill)
		{
			switch (rule.mode)
			{
			case Element::SizeMode::Pixels:  return rule.value;
			case Element::SizeMode::Percent: return available * rule.value;
			case Element::SizeMode::Fill:    return fill;
			default:                         return measured;
			}
		}
	}

	Layout::Layout(Orientation orientation)
		: orientation(orientation)
	{
		// No code
	}

	void Layout::Add(std::unique_ptr<Element> child)
	{
		children.push_back(std::move(child));
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

	sf::Vector2f Layout::Measure() const
	{
		const Axes axes{ orientation == Orientation::Horizontal };

		float mainTotal = 0.f;
		float crossMax = 0.f;

		for (const auto& child : children)
		{
			const sf::Vector2f measured = child->Measure();
			mainTotal += axes.Main(measured);
			crossMax = std::max(crossMax, axes.Cross(measured));
		}

		if (!children.empty())
		{
			mainTotal += gap * static_cast<float>(children.size() - 1);
		}

		const sf::Vector2f content = axes.Make(mainTotal, crossMax);

		return
		{
			content.x + padding.left + padding.right,
			content.y + padding.top + padding.bottom
		};
	}

	void Layout::Arrange(sf::Vector2f position, sf::Vector2f size)
	{
		Element::Arrange(position, size);

		const Axes axes{ orientation == Orientation::Horizontal };

		const sf::Vector2f available
		{
			size.x - padding.left - padding.right,
			size.y - padding.top - padding.bottom
		};
		const float availableMain = axes.Main(available);
		const float availableCross = axes.Cross(available);

		const auto mainRuleOf = [&axes](const Element& child)
			{
				return axes.horizontal ? child.GetWidthRule() : child.GetHeightRule();
			};
		const auto crossRuleOf = [&axes](const Element& child)
			{
				return axes.horizontal ? child.GetHeightRule() : child.GetWidthRule();
			};

		// Split the remaining main-axis space among the children that want to
		// fill it. Cross-axis Fill simply takes the whole cross extent.
		float fixedMain = 0.f;
		int fillMainCount = 0;

		for (const auto& child : children)
		{
			const Element::SizeRule rule = mainRuleOf(*child);

			if (rule.mode == SizeMode::Fill)
			{
				fillMainCount++;
			}
			else
			{
				fixedMain += ResolveSize(rule, axes.Main(child->Measure()), availableMain, 0.f);
			}
		}

		const float totalGap = gap * std::max(0.f, static_cast<float>(children.size()) - 1.f);
		const float fillMain = fillMainCount > 0
			? std::max(0.f, availableMain - fixedMain - totalGap) / static_cast<float>(fillMainCount)
			: 0.f;

		const Alignment crossAlignment = axes.horizontal ? verticalAlignment : horizontalAlignment;

		float cursorMain = 0.f;

		for (auto& child : children)
		{
			const sf::Vector2f measured = child->Measure();

			const float childMain = ResolveSize(mainRuleOf(*child), axes.Main(measured), availableMain, fillMain);
			float childCross = ResolveSize(crossRuleOf(*child), axes.Cross(measured), availableCross, availableCross);

			float crossOffset = 0.f;

			switch (crossAlignment)
			{
			case Alignment::Start:
				break;

			case Alignment::Center:
				crossOffset = (availableCross - childCross) / 2.f;
				break;

			case Alignment::End:
				crossOffset = availableCross - childCross;
				break;

			case Alignment::Stretch:
				childCross = availableCross;
				break;
			}

			const sf::Vector2f childPosition =
				position
				+ sf::Vector2f{ padding.left, padding.top }
				+ axes.Make(cursorMain, crossOffset);

			child->Arrange(childPosition, axes.Make(childMain, childCross));

			cursorMain += childMain + gap;
		}
	}

	void Layout::Render(sf::RenderTarget& target, NeonGlow* glow) const
	{
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