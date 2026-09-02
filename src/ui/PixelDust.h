#pragma once

#include <vector>

#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Vector2.hpp>

namespace sf
{
	class RenderTarget;
}

namespace UI
{
	// A short-lived burst of small square particles -- used to "disintegrate" a
	// menu entry into pixels that scatter and wink out. Emit once, then Update /
	// Render each frame; the particles are gone within about half a second.
	class PixelDust
	{
	public:
		// Scatters `count` particles across an `areaSize` box centred on `centre`,
		// each flung outward (and a little downward) in `colour`.
		void Emit(sf::Vector2f centre, sf::Vector2f areaSize, sf::Color colour, int count);

		void Update(float deltaTime);
		void Render(sf::RenderTarget& target) const;

		[[nodiscard]] bool Empty() const { return particles.empty(); }

	private:
		struct Particle
		{
			sf::Vector2f position;
			sf::Vector2f velocity;
			float size = 2.f;
			float life = 0.f;       // seconds remaining
			float maxLife = 1.f;
			sf::Color colour;
		};

		std::vector<Particle> particles;
	};
}
