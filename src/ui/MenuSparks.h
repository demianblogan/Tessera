#pragma once

#include <random>
#include <vector>

#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Vector2.hpp>

namespace sf
{
	class RenderTarget;
}

namespace UI
{
	// Ambient main-menu embers: small glowing motes in the tetromino colours
	// drifting slowly upward with a gentle sway, fading in and out. Additive,
	// dim. No interaction, no state.
	class MenuSparks
	{
	public:
		MenuSparks();

		void Update(float deltaTime);
		void Render(sf::RenderTarget& target) const;

	private:
		struct Spark
		{
			sf::Vector2f position;
			float riseSpeed = 0.f;
			float swayPhase = 0.f;
			float swaySpeed = 0.f;
			float swayAmp = 0.f;
			float baseX = 0.f;
			float size = 0.f;
			float age = 0.f;
			float lifetime = 1.f;
			float peakAlpha = 0.f;
			sf::Color colour;
		};

		void Respawn(Spark& spark, bool initial);

		std::vector<Spark> sparks;
		std::mt19937 rng;
	};
}
