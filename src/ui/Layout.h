#pragma once

#include <memory>
#include <vector>

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

	public:
		explicit Layout(Orientation orientation);

		void Add(std::unique_ptr<Element> child);

		void SetGap(float gap);
		void SetPadding(Padding padding);
		void SetHorizontalAlignment(Alignment alignment);
		void SetVerticalAlignment(Alignment alignment);

		[[nodiscard]] sf::Vector2f Measure() const override;
		void Arrange(sf::Vector2f position, sf::Vector2f size) override;

		void Render(sf::RenderTarget& target, NeonGlow* glow) const override;
		void Render(sf::RenderTarget& target) const override;
	};
}