#include "MenuBackdrop.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/Angle.hpp>

#include "../display/DisplaySettings.h"

namespace
{
	using Display::VirtualSize;

	constexpr int BlockSpriteSize = 16;

	constexpr int PieceCount = 18;

	constexpr float MinCellSize = 16.f;
	constexpr float MaxCellSize = 46.f;
	constexpr float MinAlpha = 0.05f;
	constexpr float MaxAlpha = 0.17f;
	constexpr float MaxSpin = 22.f;         // degrees per second

	constexpr float MinFallSpeed = 55.f;
	constexpr float FallSpeedByDepth = 145.f;   // added on top, scaled by the piece's depth

	constexpr float PushImpulse = 620.f;   // sideways velocity added per menu switch
	constexpr float MaxDrift = 1400.f;     // cap on stacked impulses
	constexpr float DriftDecay = 2.6f;     // per second, exponential

	constexpr float WrapMargin = 140.f;

	// Respawn() -- horizontal spawn span (a different tunable from WrapMargin,
	// which governs the wrap-around while a piece is alive).
	constexpr float SpawnMarginX = 120.f;

	// Respawn() -- vertical spawn span: pieces seeded at startup are scattered
	// across (and a little above/below) the whole screen; pieces respawning
	// after falling off the bottom start a bit above the top edge instead.
	constexpr float InitialSpawnYPad = 200.f;
	constexpr float InitialSpawnYOffset = 100.f;
	constexpr float RespawnAboveScreenMin = 160.f;
	constexpr float RespawnAboveScreenRange = 260.f;

	// Update() -- a piece despawns once it has fully fallen past the bottom
	// edge, measured in multiples of its own cell size.
	constexpr float DespawnCellSizeMultiple = 4.f;

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
		, randomEngine(std::random_device{}())
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

	void MenuBackdrop::Respawn(Piece& piece, bool isInitial)
	{
		std::uniform_real_distribution<float> unit(0.f, 1.f);
		std::uniform_int_distribution<int> typePick(0, static_cast<int>(ShapeCells.size()) - 1);

		piece.type = typePick(randomEngine);
		piece.cellSize = MinCellSize + unit(randomEngine) * (MaxCellSize - MinCellSize);

		const float depth = (piece.cellSize - MinCellSize) / (MaxCellSize - MinCellSize);   // 0 far .. 1 near
		piece.fallSpeed = MinFallSpeed + depth * FallSpeedByDepth;
		piece.alpha = MinAlpha + depth * (MaxAlpha - MinAlpha);
		piece.angularVelocity = (unit(randomEngine) * 2.f - 1.f) * MaxSpin;
		piece.angleDegrees = unit(randomEngine) * 360.f;

		const float x = -SpawnMarginX + unit(randomEngine) * (VirtualSize.x + 2.f * SpawnMarginX);
		const float y = isInitial
			? unit(randomEngine) * (VirtualSize.y + InitialSpawnYPad) - InitialSpawnYOffset
			: -RespawnAboveScreenMin - unit(randomEngine) * RespawnAboveScreenRange;
		piece.position = { x, y };
	}

	void MenuBackdrop::Push(float direction)
	{
		driftVelocity = std::clamp(driftVelocity + direction * PushImpulse, -MaxDrift, MaxDrift);
	}

	void MenuBackdrop::Update(float deltaTime)
	{
		driftVelocity *= std::exp(-DriftDecay * deltaTime);
		if (std::abs(driftVelocity) < 1.f)
		{
			driftVelocity = 0.f;
		}

		for (Piece& piece : pieces)
		{
			piece.position.y += piece.fallSpeed * deltaTime;
			piece.position.x += driftVelocity * deltaTime;
			piece.angleDegrees += piece.angularVelocity * deltaTime;

			// Wrap sideways so a shove can never sweep the field clean.
			if (piece.position.x < -WrapMargin)
			{
				piece.position.x += VirtualSize.x + 2.f * WrapMargin;
			}
			else if (piece.position.x > VirtualSize.x + WrapMargin)
			{
				piece.position.x -= VirtualSize.x + 2.f * WrapMargin;
			}

			if (piece.position.y - DespawnCellSizeMultiple * piece.cellSize > VirtualSize.y)
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
