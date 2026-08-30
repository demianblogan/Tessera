#pragma once

#include <SFML/Graphics/Sprite.hpp>
#include <SFML/System/Vector2.hpp>

namespace sf
{
	class RenderTarget;
	class Texture;
}

namespace UI
{
	// The game's own mouse cursor, drawn as a screen overlay in place of the
	// hidden system cursor. The glow is applied to the sprite itself -- an
	// additive, pulsing pass confined to the cursor's own pixels -- not a halo
	// around it. No trail.
	class GlowingCursor
	{
	public:
		explicit GlowingCursor(const sf::Texture& texture);

		void Update(float deltaTime);
		void Render(sf::RenderTarget& target, sf::Vector2f position);

	private:
		sf::Sprite sprite;
		float pulseTime = 0.f;
	};
}
