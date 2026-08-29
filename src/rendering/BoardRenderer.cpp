#include "BoardRenderer.h"

#include <algorithm>
#include <cstdint>
#include <utility>

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Sprite.hpp>

#include "../core/Context.h"
#include "../gameplay/Board.h"
#include "../gameplay/GameplaySession.h"
#include "../gameplay/Tetromino.h"
#include "../gameplay/TetrominoShapes.h"
#include "../settings/GameSettings.h"
#include "../settings/SettingsManager.h"
#include "EffectsController.h"
#include "NeonGlow.h"

namespace
{
	// The neon halo colour for the active piece; matches the game's cyan accent.
	const sf::Color NeonTint(120, 210, 255);
}

BoardRenderer::BoardRenderer(Context& context)
	: context(context)
{
	// No code
}

Assets::TextureID BoardRenderer::ResolveBlockTexture() const
{
	switch (context.settings.GetSettings().blockRenderStyle)
	{
	case BlockRenderStyle::WithOutline:
		return Assets::TextureID::BlockSpritesheetWithOutline;

	case BlockRenderStyle::WithoutOutline:
		return Assets::TextureID::BlockSpritesheetWithoutOutline;
	}

	std::unreachable();
}

void BoardRenderer::Render(sf::RenderTarget& target, const GameplaySession& session, const EffectsController& effects,
	NeonGlow& glow) const
{
	sf::Sprite blockSprite(context.textures.Get(ResolveBlockTexture()));

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

	for (int y = 0; y < Board::HEIGHT; y++)
	{
		const float t = static_cast<float>(y) / (Board::HEIGHT - 1);
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
					BoardPosition.y + y * BlockSize
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

	for (int y = 0; y < Board::HEIGHT; y++)
	{
		blockSprite.setPosition(
			{
				BoardPosition.x - BlockSize,
				BoardPosition.y + y * BlockSize
			}
		);
		target.draw(blockSprite);

		blockSprite.setPosition(
			{
				BoardPosition.x + Board::WIDTH * BlockSize,
				BoardPosition.y + y * BlockSize
			}
		);
		target.draw(blockSprite);
	}

	for (int x = -1; x <= Board::WIDTH; x++)
	{
		blockSprite.setPosition(
			{
				BoardPosition.x + x * BlockSize,
				BoardPosition.y + Board::HEIGHT * BlockSize
			}
		);
		target.draw(blockSprite);
	}

	// =====================================================
	// Locked cells
	// =====================================================

	const Board::Grid& grid = session.GetBoard().GetGrid();

	for (int y = 0; y < Board::HEIGHT; y++)
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

			blockSprite.setPosition(
				{
					BoardPosition.x + x * BlockSize,
					BoardPosition.y + y * BlockSize
				}
			);

			target.draw(blockSprite);
		}
	}

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
				BoardPosition.y + effect.row * BlockSize
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
				BoardPosition.y + effect.row * BlockSize
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
			blockSprite.setPosition(
				{
					BoardPosition.x + blockPosition.x * BlockSize,
					BoardPosition.y + blockPosition.y * BlockSize
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
			blockSprite.setPosition(
				{
					BoardPosition.x + blockPosition.x * BlockSize,
					BoardPosition.y + blockPosition.y * BlockSize
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

		// Bounding box of the piece in board pixels, for the bloom buffer.
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

		const sf::FloatRect pieceArea{
			{ BoardPosition.x + minX * BlockSize, BoardPosition.y + minY * BlockSize },
			{ (maxX - minX + 1) * BlockSize, (maxY - minY + 1) * BlockSize }
		};

		glow.Draw(target, pieceArea,
			[&](sf::RenderTarget& buffer, const sf::RenderStates& states)
			{
				sf::Sprite pieceSprite(context.textures.Get(ResolveBlockTexture()));
				pieceSprite.setTextureRect(pieceTextureRect);
				pieceSprite.setScale({ BlockSize / 16.f, BlockSize / 16.f });

				for (const sf::Vector2i& blockPosition : blockPositions)
				{
					pieceSprite.setPosition(
						{
							BoardPosition.x + blockPosition.x * BlockSize,
							BoardPosition.y + blockPosition.y * BlockSize
						}
					);

					buffer.draw(pieceSprite, states);
				}
			},
			NeonTint);

		blockSprite.setTextureRect(pieceTextureRect);
		blockSprite.setScale({ BlockSize / 16.f, BlockSize / 16.f });
		blockSprite.setColor(sf::Color::White);

		for (const sf::Vector2i& blockPosition : blockPositions)
		{
			blockSprite.setPosition(
				{
					BoardPosition.x + blockPosition.x * BlockSize,
					BoardPosition.y + blockPosition.y * BlockSize
				}
			);

			target.draw(blockSprite);
		}
	}
}

void BoardRenderer::RenderNextPreview(sf::RenderTarget& target, const GameplaySession& session, sf::Vector2f centre) const
{
	sf::Sprite blockSprite(context.textures.Get(ResolveBlockTexture()));

	const auto previewBlockPositions = session.GetNextTetromino().GetBlockPositions();

	blockSprite.setTextureRect(
		{
			{ static_cast<int>(session.GetNextTetromino().GetType()) * SpriteSize, 0 },
			{ SpriteSize, SpriteSize }
		}
	);

	blockSprite.setScale({ PreviewBlockSize / 16.f, PreviewBlockSize / 16.f });

	// Different pieces occupy different cells of the 4x4 shape matrix, so centre
	// the piece's own bounding box on `centre` instead of pinning its top-left
	// corner there.
	int minBlockX = TetrominoShapes::MATRIX_SIZE;
	int maxBlockX = -1;
	int minBlockY = TetrominoShapes::MATRIX_SIZE;
	int maxBlockY = -1;

	for (const sf::Vector2i& blockPosition : previewBlockPositions)
	{
		minBlockX = std::min(minBlockX, blockPosition.x);
		maxBlockX = std::max(maxBlockX, blockPosition.x);
		minBlockY = std::min(minBlockY, blockPosition.y);
		maxBlockY = std::max(maxBlockY, blockPosition.y);
	}

	const sf::Vector2f previewOrigin =
	{
		centre.x - (minBlockX + maxBlockX + 1) * 0.5f * PreviewBlockSize,
		centre.y - (minBlockY + maxBlockY + 1) * 0.5f * PreviewBlockSize
	};

	for (const sf::Vector2i& blockPosition : previewBlockPositions)
	{
		blockSprite.setPosition(
			{
				previewOrigin.x + blockPosition.x * PreviewBlockSize,
				previewOrigin.y + blockPosition.y * PreviewBlockSize
			}
		);

		target.draw(blockSprite);
	}
}
