#include "MenuBackdrop.h"

#include <array>
#include <cmath>
#include <cstdint>

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/Angle.hpp>

namespace
{
	constexpr sf::Vector2f VirtualSize{ 1920.f, 1080.f };
	constexpr int BlockSpriteSize = 16;

	constexpr int PieceCount = 18;

	constexpr float MinCellSize = 16.f;
	constexpr float MaxCellSize = 46.f;
	constexpr float MinAlpha = 0.05f;
	constexpr float MaxAlpha = 0.17f;
	constexpr float MaxSpin = 22.f;         // degrees per second

	// Cell coordinates of each shape's four blocks (spawn orientation).
	constexpr std::array<std::array<sf::Vector2i, 4>, 7> ShapeCells{ {
		{ { { 0, 1 }, { 1, 1 }, { 2, 1 }, { 3, 1 } } },   // I
		{ { { 1, 0 }, { 2, 0 }, { 1, 1 }, { 2, 1 } } },   // O
		{ { { 1, 0 }, { 0, 1 }, { 1, 1 }, { 2, 1 } } },   // T
		{ { { 1, 0 }, { 2, 0 }, { 0, 1 }, { 1, 1 } } },   // S
		{ { { 0, 0 }, { 1, 0 }, { 1, 1 }, { 2, 1 } } },   // Z
		{ { { 0, 0 }, { 0, 1 }, { 1, 1 }, { 2, 1 } } },   // J
		{ { { 2, 0 }, { 0, 1 }, { 1, 1 }, { 2, 1 } } },   // L
	} };
}

namespace UI
{
	MenuBackdrop::MenuBackdrop(const sf::Texture& blockSheet)
		: sheet(blockSheet)
		, rng(std::random_device{}())
	{
		for (std::size_t type = 0; type < ShapeCells.size(); ++type)
		{
			sf::Vector2f centroid;
			for (const sf::Vector2i& cell : ShapeCells[type])
			{
				centroid += sf::Vector2f(cell);
			}
			centroid /= 4.f;

			for (std::size_t i = 0; i < 4; ++i)
			{
				relativeCells[type][i] = sf::Vector2f(ShapeCells[type][i]) - centroid;
			}
		}

		pieces.resize(PieceCount);
		for (Piece& piece : pieces)
		{
			Respawn(piece, true);
		}
	}

	void MenuBackdrop::Respawn(Piece& piece, bool initial)
	{
		std::uniform_real_distribution<float> unit(0.f, 1.f);
		std::uniform_int_distribution<int> typePick(0, 6);

		piece.type = typePick(rng);
		piece.cellSize = MinCellSize + unit(rng) * (MaxCellSize - MinCellSize);

		const float depth = (piece.cellSize - MinCellSize) / (MaxCellSize - MinCellSize);   // 0 far .. 1 near
		piece.fallSpeed = 18.f + depth * 55.f;
		piece.alpha = MinAlpha + depth * (MaxAlpha - MinAlpha);
		piece.angularVelocity = (unit(rng) * 2.f - 1.f) * MaxSpin;
		piece.angleDegrees = unit(rng) * 360.f;

		const float x = -120.f + unit(rng) * (VirtualSize.x + 240.f);
		const float y = initial
			? unit(rng) * (VirtualSize.y + 200.f) - 100.f
			: -160.f - unit(rng) * 260.f;
		piece.position = { x, y };
	}

	void MenuBackdrop::Update(float deltaTime)
	{
		for (Piece& piece : pieces)
		{
			piece.position.y += piece.fallSpeed * deltaTime;
			piece.angleDegrees += piece.angularVelocity * deltaTime;

			if (piece.position.y - 4.f * piece.cellSize > VirtualSize.y)
			{
				Respawn(piece, false);
			}
		}
	}

	void MenuBackdrop::Render(sf::RenderTarget& target) const
	{
		sf::Sprite cell(sheet);
		cell.setOrigin({ BlockSpriteSize * 0.5f, BlockSpriteSize * 0.5f });

		for (const Piece& piece : pieces)
		{
			const float scale = piece.cellSize / static_cast<float>(BlockSpriteSize);
			const sf::Angle angle = sf::degrees(piece.angleDegrees);
			const float cos = std::cos(angle.asRadians());
			const float sin = std::sin(angle.asRadians());

			cell.setTextureRect(sf::IntRect{ { piece.type * BlockSpriteSize, 0 }, { BlockSpriteSize, BlockSpriteSize } });
			cell.setScale({ scale, scale });
			cell.setRotation(angle);
			cell.setColor(sf::Color(255, 255, 255, static_cast<std::uint8_t>(piece.alpha * 255.f)));

			for (const sf::Vector2f& rel : relativeCells[static_cast<std::size_t>(piece.type)])
			{
				const sf::Vector2f scaled = rel * piece.cellSize;
				const sf::Vector2f rotated{
					scaled.x * cos - scaled.y * sin,
					scaled.x * sin + scaled.y * cos };
				cell.setPosition(piece.position + rotated);
				target.draw(cell);
			}
		}
	}
}
