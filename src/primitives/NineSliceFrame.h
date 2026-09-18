#pragma once

#include <vector>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/System/Vector2.hpp>

namespace sf
{
	class RenderTarget;
	struct RenderStates;
	class Texture;
}

// The decorative border width, in source pixels, of the menu frame textures
// (menu_background_*_frame.png, 62x62). Shared so every panel that
// nine-slices one stays in sync when it is tuned.
inline constexpr unsigned int MenuFrameSourceBorder = 18u;

// A resizable frame drawn from a 3x3 grid of slices cut from one texture:
// four corners that never scale, four edges that stretch along one axis, and
// a center that stretches both ways. Keeps ornate corners crisp at any size.
//
// Rebuild it (construct a new one) whenever the destination bounds change.
class NineSliceFrame
{
public:
	NineSliceFrame(const sf::Texture& texture, sf::FloatRect destinationBounds,
		unsigned int sourceBorderPixels, sf::Vector2f targetBorderSize);

	void SetColor(sf::Color color);

	void Draw(sf::RenderTarget& target) const;
	// Same, but through extra render states -- e.g. a transform that slides
	// the whole frame without rebuilding it.
	void Draw(sf::RenderTarget& target, const sf::RenderStates& states) const;

private:
	std::vector<sf::Sprite> slices;
};
