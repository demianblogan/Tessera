#pragma once

#include <vector>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/System/Vector2.hpp>

namespace sf
{
	class RenderTarget;
	class Texture;
}

namespace UI
{
	// A resizable frame drawn from a 3x3 grid of slices cut from one texture:
	// four corners that never scale, four edges that stretch along one axis, and
	// a centre that stretches both ways. Keeps ornate corners crisp at any size.
	//
	// Rebuild it (construct a new one) whenever the destination bounds change.
	class NineSliceFrame
	{
	public:
		NineSliceFrame(const sf::Texture& texture, sf::FloatRect destinationBounds,
			unsigned int sourceBorderPixels, sf::Vector2f targetBorderSize);

		// Builds a frame with border sizes derived from the texture and the
		// destination: the source border is a fraction of the texture's shorter
		// side (enough to hold the ornate corners), the on-screen border a
		// clamped fraction of the destination's shorter side.
		[[nodiscard]] static NineSliceFrame ForWidget(const sf::Texture& texture, sf::FloatRect destinationBounds);

		void SetColor(sf::Color color);

		void Draw(sf::RenderTarget& target) const;

	private:
		std::vector<sf::Sprite> slices;
	};
}
