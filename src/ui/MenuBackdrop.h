#pragma once

#include <array>
#include <random>
#include <vector>

#include <SFML/System/Vector2.hpp>

namespace sf
{
	class RenderTarget;
	class Texture;
}

namespace UI
{
	// Ambient main-menu background: tetromino silhouettes drifting slowly down
	// at a range of sizes and speeds, so the field reads as having depth. Dim
	// enough not to fight the title or the ring. No interaction, no state.
	class MenuBackdrop
	{
	public:
		explicit MenuBackdrop(const sf::Texture& blockSheet);

		void Update(float deltaTime);
		void Render(sf::RenderTarget& target) const;

	private:
		struct Piece
		{
			int type = 0;
			sf::Vector2f position;   // centre, virtual coordinates
			float angleDegrees = 0.f;
			float angularVelocity = 0.f;
			float fallSpeed = 0.f;
			float cellSize = 0.f;
			float alpha = 0.f;
		};

		void Respawn(Piece& piece, bool initial);

		const sf::Texture& sheet;
		std::array<std::array<sf::Vector2f, 4>, 7> relativeCells{};   // cell offsets from each shape's centroid
		std::vector<Piece> pieces;
		std::mt19937 rng;
	};
}
