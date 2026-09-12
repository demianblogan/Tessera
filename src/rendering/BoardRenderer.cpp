#include "BoardRenderer.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <utility>
#include <vector>

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Sprite.hpp>

#include "../config/HapticSettings.h"
#include "../core/Context.h"
#include "../gameplay/Board.h"
#include "../gameplay/GameplaySession.h"
#include "../gameplay/Tetromino.h"
#include "../gameplay/TetrominoShapes.h"
#include "EffectsController.h"
#include "NeonGlow.h"
#include "../ui/Easing.h"

namespace
{
	// A per-cell head start into the crumble, [0, 0.4), so the stack falls apart
	// unevenly rather than as one slab.
	[[nodiscard]] float CellCrumbleDelay(int x, int y)
	{
		const float noise = std::sin(x * 12.9898f + y * 4.1414f) * 43758.5453f;
		return (noise - std::floor(noise)) * 0.4f;
	}

	// Screen Y of a grid row's top edge. The hidden buffer rows sit above the
	// board, so only rows from Board::BufferHeight down are actually on screen.
	[[nodiscard]] float RowTop(int gridY)
	{
		return BoardRenderer::BoardPosition.y
			+ static_cast<float>(gridY - Board::BufferHeight) * BoardRenderer::BlockSize;
	}
}

BoardRenderer::BoardRenderer(Context& context)
	: context(context)
	, goldenGlow(context.shaders.Get(Assets::ShaderID::NeonDilate), context.shaders.Get(Assets::ShaderID::NeonBlur))
{
	// No code
}

void BoardRenderer::Update(float deltaTime, const GameplaySession& session)
{
	goldenGlow.Update(deltaTime);

	const int spawnCount = session.GetSpawnCount();

	if (previousSpawnCount && *previousSpawnCount != spawnCount)
	{
		nextSlideProgress = 0.f;
	}
	previousSpawnCount = spawnCount;

	if (nextSlideProgress < 1.f)
	{
		nextSlideProgress = std::min(1.f, nextSlideProgress + deltaTime / NextSlideDuration);
	}

	if (outgoingFlight)
	{
		outgoingFlight->timer += deltaTime;
		if (outgoingFlight->timer >= HoldFlightDuration)
		{
			outgoingFlight.reset();
		}
	}

	if (incomingFlight)
	{
		incomingFlight->timer += deltaTime;
		if (incomingFlight->timer >= HoldFlightDuration)
		{
			incomingFlight.reset();
		}
	}
}

void BoardRenderer::Render(sf::RenderTarget& target, const GameplaySession& session, const EffectsController& effects,
	NeonGlow& glow, float deathProgress) const
{
	sf::Sprite blockSprite(context.textures.Get(Assets::TextureID::BlockSpritesheetWithOutline));

	blockSprite.setScale({ BlockSize / 16.f, BlockSize / 16.f });

	// =====================================================
	// Board background tiles
	// =====================================================

	blockSprite.setTextureRect(
		{
			{ WallTextureIndex * SpriteSize, 0 },
			{ SpriteSize, SpriteSize }
		}
	);

	for (int y = Board::BufferHeight; y < Board::HEIGHT; y++)
	{
		const float t = static_cast<float>(y - Board::BufferHeight) / (Board::VisibleHeight - 1);
		const auto brightness = static_cast<std::uint8_t>(6 + t * 18);

		blockSprite.setColor(sf::Color(
			static_cast<std::uint8_t>(brightness / 2),
			brightness,
			static_cast<std::uint8_t>(brightness + 20)
		));

		for (int x = 0; x < Board::WIDTH; x++)
		{
			blockSprite.setPosition(
				{
					BoardPosition.x + x * BlockSize,
					RowTop(y)
				}
			);

			target.draw(blockSprite);
		}
	}

	blockSprite.setColor(sf::Color::White);

	// =====================================================
	// Walls
	// =====================================================

	blockSprite.setTextureRect(
		{
			{ WallTextureIndex * SpriteSize, 0 },
			{ SpriteSize, SpriteSize }
		}
	);

	for (int y = Board::BufferHeight; y < Board::HEIGHT; y++)
	{
		blockSprite.setPosition(
			{
				BoardPosition.x - BlockSize,
				RowTop(y)
			}
		);
		target.draw(blockSprite);

		blockSprite.setPosition(
			{
				BoardPosition.x + Board::WIDTH * BlockSize,
				RowTop(y)
			}
		);
		target.draw(blockSprite);
	}

	for (int x = -1; x <= Board::WIDTH; x++)
	{
		blockSprite.setPosition(
			{
				BoardPosition.x + x * BlockSize,
				RowTop(Board::HEIGHT)
			}
		);
		target.draw(blockSprite);
	}

	// =====================================================
	// Locked cells
	// =====================================================

	const Board::Grid& grid = session.GetBoard().GetGrid();

	// A golden lock (see EscalationDirector) gets a standing pulsing halo, drawn
	// once behind every such cell before the crisp pass below -- one shared
	// glow call over a fixed area (the whole visible field) rather than one per
	// cell, so this never triggers NeonGlow's resize cost as cells move/clear.
	if (deathProgress <= 0.f)
	{
		std::vector<sf::Vector2i> goldenCells;
		for (int y = Board::BufferHeight; y < Board::HEIGHT; y++)
		{
			for (int x = 0; x < Board::WIDTH; x++)
			{
				if (grid[y][x].occupied && grid[y][x].kind == Cell::Kind::Golden)
				{
					goldenCells.push_back({ x, y });
				}
			}
		}

		if (!goldenCells.empty())
		{
			const sf::FloatRect boardArea{
				BoardPosition,
				{ Board::WIDTH * BlockSize, Board::VisibleHeight * BlockSize }
			};

			goldenGlow.Draw(target, boardArea,
				[&](sf::RenderTarget& buffer, const sf::RenderStates& states)
				{
					sf::Sprite goldSprite(context.textures.Get(Assets::TextureID::BlockSpritesheetWithOutline));
					goldSprite.setScale({ BlockSize / 16.f, BlockSize / 16.f });
					goldSprite.setTextureRect({ { WallTextureIndex * SpriteSize, 0 }, { SpriteSize, SpriteSize } });

					for (const sf::Vector2i& cellPos : goldenCells)
					{
						goldSprite.setPosition({ BoardPosition.x + cellPos.x * BlockSize, RowTop(cellPos.y) });
						buffer.draw(goldSprite, states);
					}
				},
				sf::Color(255, 200, 60));
		}
	}

	for (int y = Board::BufferHeight; y < Board::HEIGHT; y++)
	{
		for (int x = 0; x < Board::WIDTH; x++)
		{
			const Cell& cell = grid[y][x];

			if (!cell.occupied)
			{
				continue;
			}

			// A garbage row (see EscalationDirector) borrows the wall tile instead
			// of a tetromino colour, so it reads as structural rather than a piece
			// once darkened below -- a normal lock never looks like this.
			const bool isGarbage = cell.kind == Cell::Kind::Garbage;
			const int textureIndex = isGarbage ? WallTextureIndex : static_cast<int>(cell.tetrominoType);

			blockSprite.setTextureRect(
				{
					{ textureIndex * SpriteSize, 0 },
					{ SpriteSize, SpriteSize }
				}
			);

			if (deathProgress > 0.f)
			{
				const float jitter = CellCrumbleDelay(x, y);
				const float local = std::clamp((deathProgress - jitter) / std::max(0.05f, 1.f - jitter), 0.f, 1.f);

				blockSprite.setPosition(
					{
						BoardPosition.x + x * BlockSize,
						RowTop(y) + local * local * 1200.f
					}
				);

				const auto grey = static_cast<std::uint8_t>(255.f - 100.f * deathProgress);
				blockSprite.setColor(sf::Color(grey, grey, grey, static_cast<std::uint8_t>((1.f - local) * 255.f)));
			}
			else
			{
				blockSprite.setPosition(
					{
						BoardPosition.x + x * BlockSize,
						RowTop(y)
					}
				);

				// Escalation cells (see EscalationDirector) read distinctly from a
				// normal lock: garbage is the wall tile, darkened well below any
				// piece colour; a golden lock pulses warm (see below).
				if (isGarbage)
				{
					blockSprite.setColor(sf::Color(60, 62, 72));
				}
				else if (cell.kind == Cell::Kind::Golden)
				{
					const float pulse = 0.5f + 0.5f * std::sin(context.totalTime * 6.f);
					blockSprite.setColor(sf::Color(255, static_cast<std::uint8_t>(190.f + pulse * 60.f), 60));
				}
				else
				{
					blockSprite.setColor(sf::Color::White);
				}
			}

			target.draw(blockSprite);
		}
	}

	blockSprite.setColor(sf::Color::White);

	// =====================================================
	// Row-clear flash / sweep
	// =====================================================

	for (const EffectsController::RowClearEffect& effect : effects.GetRowClearEffects())
	{
		const float t = effect.timer / EffectsController::RowClearDuration;
		const auto alpha = static_cast<std::uint8_t>((1.f - t) * 255.f);

		sf::RectangleShape flash;
		flash.setPosition(
			{
				BoardPosition.x,
				RowTop(effect.row)
			}
		);
		flash.setSize({ Board::WIDTH * BlockSize, BlockSize });
		flash.setFillColor(sf::Color(120, 220, 255, alpha));
		target.draw(flash);

		const float sweepWidth = 120.f;
		const float sweepX = -sweepWidth + t * (Board::WIDTH * BlockSize + sweepWidth * 2.f);

		sf::RectangleShape sweep;
		sweep.setPosition(
			{
				BoardPosition.x + sweepX,
				RowTop(effect.row)
			}
		);
		sweep.setSize({ sweepWidth, BlockSize });
		sweep.setFillColor(sf::Color(180, 255, 255, alpha));
		target.draw(sweep);
	}

	// =====================================================
	// Ghost  (hidden once the piece is locked and rows are clearing)
	// =====================================================

	if (ghostEnabled && session.IsFalling())
	{
		const Tetromino ghostTetromino = session.GetGhostTetromino();

		blockSprite.setTextureRect(
			{
				{ static_cast<int>(ghostTetromino.GetType()) * SpriteSize, 0 },
				{ SpriteSize, SpriteSize }
			}
		);

		sf::Shader& ghostShader = context.shaders.Get(Assets::ShaderID::GhostTetromino);
		ghostShader.setUniform("time", context.totalTime);

		for (const sf::Vector2i& blockPosition : ghostTetromino.GetBlockPositions())
		{
			if (blockPosition.y < Board::BufferHeight)
			{
				continue;
			}

			blockSprite.setPosition(
				{
					BoardPosition.x + blockPosition.x * BlockSize,
					RowTop(blockPosition.y)
				}
			);

			target.draw(blockSprite, &ghostShader);
		}
	}

	// =====================================================
	// Landing flash
	// =====================================================

	if (effects.HasLandingFlash())
	{
		const float alpha = effects.GetLandingFlashProgress();

		blockSprite.setColor(sf::Color(255, 255, 255, static_cast<std::uint8_t>(alpha * 120.f)));

		for (const sf::Vector2i& blockPosition : effects.GetLandingFlashBlocks())
		{
			if (blockPosition.y < Board::BufferHeight)
			{
				continue;
			}

			blockSprite.setPosition(
				{
					BoardPosition.x + blockPosition.x * BlockSize,
					RowTop(blockPosition.y)
				}
			);

			target.draw(blockSprite);
		}

		blockSprite.setColor(sf::Color::White);
	}

	// =====================================================
	// Active piece  (neon bloom pass, then crisp normal pass)
	// =====================================================

	if (session.IsFalling())
	{
		const Tetromino& piece = session.GetCurrentTetromino();
		const auto blockPositions = piece.GetBlockPositions();
		const sf::IntRect pieceTextureRect{ { static_cast<int>(piece.GetType()) * SpriteSize, 0 }, { SpriteSize, SpriteSize } };

		// Bounding box of the piece in board pixels, for the bloom buffer. Rows in
		// the hidden buffer are clipped off the top.
		int minX = Board::WIDTH;
		int minY = Board::HEIGHT;
		int maxX = 0;
		int maxY = 0;

		for (const sf::Vector2i& blockPosition : blockPositions)
		{
			minX = std::min(minX, blockPosition.x);
			minY = std::min(minY, blockPosition.y);
			maxX = std::max(maxX, blockPosition.x);
			maxY = std::max(maxY, blockPosition.y);
		}

		const int visibleMinY = std::max(minY, Board::BufferHeight);

		// An escalation Chaos-tier bonus piece glows gold instead of the usual
		// cyan accent, so it reads as special the instant it spawns.
		const bool isGolden = session.IsCurrentPieceGolden();
		const HapticSettings::Colour& glowColour = context.hapticSettings.activePieceGlow;
		const sf::Color neonTint = isGolden ? sf::Color(255, 210, 60) : sf::Color(glowColour.r, glowColour.g, glowColour.b);

		if (maxY >= Board::BufferHeight)
		{
			const sf::FloatRect pieceArea{
				{ BoardPosition.x + minX * BlockSize, RowTop(visibleMinY) },
				{ (maxX - minX + 1) * BlockSize, (maxY - visibleMinY + 1) * BlockSize }
			};

			glow.Draw(target, pieceArea,
				[&](sf::RenderTarget& buffer, const sf::RenderStates& states)
				{
					sf::Sprite pieceSprite(context.textures.Get(Assets::TextureID::BlockSpritesheetWithOutline));
					pieceSprite.setTextureRect(pieceTextureRect);
					pieceSprite.setScale({ BlockSize / 16.f, BlockSize / 16.f });

					for (const sf::Vector2i& blockPosition : blockPositions)
					{
						if (blockPosition.y < Board::BufferHeight)
						{
							continue;
						}

						pieceSprite.setPosition(
							{
								BoardPosition.x + blockPosition.x * BlockSize,
								RowTop(blockPosition.y)
							}
						);

						buffer.draw(pieceSprite, states);
					}
				},
				neonTint);
		}

		blockSprite.setTextureRect(pieceTextureRect);
		blockSprite.setScale({ BlockSize / 16.f, BlockSize / 16.f });
		blockSprite.setColor(isGolden ? sf::Color(255, 215, 80) : sf::Color::White);

		for (const sf::Vector2i& blockPosition : blockPositions)
		{
			if (blockPosition.y < Board::BufferHeight)
			{
				continue;
			}

			blockSprite.setPosition(
				{
					BoardPosition.x + blockPosition.x * BlockSize,
					RowTop(blockPosition.y)
				}
			);

			target.draw(blockSprite);
		}
	}
}

float BoardRenderer::NextSlotCentreY(sf::FloatRect area, int slot)
{
	if (slot <= 0)
	{
		return area.position.y + NextHeroSlotHeight * 0.5f;
	}

	return area.position.y + NextHeroSlotHeight
		+ static_cast<float>(slot - 1) * NextRestSlotHeight + NextRestSlotHeight * 0.5f;
}

float BoardRenderer::NextSlotBlockSize(int slot)
{
	return slot <= 0 ? NextHeroBlockSize : NextRestBlockSize;
}

sf::Color BoardRenderer::NextSlotTint(int slot, int count)
{
	if (slot <= 0 || count <= 2)
	{
		return slot <= 0 ? sf::Color::White : sf::Color(NextMinBrightness, NextMinBrightness, NextMinBrightness);
	}

	// Slot 1 (the piece right after the hero) stays bright; it fades toward
	// NextMinBrightness by the last slot.
	const float t = static_cast<float>(slot - 1) / static_cast<float>(count - 2);
	const auto level = static_cast<std::uint8_t>(
		UI::Easing::Lerp(255.f, static_cast<float>(NextMinBrightness), t));
	return sf::Color(level, level, level);
}

void BoardRenderer::RenderNextPreview(sf::RenderTarget& target, const GameplaySession& session, sf::FloatRect area) const
{
	const int count = session.GetNextCount();
	if (count <= 0)
	{
		return;
	}

	const float centreX = area.position.x + area.size.x * 0.5f;
	const float ease = UI::Easing::EaseOutCubic(nextSlideProgress);

	for (int slot = 0; slot < count; ++slot)
	{
		// Mid-slide, every piece is still drawn one slot behind where the queue
		// just put it, and eases into its real slot -- a smooth slide up rather
		// than the queue snapping into place.
		const float y = UI::Easing::Lerp(NextSlotCentreY(area, slot + 1), NextSlotCentreY(area, slot), ease);
		const float blockSize = UI::Easing::Lerp(
			static_cast<float>(NextSlotBlockSize(slot + 1)), static_cast<float>(NextSlotBlockSize(slot)), ease);

		DrawPiecePreview(target, session.GetNextPiece(slot), blockSize, { centreX, y }, NextSlotTint(slot, count));
	}
}

void BoardRenderer::RenderHoldPreview(sf::RenderTarget& target, const GameplaySession& session, sf::FloatRect area) const
{
	// A flight animation is standing in for the HOLD box's contents right now.
	if (IsHoldFlightActive())
	{
		return;
	}

	if (!session.HasHeldPiece())
	{
		return;
	}

	const sf::Color tint = session.CanHold()
		? sf::Color::White
		: sf::Color(NextMinBrightness, NextMinBrightness, NextMinBrightness);

	const sf::Vector2f centre{ area.position.x + area.size.x * 0.5f, area.position.y + area.size.y * 0.5f };

	DrawPiecePreview(target, session.GetHeldPiece(), NextHeroBlockSize, centre, tint);
}

void BoardRenderer::RenderHoldFlight(sf::RenderTarget& target) const
{
	const auto drawFlight = [&](const std::optional<PieceFlight>& flight)
	{
		if (!flight)
		{
			return;
		}

		const float t = UI::Easing::EaseOutCubic(std::min(flight->timer / HoldFlightDuration, 1.f));
		const sf::Vector2f centre = UI::Easing::Lerp(flight->fromCentre, flight->toCentre, t);
		const float blockSize = UI::Easing::Lerp(flight->fromBlockSize, flight->toBlockSize, t);

		DrawPiecePreview(target, Tetromino(flight->type, { 0, 0 }), blockSize, centre);
	};

	drawFlight(outgoingFlight);
	drawFlight(incomingFlight);
}

void BoardRenderer::TriggerHoldSwap(const Tetromino& outgoingPiece, std::optional<Tetromino> incomingPiece,
	sf::FloatRect holdBoxArea)
{
	const sf::Vector2f holdCentre{
		holdBoxArea.position.x + holdBoxArea.size.x * 0.5f,
		holdBoxArea.position.y + holdBoxArea.size.y * 0.5f };

	outgoingFlight = PieceFlight{
		outgoingPiece.GetType(),
		BoardSpaceCentre(outgoingPiece), BlockSize,
		holdCentre, NextHeroBlockSize,
		0.f };

	if (incomingPiece)
	{
		incomingFlight = PieceFlight{
			incomingPiece->GetType(),
			holdCentre, NextHeroBlockSize,
			BoardSpaceCentre(*incomingPiece), BlockSize,
			0.f };
	}
	else
	{
		incomingFlight.reset();
	}
}

sf::Vector2f BoardRenderer::BoardSpaceCentre(const Tetromino& piece)
{
	const auto blocks = piece.GetBlockPositions();

	int minX = Board::WIDTH;
	int maxX = 0;
	int minY = Board::HEIGHT;
	int maxY = 0;

	for (const sf::Vector2i& block : blocks)
	{
		minX = std::min(minX, block.x);
		maxX = std::max(maxX, block.x);
		minY = std::min(minY, block.y);
		maxY = std::max(maxY, block.y);
	}

	return {
		BoardPosition.x + static_cast<float>(minX + maxX + 1) * 0.5f * BlockSize,
		RowTop(minY) + static_cast<float>(maxY - minY + 1) * 0.5f * BlockSize
	};
}

void BoardRenderer::DrawPiecePreview(sf::RenderTarget& target, const Tetromino& piece,
	float blockSize, sf::Vector2f centre, sf::Color tint) const
{
	sf::Sprite blockSprite(context.textures.Get(Assets::TextureID::BlockSpritesheetWithOutline));

	blockSprite.setTextureRect(
		{
			{ static_cast<int>(piece.GetType()) * SpriteSize, 0 },
			{ SpriteSize, SpriteSize }
		}
	);
	blockSprite.setScale({ blockSize / 16.f, blockSize / 16.f });
	blockSprite.setColor(tint);

	const auto blockPositions = piece.GetBlockPositions();

	// Different pieces fill different cells of the 4x4 shape matrix, so centre the
	// piece's own bounding box on `centre` rather than pinning its top-left there.
	int minBlockX = TetrominoShapes::MATRIX_SIZE;
	int maxBlockX = -1;
	int minBlockY = TetrominoShapes::MATRIX_SIZE;
	int maxBlockY = -1;

	for (const sf::Vector2i& blockPosition : blockPositions)
	{
		minBlockX = std::min(minBlockX, blockPosition.x);
		maxBlockX = std::max(maxBlockX, blockPosition.x);
		minBlockY = std::min(minBlockY, blockPosition.y);
		maxBlockY = std::max(maxBlockY, blockPosition.y);
	}

	const sf::Vector2f origin =
	{
		centre.x - static_cast<float>(minBlockX + maxBlockX + 1) * 0.5f * blockSize,
		centre.y - static_cast<float>(minBlockY + maxBlockY + 1) * 0.5f * blockSize
	};

	for (const sf::Vector2i& blockPosition : blockPositions)
	{
		blockSprite.setPosition(
			{
				origin.x + static_cast<float>(blockPosition.x) * blockSize,
				origin.y + static_cast<float>(blockPosition.y) * blockSize
			}
		);

		target.draw(blockSprite);
	}
}
