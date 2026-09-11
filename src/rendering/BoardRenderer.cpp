#include "BoardRenderer.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <utility>

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
{
	// No code
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

	for (int y = Board::BufferHeight; y < Board::HEIGHT; y++)
	{
		for (int x = 0; x < Board::WIDTH; x++)
		{
			const Cell& cell = grid[y][x];

			if (!cell.occupied)
			{
				continue;
			}

			blockSprite.setTextureRect(
				{
					{ static_cast<int>(cell.tetrominoType) * SpriteSize, 0 },
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

	if (session.IsFalling())
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

		// The neon halo colour for the active piece; matches the game's cyan accent.
		const HapticSettings::Colour& glowColour = context.hapticSettings.activePieceGlow;
		const sf::Color neonTint(glowColour.r, glowColour.g, glowColour.b);

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
		blockSprite.setColor(sf::Color::White);

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

void BoardRenderer::RenderNextPreview(sf::RenderTarget& target, const GameplaySession& session, sf::FloatRect area) const
{
	const int count = session.GetNextCount();
	if (count <= 0)
	{
		return;
	}

	// A vertical stack filling `area`: the piece that spawns next on top, the
	// rest below it in equal slots, so this keeps working as the queue length
	// (currently fixed at 5) becomes a player setting.
	const float centreX = area.position.x + area.size.x * 0.5f;
	const float slotStride = area.size.y / static_cast<float>(count);
	const float firstY = area.position.y + slotStride * 0.5f;

	for (int i = 0; i < count; ++i)
	{
		DrawPiecePreview(target, session.GetNextPiece(i), NextBlockSize,
			{ centreX, firstY + slotStride * static_cast<float>(i) });
	}
}

void BoardRenderer::DrawPiecePreview(sf::RenderTarget& target, const Tetromino& piece,
	float blockSize, sf::Vector2f centre) const
{
	sf::Sprite blockSprite(context.textures.Get(Assets::TextureID::BlockSpritesheetWithOutline));

	blockSprite.setTextureRect(
		{
			{ static_cast<int>(piece.GetType()) * SpriteSize, 0 },
			{ SpriteSize, SpriteSize }
		}
	);
	blockSprite.setScale({ blockSize / 16.f, blockSize / 16.f });

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
