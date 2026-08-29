#pragma once

#include <memory>
#include <optional>

#include <SFML/Graphics/Sprite.hpp>

#include "Element.h"
#include "NineSliceFrame.h"

namespace UI
{
	class Panel final : public Element
	{
	private:
		sf::Sprite backgroundSprite;
		std::optional<NineSliceFrame> frame;
		std::unique_ptr<Element> child;
		sf::Vector2f padding = { 0.f, 0.f };

	public:
		explicit Panel(const sf::Sprite& sprite);

		void SetPadding(sf::Vector2f padding);
		void SetChild(std::unique_ptr<Element> child);

		[[nodiscard]] sf::Vector2f Measure() const override;
		void Arrange(sf::Vector2f position, sf::Vector2f size) override;

		void Render(sf::RenderTarget& target) const override;
	};
}