#pragma once

#include <memory>
#include <vector>

#include <SFML/Graphics/Color.hpp>

#include "Element.h"

namespace sf
{
	class RenderTarget;
}

namespace UI
{
	class Layout final : public Element
	{
	public:
		enum class Orientation
		{
			Horizontal,
			Vertical
		};

		enum class Alignment
		{
			Start,
			Center,
			End,
			Stretch
		};

		struct Padding
		{
			float left = 0.f;
			float top = 0.f;
			float right = 0.f;
			float bottom = 0.f;
		};

	private:
		std::vector<std::unique_ptr<Element>> children;

		Orientation orientation;
		Alignment horizontalAlignment = Alignment::Start;
		Alignment verticalAlignment = Alignment::Start;
		Padding padding;
		float gap = 0.f;

		sf::Color backgroundColor = sf::Color::Transparent;

		[[nodiscard]] float ResolveChildWidth(const Element& child, float availableWidth, float fillWidth) const;
		[[nodiscard]] float ResolveChildHeight(const Element& child, float availableHeight, float fillHeight) const;

	public:
		explicit Layout(Orientation orientation);

		void Add(std::unique_ptr<Element> child);
		[[nodiscard]] const std::vector<std::unique_ptr<Element>>& GetChildren() const;

		void SetGap(float gap);
		void SetPadding(Padding padding);
		void SetHorizontalAlignment(Alignment alignment);
		void SetVerticalAlignment(Alignment alignment);
		void SetBackgroundColor(sf::Color color);

		[[nodiscard]] sf::Vector2f Measure() const override;
		void Arrange(sf::Vector2f position, sf::Vector2f size) override;

		void Render(sf::RenderTarget& target, NeonGlow* glow) const override;
		void Render(sf::RenderTarget& target) const override;
	};
}