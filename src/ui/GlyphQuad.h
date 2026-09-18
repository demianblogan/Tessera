#pragma once

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Glyph.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/System/Vector2.hpp>

namespace UI
{
	// Append one glyph's textured quad (two triangles) to `array`, in string-local
	// space: `penX` is the pen origin, `offset` a local nudge (e.g. for a drop
	// shadow or a per-letter wave). Top and bottom vertices carry separate
	// colors so a caller can draw a vertical gradient. The array must be a
	// Triangles primitive and the render state textured with the font atlas for
	// the matching character size.
	inline void AppendGlyphQuad(sf::VertexArray& array, float penX, const sf::Glyph& glyph,
		sf::Color topColor, sf::Color bottomColor, sf::Vector2f offset = {})
	{
		const sf::FloatRect bounds = glyph.bounds;
		const sf::FloatRect tex(glyph.textureRect);

		const float x0 = penX + bounds.position.x + offset.x;
		const float x1 = x0 + bounds.size.x;
		const float y0 = bounds.position.y + offset.y;
		const float y1 = y0 + bounds.size.y;

		const float u0 = tex.position.x;
		const float u1 = tex.position.x + tex.size.x;
		const float v0 = tex.position.y;
		const float v1 = tex.position.y + tex.size.y;

		array.append({ { x0, y0 }, topColor, { u0, v0 } });
		array.append({ { x1, y0 }, topColor, { u1, v0 } });
		array.append({ { x1, y1 }, bottomColor, { u1, v1 } });
		array.append({ { x0, y0 }, topColor, { u0, v0 } });
		array.append({ { x1, y1 }, bottomColor, { u1, v1 } });
		array.append({ { x0, y1 }, bottomColor, { u0, v1 } });
	}
}
