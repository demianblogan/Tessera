#include "PixelDust.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

#include "../utils/Random.h"

namespace
{
	constexpr float Gravity = 900.f;
	constexpr float MinSpeed = 60.f;
	constexpr float MaxSpeed = 320.f;
	constexpr float MinLife = 0.18f;
	constexpr float MaxLife = 0.42f;
	constexpr float MinSize = 2.f;
	constexpr float MaxSize = 5.f;
	constexpr float Pi = 3.14159265f;
}

namespace UI
{
	void PixelDust::Emit(sf::Vector2f centre, sf::Vector2f areaSize, sf::Color colour, int count)
	{
		particles.reserve(particles.size() + static_cast<std::size_t>(std::max(0, count)));

		for (int i = 0; i < count; ++i)
		{
			const sf::Vector2f origin{
				centre.x + Random::Float(-0.5f, 0.5f) * areaSize.x,
				centre.y + Random::Float(-0.5f, 0.5f) * areaSize.y };

			// Mostly outward from the centre, biased a little downward.
			const float angle = Random::Float(0.f, 2.f * Pi);
			const float speed = Random::Float(MinSpeed, MaxSpeed);
			const sf::Vector2f velocity{
				std::cos(angle) * speed,
				std::sin(angle) * speed * 0.6f + Random::Float(20.f, 120.f) };

			const float life = Random::Float(MinLife, MaxLife);

			particles.push_back(Particle{
				origin, velocity, Random::Float(MinSize, MaxSize), life, life, colour });
		}
	}

	void PixelDust::Update(float deltaTime)
	{
		for (Particle& particle : particles)
		{
			particle.velocity.y += Gravity * deltaTime;
			particle.position += particle.velocity * deltaTime;
			particle.life -= deltaTime;
		}

		std::erase_if(particles, [](const Particle& particle) { return particle.life <= 0.f; });
	}

	void PixelDust::Render(sf::RenderTarget& target) const
	{
		sf::RectangleShape square;

		for (const Particle& particle : particles)
		{
			const float fade = std::clamp(particle.life / particle.maxLife, 0.f, 1.f);

			square.setSize({ particle.size, particle.size });
			square.setOrigin({ particle.size * 0.5f, particle.size * 0.5f });
			square.setPosition(particle.position);
			square.setFillColor(sf::Color(particle.colour.r, particle.colour.g, particle.colour.b,
				static_cast<std::uint8_t>(fade * 255.f)));
			target.draw(square);
		}
	}
}
