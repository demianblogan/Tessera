#pragma once

#include <vector>

#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Vector2.hpp>

namespace sf
{
	class RenderTarget;
}

// A short-lived burst of small square particles -- used to "disintegrate" a
// menu entry into pixels that scatter and wink out. Emit once, then Update /
// Render each frame; the particles are gone within about half a second.
class PixelDust
{
public:
	// Scatters `count` particles across an `areaSize` box centered on `center`,
	// each flung outward (and a little downward) in `color`.
	void Emit(sf::Vector2f center, sf::Vector2f areaSize, sf::Color color, int count);

	void Update(float deltaTime);
	void Render(sf::RenderTarget& target) const;

private:
	struct Particle
	{
		sf::Vector2f position;
		sf::Vector2f velocity;
		float sideLength = 2.f; // square side, in pixels
		float remainingLifetimeSeconds = 0.f;

		// the value remainingLifetimeSeconds started at, so Render can fade by fraction left
		float totalLifetimeSeconds = 1.f; 
		sf::Color color;
	};

	std::vector<Particle> particles;
};
