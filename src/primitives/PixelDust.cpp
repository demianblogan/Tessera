#include "PixelDust.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <cstdint>

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

#include "../utils/Random.h"

namespace
{
	constexpr float Gravity = 900.f;
	constexpr float MinInitialSpeed = 60.f;
	constexpr float MaxInitialSpeed = 320.f;
	constexpr float MinLifetimeSeconds = 0.18f;
	constexpr float MaxLifetimeSeconds = 0.42f;
	constexpr float MinSideLength = 2.f;
	constexpr float MaxSideLength = 5.f;
}

void PixelDust::Emit(sf::Vector2f center, sf::Vector2f areaSize, sf::Color color, int count)
{
	particles.reserve(particles.size() + static_cast<std::size_t>(std::max(0, count)));

	for (int i = 0; i < count; i++)
	{
		const sf::Vector2f origin
		{
			center.x + Random::Float(-0.5f, 0.5f) * areaSize.x,
			center.y + Random::Float(-0.5f, 0.5f) * areaSize.y
		};

		// Mostly outward from the center, biased a little downward.
		const float angle = Random::Float(0.f, 2.f * std::numbers::pi_v<float>);
		const float speed = Random::Float(MinInitialSpeed, MaxInitialSpeed);
		const sf::Vector2f velocity
		{
			std::cos(angle) * speed,
			std::sin(angle) * speed * 0.6f + Random::Float(20.f, 120.f)
		};

		const float lifetime = Random::Float(MinLifetimeSeconds, MaxLifetimeSeconds);

		particles.push_back(
			Particle{
				origin,
				velocity,
				Random::Float(MinSideLength, MaxSideLength),
				lifetime,
				lifetime,
				color
			});
	}
}

void PixelDust::Update(float deltaTime)
{
	for (Particle& particle : particles)
	{
		particle.velocity.y += Gravity * deltaTime;
		particle.position += particle.velocity * deltaTime;
		particle.remainingLifetimeSeconds -= deltaTime;
	}

	std::erase_if(particles,
		[](const Particle& particle) { return particle.remainingLifetimeSeconds <= 0.f; });
}

void PixelDust::Render(sf::RenderTarget& target) const
{
	sf::RectangleShape square;

	for (const Particle& particle : particles)
	{
		const float fade = std::clamp(particle.remainingLifetimeSeconds / particle.totalLifetimeSeconds, 0.f, 1.f);

		square.setSize({ particle.sideLength, particle.sideLength });
		square.setOrigin({ particle.sideLength * 0.5f, particle.sideLength * 0.5f });
		square.setPosition(particle.position);
		square.setFillColor(sf::Color(particle.color.r, particle.color.g, particle.color.b,
			static_cast<std::uint8_t>(fade * 255.f)));

		target.draw(square);
	}
}
