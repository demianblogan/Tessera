#pragma once

#include <algorithm>

#include <SFML/Graphics/Text.hpp>
#include <SFML/System/Vector2.hpp>

namespace UI::TextLayout
{
	// Shrinks `text` (by scaling, never by changing its character size) until it
	// fits `maximumWidth`, down to a floor of `minimumSize` equivalent pixels.
	// Text already within the limit is left untouched.
	//
	// Scaling keeps the glyph size the font atlas was built for; calling
	// setCharacterSize instead would rebuild the atlas for every distinct line,
	// which is expensive once a screen is text-heavy or localized.
	inline void FitWidth(sf::Text& text, float maximumWidth, unsigned int minimumSize = 14u)
	{
		const auto characterSize = static_cast<float>(text.getCharacterSize());
		const float naturalWidth = text.getLocalBounds().size.x;

		if (characterSize <= 0.f || naturalWidth <= 0.f || maximumWidth <= 0.f)
		{
			return;
		}

		const float minimumScale = static_cast<float>(minimumSize) / characterSize;
		const float fitScale = std::clamp(maximumWidth / naturalWidth, minimumScale, 1.f);

		text.setScale({ fitScale, fitScale });
	}

	// Moves `text`'s origin to the centre of its own visual bounds, so a
	// following setPosition() centres it regardless of the string's length.
	inline void CentreOrigin(sf::Text& text)
	{
		const sf::FloatRect bounds = text.getLocalBounds();

		text.setOrigin(
			{
				bounds.position.x + bounds.size.x * 0.5f,
				bounds.position.y + bounds.size.y * 0.5f
			});
	}
}
