#pragma once

#include <array>
#include <vector>

#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Vector2.hpp>

namespace sf
{
	class RenderTarget;
}

namespace UI
{
	// The high-score flourish on the game-over screen: firework shells that rise
	// from below the screen and burst in warm colors, plus four continuous jets
	// of sparks streaming out of the panel's corners. Additive, CPU particles.
	// Fireworks draw behind the panel (RenderFireworks), the corner jets in front
	// of it (RenderCornerSparks).
	class Celebration
	{
	public:
		Celebration();

		// Panel corners in view coordinates: top-left, top-right, bottom-left,
		// bottom-right. The jets fire diagonally outward from each.
		void SetCorners(const std::array<sf::Vector2f, 4>& corners);

		void Update(float deltaTime);
		void RenderFireworks(sf::RenderTarget& target) const;
		void RenderCornerSparks(sf::RenderTarget& target) const;

	private:
		struct Rocket
		{
			sf::Vector2f position;
			sf::Vector2f velocity;
			float fuse = 0.f;
			sf::Color color;
		};

		struct Spark
		{
			sf::Vector2f position;
			sf::Vector2f velocity;
			float size = 2.f;
			float life = 0.f;
			float maxLife = 1.f;
			float gravity = 200.f;
			float drag = 0.f;
			sf::Color color;
		};

		void Explode(const Rocket& rocket);

		std::array<sf::Vector2f, 4> cornerPoints{};
		std::array<sf::Vector2f, 4> cornerDirections{};
		std::array<float, 4> cornerFlash{};
		std::array<float, 4> cornerFlashTimer{};
		bool isCornersSet = false;

		std::vector<Rocket> rockets;
		std::vector<Spark> burstSparks;
		std::vector<Spark> cornerSparks;

		float launchTimer = 0.f;
		float cornerEmitCarry = 0.f;
	};
}
