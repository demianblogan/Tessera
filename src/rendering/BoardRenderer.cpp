#include "BoardRenderer.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <utility>
#include <vector>

#include <SFML/Graphics/BlendMode.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/System/Angle.hpp>

#include "../core/Context.h"
#include "../gameplay/Board.h"
#include "../gameplay/GameplaySession.h"
#include "../gameplay/Tetromino.h"
#include "../gameplay/TetrominoShapes.h"
#include "../haptics/HapticSettings.h"
#include "EffectsController.h"
#include "../primitives/NeonGlow.h"
#include "../utils/Easing.h"

namespace
{
	// CellCrumbleDelay: the canonical GLSL "fake random" magic constants (a
	// fast, deterministic pseudo-noise from a cell's coordinates -- the same
	// trick countless shaders use), plus how much of a head start it spreads
	// crumbling cells across.
	constexpr float CrumbleNoiseFrequencyX = 12.9898f;
	constexpr float CrumbleNoiseFrequencyY = 4.1414f;
	constexpr float CrumbleNoiseScale = 43758.5453f;
	constexpr float CrumbleDelayRange = 0.4f;

	// A per-cell head start into the crumble, [0, CrumbleDelayRange), so the
	// stack falls apart unevenly rather than as one slab.
	[[nodiscard]] float CellCrumbleDelay(int x, int y)
	{
		const float noise = std::sin(x * CrumbleNoiseFrequencyX + y * CrumbleNoiseFrequencyY) * CrumbleNoiseScale;
		return (noise - std::floor(noise)) * CrumbleDelayRange;
	}

	// Screen Y of a grid row's top edge. The hidden buffer rows sit above the
	// board, so only rows from Board::BufferHeight down are actually on screen.
	[[nodiscard]] float RowTop(int gridY)
	{
		return BoardRenderer::BoardPosition.y + static_cast<float>(gridY - Board::BufferHeight) * BoardRenderer::BlockSize;
	}

	[[nodiscard]] sf::Color LerpColor(sf::Color a, sf::Color b, float t)
	{
		const auto lerp =
			[t](std::uint8_t x, std::uint8_t y)
			{
				return static_cast<std::uint8_t>(static_cast<float>(x) + (static_cast<float>(y) - x) * t);
			};

		return sf::Color(lerp(a.r, b.r), lerp(a.g, b.g), lerp(a.b, b.b));
	}

	// Row-clear flash/sweep color by rank (0 Single .. 1 Tetris): cool cyan
	// climbing to a hot white-gold, so a Tetris reads as visibly hotter than a
	// Single, not just wider.
	[[nodiscard]] sf::Color RowClearFlashColor(float rankT)
	{
		return LerpColor(sf::Color(120, 220, 255), sf::Color(255, 235, 160), rankT);
	}

	[[nodiscard]] sf::Color RowClearSweepColor(float rankT)
	{
		return LerpColor(sf::Color(180, 255, 255), sf::Color(255, 250, 210), rankT);
	}

	// Board background tiles: a subtle blue-toned gradient, brighter near the
	// bottom of the well.
	constexpr std::uint8_t BackgroundMinBrightness = 6;
	constexpr std::uint8_t BackgroundBrightnessRange = 18;
	constexpr std::uint8_t BackgroundBlueBoost = 20;

	const sf::Color GoldenGlowTint{ 255, 200, 60 };

	// Game-over crumble.
	constexpr float DeathCrumbleFallDistance = 1200.f;   // pixels a cell falls once fully crumbled
	constexpr float DeathGreyRange = 100.f;              // how far toward grey a fully-crumbled cell darkens
	constexpr float DeathCrumbleMinSpread = 0.05f;       // floor on a cell's own crumble-progress span

	// Escalation cell tints (see EscalationDirector).
	const sf::Color GarbageCellTint{ 60, 62, 72 };
	constexpr float GoldenCellPulseSpeed = 6.f;
	constexpr float GoldenCellPulseBrightnessBase = 190.f;
	constexpr float GoldenCellPulseBrightnessRange = 60.f;

	// Row-clear flash / sweep.
	constexpr float MaxRowClearRank = 3.f;
	constexpr float RowClearFlashPeakAlphaBase = 200.f;
	constexpr float RowClearFlashPeakAlphaRange = 55.f;
	constexpr float RowClearSweepWidthBase = 120.f;
	constexpr float RowClearSweepWidthRange = 140.f;

	// Perfect Clear wash.
	constexpr float PerfectClearWashMaxAlpha = 130.f;
	const sf::Color PerfectClearWashColor{ 255, 215, 90 };

	// Speed Surge wash.
	constexpr float SpeedSurgeWashMaxAlpha = 60.f;
	const sf::Color SpeedSurgeWashColor{ 255, 60, 50 };

	// Garbage wave.
	constexpr float GarbageWaveBandHeightScale = 1.6f;   // multiple of BlockSize
	constexpr float GarbageWaveAlphaFalloff = 0.3f;      // how much the band fades as it nears the top
	constexpr float GarbageWaveMaxAlpha = 150.f;
	const sf::Color GarbageWaveBandColor{ 255, 80, 60 };
	constexpr float GarbageWaveCoreThickness = 4.f;
	const sf::Color GarbageWaveCoreColor{ 255, 210, 200 };
	constexpr float GarbageWaveCoreAlphaBoost = 1.4f;

	// Combo glow around the well.
	constexpr float MinVisibleComboGlowLevel = 0.02f;
	constexpr float ComboPeakLevel = 5.f;
	constexpr float ComboHotRange = 3.f;               // levels above ComboPeakLevel to fully shift to gold
	const sf::Color ComboColorCold{ 70, 140, 255 };
	const sf::Color ComboColorWarm{ 140, 200, 255 };
	const sf::Color ComboColorHot{ 255, 210, 70 };
	constexpr float ComboGlowPulseBase = 0.55f;
	constexpr float ComboGlowPulseAmplitude = 0.45f;
	constexpr float ComboGlowPulseSpeedBase = 7.f;
	constexpr float ComboGlowPulseSpeedHotBoost = 6.f;
	constexpr float ComboGlowThicknessBase = 8.f;
	constexpr float ComboGlowThicknessRange = 10.f;
	constexpr float ComboGlowPadding = 26.f;
	constexpr float ComboCoreAlphaScale = 220.f;
	constexpr float ComboCoreThicknessBase = 2.f;
	constexpr float ComboCoreThicknessRange = 2.f;

	// Ghost drop-path trail.
	constexpr float GhostTrailWidth = 6.f;
	const sf::Color GhostTrailTopColor{ 190, 225, 255, 80 };
	const sf::Color GhostTrailBottomColor{ 190, 225, 255, 6 };
	constexpr float GhostTrailMinLength = 2.f;   // shorter than this isn't worth drawing

	constexpr float LandingFlashMaxAlpha = 120.f;

	// Active piece glow/tint when it's an escalation Chaos-tier bonus piece.
	const sf::Color GoldenPieceGlowTint{ 255, 210, 60 };
	const sf::Color GoldenPieceBlockTint{ 255, 215, 80 };
}

BoardRenderer::BoardRenderer(Context& context)
	: context(context)
	, goldenGlow(context.shaders.Get(Assets::ShaderID::NeonDilate), context.shaders.Get(Assets::ShaderID::NeonBlur))
	, comboGlow(context.shaders.Get(Assets::ShaderID::NeonDilate), context.shaders.Get(Assets::ShaderID::NeonBlur))
{}

void BoardRenderer::Update(float deltaTime, const GameplaySession& session)
{
	goldenGlow.Update(deltaTime);
	comboGlow.Update(deltaTime);

	const int spawnCount = session.GetSpawnCount();

	if (previousSpawnCount.has_value() && *previousSpawnCount != spawnCount)
		nextSlideProgress = 0.f;
	previousSpawnCount = spawnCount;

	if (nextSlideProgress < 1.f)
		nextSlideProgress = std::min(1.f, nextSlideProgress + deltaTime / NextSlideDuration);

	if (outgoingFlight.has_value())
	{
		outgoingFlight->timer += deltaTime;
		if (outgoingFlight->timer >= HoldFlightDuration)
			outgoingFlight.reset();
	}

	if (incomingFlight.has_value())
	{
		incomingFlight->timer += deltaTime;
		if (incomingFlight->timer >= HoldFlightDuration)
			incomingFlight.reset();
	}

	if (hardDropFlight.has_value())
	{
		hardDropFlight->timer += deltaTime;
		if (hardDropFlight->timer >= HardDropFlightDuration)
			hardDropFlight.reset();
	}
}

bool BoardRenderer::IsHoldFlightActive() const
{
	return outgoingFlight.has_value() || incomingFlight.has_value();
}

void BoardRenderer::SetGhostEnabled(bool isEnabled)
{
	isGhostEnabled = isEnabled;
}

void BoardRenderer::TriggerHardDropFlight(Tetromino::Type type,
	const std::array<sf::Vector2i, TetrominoShapes::BlockCount>& cells, int droppedRows)
{
	if (droppedRows <= 0)
		return;   // already resting -- nothing to slide from

	hardDropFlight = HardDropFlight{ type, cells, droppedRows, 0.f };
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

	for (int y = Board::BufferHeight; y < Board::Height; y++)
	{
		const float t = static_cast<float>(y - Board::BufferHeight) / (Board::VisibleHeight - 1);
		const auto brightness = static_cast<std::uint8_t>(BackgroundMinBrightness + t * BackgroundBrightnessRange);

		blockSprite.setColor(sf::Color(
			static_cast<std::uint8_t>(brightness / 2),
			brightness,
			static_cast<std::uint8_t>(brightness + BackgroundBlueBoost)
		));

		for (int x = 0; x < Board::Width; x++)
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

	for (int y = Board::BufferHeight; y < Board::Height; y++)
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
				BoardPosition.x + Board::Width * BlockSize,
				RowTop(y)
			}
		);
		target.draw(blockSprite);
	}

	for (int x = -1; x <= Board::Width; x++)
	{
		blockSprite.setPosition(
			{
				BoardPosition.x + x * BlockSize,
				RowTop(Board::Height)
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
		for (int y = Board::BufferHeight; y < Board::Height; y++)
			for (int x = 0; x < Board::Width; x++)
				if (grid[y][x].isOccupied && grid[y][x].kind == Cell::Kind::Golden)
					goldenCells.push_back({ x, y });

		if (!goldenCells.empty())
		{
			const sf::FloatRect boardArea
			{
				BoardPosition,
				{ Board::Width * BlockSize, Board::VisibleHeight * BlockSize }
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
				GoldenGlowTint);
		}
	}

	for (int y = Board::BufferHeight; y < Board::Height; y++)
	{
		for (int x = 0; x < Board::Width; x++)
		{
			const Cell& cell = grid[y][x];

			if (!cell.isOccupied)
				continue;

			// A garbage row (see EscalationDirector) borrows the wall tile instead
			// of a tetromino color, so it reads as structural rather than a piece
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
				const float local =
					std::clamp((deathProgress - jitter) / std::max(DeathCrumbleMinSpread, 1.f - jitter), 0.f, 1.f);

				blockSprite.setPosition(
					{
						BoardPosition.x + x * BlockSize,
						RowTop(y) + local * local * DeathCrumbleFallDistance
					}
				);

				const auto grey = static_cast<std::uint8_t>(255.f - DeathGreyRange * deathProgress);
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
				// piece color; a golden lock pulses warm (see below).
				if (isGarbage)
				{
					blockSprite.setColor(GarbageCellTint);
				}
				else if (cell.kind == Cell::Kind::Golden)
				{
					const float pulse = 0.5f + 0.5f * std::sin(context.totalTime * GoldenCellPulseSpeed);
					blockSprite.setColor(sf::Color(255,
						static_cast<std::uint8_t>(GoldenCellPulseBrightnessBase + pulse * GoldenCellPulseBrightnessRange), 60));
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
	// Row-clear flash / sweep -- color, width and peak brightness escalate
	// with rank (0 Single .. 3 Tetris), so a Tetris reads as a hotter, wider
	// event than a Single, not just four of the same thing at once.
	// =====================================================

	for (const EffectsController::RowClearEffect& effect : effects.GetRowClearEffects())
	{
		const float t = effect.timer / EffectsController::RowClearDuration;
		const float rankT = std::clamp(static_cast<float>(effect.rank) / MaxRowClearRank, 0.f, 1.f);

		const sf::Color flashColor = RowClearFlashColor(rankT);
		const sf::Color sweepColor = RowClearSweepColor(rankT);
		const float peakAlpha = RowClearFlashPeakAlphaBase + RowClearFlashPeakAlphaRange * rankT;
		const auto alpha = static_cast<std::uint8_t>((1.f - t) * peakAlpha);

		sf::RectangleShape flash;
		flash.setPosition(
			{
				BoardPosition.x,
				RowTop(effect.row)
			}
		);
		flash.setSize({ Board::Width * BlockSize, BlockSize });
		flash.setFillColor(sf::Color(flashColor.r, flashColor.g, flashColor.b, alpha));
		target.draw(flash);

		const float sweepWidth = RowClearSweepWidthBase + RowClearSweepWidthRange * rankT;
		const float sweepX = -sweepWidth + t * (Board::Width * BlockSize + sweepWidth * 2.f);

		sf::RectangleShape sweep;
		sweep.setPosition(
			{
				BoardPosition.x + sweepX,
				RowTop(effect.row)
			}
		);
		sweep.setSize({ sweepWidth, BlockSize });
		sweep.setFillColor(sf::Color(sweepColor.r, sweepColor.g, sweepColor.b, alpha));
		target.draw(sweep);
	}

	// =====================================================
	// Perfect Clear -- a slow-fading golden wash over the whole board, timed
	// with the shard burst TriggerPerfectClearBurst spawned into GetShards().
	// =====================================================

	if (effects.HasPerfectClearFlash())
	{
		const float progress = effects.GetPerfectClearFlashProgress();
		const auto alpha = static_cast<std::uint8_t>(progress * PerfectClearWashMaxAlpha);

		sf::RectangleShape wash({ Board::Width * BlockSize, Board::VisibleHeight * BlockSize });
		wash.setPosition(BoardPosition);
		wash.setFillColor(sf::Color(PerfectClearWashColor.r, PerfectClearWashColor.g, PerfectClearWashColor.b, alpha));
		target.draw(wash, sf::RenderStates(sf::BlendAdd));
	}

	// =====================================================
	// Speed Surge -- a reddish board-wide wash for the whole surge, so
	// something stays visibly different while it's active, not just at the
	// instant it starts.
	// =====================================================

	if (effects.HasSpeedSurgeGlow())
	{
		const float progress = effects.GetSpeedSurgeGlowProgress();
		const auto alpha = static_cast<std::uint8_t>(progress * SpeedSurgeWashMaxAlpha);

		sf::RectangleShape wash({ Board::Width * BlockSize, Board::VisibleHeight * BlockSize });
		wash.setPosition(BoardPosition);
		wash.setFillColor(sf::Color(SpeedSurgeWashColor.r, SpeedSurgeWashColor.g, SpeedSurgeWashColor.b, alpha));
		target.draw(wash, sf::RenderStates(sf::BlendAdd));
	}

	// =====================================================
	// Garbage wave -- a bright band shockwaves from the bottom of the well
	// (where the new row just shoved everything up) to the top, instead of a
	// glow held for the rest of the run.
	// =====================================================

	if (effects.HasGarbageWave())
	{
		const float progress = effects.GetGarbageWaveProgress();
		const float bandHeight = BlockSize * GarbageWaveBandHeightScale;

		const float bottomY = BoardPosition.y + Board::VisibleHeight * BlockSize + bandHeight * 0.5f;
		const float topY = BoardPosition.y - bandHeight * 0.5f;
		const float centerY = bottomY + progress * (topY - bottomY);

		const auto alpha = static_cast<std::uint8_t>((1.f - progress * GarbageWaveAlphaFalloff) * GarbageWaveMaxAlpha);

		sf::RectangleShape band({ Board::Width * BlockSize, bandHeight });
		band.setOrigin({ 0.f, bandHeight * 0.5f });
		band.setPosition({ BoardPosition.x, centerY });
		band.setFillColor(sf::Color(GarbageWaveBandColor.r, GarbageWaveBandColor.g, GarbageWaveBandColor.b, alpha));
		target.draw(band, sf::RenderStates(sf::BlendAdd));

		sf::RectangleShape core({ Board::Width * BlockSize, GarbageWaveCoreThickness });
		core.setOrigin({ 0.f, GarbageWaveCoreThickness * 0.5f });
		core.setPosition({ BoardPosition.x, centerY });
		core.setFillColor(sf::Color(GarbageWaveCoreColor.r, GarbageWaveCoreColor.g, GarbageWaveCoreColor.b,
			static_cast<std::uint8_t>(std::min(255.f, alpha * GarbageWaveCoreAlphaBoost))));
		target.draw(core, sf::RenderStates(sf::BlendAdd));
	}

	// =====================================================
	// Combo glow -- a real soft neon bloom around the well (not just a flat
	// line) that builds with each clear that directly follows another, and
	// lingers/fades once the chain breaks (EffectsController eases the level
	// itself). Past ComboPeakLevel it shifts from blue to a hot gold and
	// pulses faster, so a long chain keeps escalating rather than plateauing.
	// =====================================================

	if (const float comboLevel = effects.GetComboGlowLevel(); comboLevel > MinVisibleComboGlowLevel)
	{
		const float t = std::clamp(comboLevel / ComboPeakLevel, 0.f, 1.f);
		const float hot = std::clamp((comboLevel - ComboPeakLevel) / ComboHotRange, 0.f, 1.f);

		const sf::Color color = LerpColor(
			LerpColor(ComboColorCold, ComboColorWarm, t),
			ComboColorHot, hot);

		const float pulse =
			ComboGlowPulseBase + ComboGlowPulseAmplitude *
			std::sin(context.totalTime * (ComboGlowPulseSpeedBase + hot * ComboGlowPulseSpeedHotBoost));

		const float thickness = ComboGlowThicknessBase + ComboGlowThicknessRange * t;
		const float glowAlpha = std::clamp((ComboGlowPulseBase + ComboGlowPulseAmplitude * t) * pulse, 0.f, 1.f);

		const sf::FloatRect boardRect{ BoardPosition, { Board::Width * BlockSize, Board::VisibleHeight * BlockSize } };
		const sf::FloatRect glowArea
		{
			{ boardRect.position.x - ComboGlowPadding, boardRect.position.y - ComboGlowPadding },
			{ boardRect.size.x + ComboGlowPadding * 2.f, boardRect.size.y + ComboGlowPadding * 2.f }
		};

		comboGlow.Draw(target, glowArea,
			[&](sf::RenderTarget& buffer, const sf::RenderStates& states)
			{
				sf::RectangleShape edge;
				edge.setFillColor(sf::Color::White);

				edge.setSize({ boardRect.size.x + thickness * 2.f, thickness });
				edge.setPosition({ boardRect.position.x - thickness, boardRect.position.y - thickness });
				buffer.draw(edge, states);

				edge.setPosition({ boardRect.position.x - thickness, boardRect.position.y + boardRect.size.y });
				buffer.draw(edge, states);

				edge.setSize({ thickness, boardRect.size.y + thickness * 2.f });
				edge.setPosition({ boardRect.position.x - thickness, boardRect.position.y - thickness });
				buffer.draw(edge, states);

				edge.setPosition({ boardRect.position.x + boardRect.size.x, boardRect.position.y - thickness });
				buffer.draw(edge, states);
			},
			sf::Color(
				static_cast<std::uint8_t>(color.r * glowAlpha),
				static_cast<std::uint8_t>(color.g * glowAlpha),
				static_cast<std::uint8_t>(color.b * glowAlpha)),
			false);

		// The glow alone reads as soft/hazy at low intensity; a crisp crackling
		// core edge on top of it sells the "electric" energy the further the
		// chain has built.
		sf::RectangleShape core;
		const auto coreAlpha = static_cast<std::uint8_t>(glowAlpha * ComboCoreAlphaScale);
		core.setFillColor(sf::Color(255, 255, 255, coreAlpha));

		const float coreThickness = ComboCoreThicknessBase + ComboCoreThicknessRange * t;
		core.setSize({ boardRect.size.x + coreThickness * 2.f, coreThickness });
		core.setPosition({ boardRect.position.x - coreThickness, boardRect.position.y - coreThickness });
		target.draw(core, sf::RenderStates(sf::BlendAdd));

		core.setPosition({ boardRect.position.x - coreThickness, boardRect.position.y + boardRect.size.y });
		target.draw(core, sf::RenderStates(sf::BlendAdd));

		core.setSize({ coreThickness, boardRect.size.y + coreThickness * 2.f });
		core.setPosition({ boardRect.position.x - coreThickness, boardRect.position.y - coreThickness });
		target.draw(core, sf::RenderStates(sf::BlendAdd));

		core.setPosition({ boardRect.position.x + boardRect.size.x, boardRect.position.y - coreThickness });
		target.draw(core, sf::RenderStates(sf::BlendAdd));
	}

	// =====================================================
	// Ghost trail -- a faint additive beam per column, from the active piece's
	// underside down to the ghost's landing preview, so the drop path reads at
	// a glance instead of two disconnected shapes.
	// =====================================================

	if (isGhostEnabled && session.IsFalling())
	{
		const Tetromino& activePiece = session.GetCurrentTetromino();
		const Tetromino ghostTetromino = session.GetGhostTetromino();

		std::array<int, Board::Width> pieceBottom{};
		std::array<int, Board::Width> ghostTop{};
		pieceBottom.fill(-1);
		ghostTop.fill(Board::Height);

		for (const sf::Vector2i& block : activePiece.GetBlockPositions())
		{
			if (block.x >= 0 && block.x < Board::Width)
			{
				pieceBottom[static_cast<std::size_t>(block.x)] =
					std::max(pieceBottom[static_cast<std::size_t>(block.x)], block.y);
			}
		}
		for (const sf::Vector2i& block : ghostTetromino.GetBlockPositions())
		{
			if (block.x >= 0 && block.x < Board::Width)
			{
				ghostTop[static_cast<std::size_t>(block.x)] =
					std::min(ghostTop[static_cast<std::size_t>(block.x)], block.y);
			}
		}

		for (int x = 0; x < Board::Width; x++)
		{
			const int bottom = pieceBottom[static_cast<std::size_t>(x)];
			const int top = ghostTop[static_cast<std::size_t>(x)];
			if (bottom < 0 || top >= Board::Height || top <= bottom)
				continue;

			const float topY = RowTop(bottom) + BlockSize;
			const float bottomY = RowTop(top);
			if (bottomY <= topY + GhostTrailMinLength)
				continue;

			const float left = BoardPosition.x + static_cast<float>(x) * BlockSize + (BlockSize - GhostTrailWidth) * 0.5f;
			const float right = left + GhostTrailWidth;

			sf::VertexArray beam(sf::PrimitiveType::Triangles, 6);
			beam[0] = { { left, topY }, GhostTrailTopColor };
			beam[1] = { { right, topY }, GhostTrailTopColor };
			beam[2] = { { right, bottomY }, GhostTrailBottomColor };
			beam[3] = { { left, topY }, GhostTrailTopColor };
			beam[4] = { { right, bottomY }, GhostTrailBottomColor };
			beam[5] = { { left, bottomY }, GhostTrailBottomColor };

			target.draw(beam, sf::RenderStates(sf::BlendAdd));
		}
	}

	// =====================================================
	// Ghost  (hidden once the piece is locked and rows are clearing)
	// =====================================================

	if (isGhostEnabled && session.IsFalling())
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
				continue;

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

		blockSprite.setColor(sf::Color(255, 255, 255, static_cast<std::uint8_t>(alpha * LandingFlashMaxAlpha)));

		for (const sf::Vector2i& blockPosition : effects.GetLandingFlashBlocks())
		{
			if (blockPosition.y < Board::BufferHeight)
				continue;

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
		const sf::IntRect pieceTextureRect
		{
			{ static_cast<int>(piece.GetType()) * SpriteSize, 0 },
			{ SpriteSize, SpriteSize }
		};

		// Bounding box of the piece in board pixels, for the bloom buffer. Rows in
		// the hidden buffer are clipped off the top.
		int minX = Board::Width;
		int minY = Board::Height;
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
		const HapticSettings::Color& glowColor = context.hapticSettings.activePieceGlow;
		const sf::Color neonTint = isGolden ? GoldenPieceGlowTint : sf::Color(glowColor.r, glowColor.g, glowColor.b);

		if (maxY >= Board::BufferHeight)
		{
			const sf::FloatRect pieceArea
			{
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
							continue;

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
		blockSprite.setColor(isGolden ? GoldenPieceBlockTint : sf::Color::White);

		for (const sf::Vector2i& blockPosition : blockPositions)
		{
			if (blockPosition.y < Board::BufferHeight)
				continue;

			blockSprite.setPosition(
				{
					BoardPosition.x + blockPosition.x * BlockSize,
					RowTop(blockPosition.y)
				}
			);

			target.draw(blockSprite);
		}
	}

	// =====================================================
	// Hard-drop flight -- the cosmetic slide from where the piece was to where
	// it already (instantly) locked, so the drop doesn't read as a teleport.
	// =====================================================

	if (hardDropFlight.has_value())
	{
		const float t = std::clamp(hardDropFlight->timer / HardDropFlightDuration, 0.f, 1.f);
		const float remaining = 1.f - Easing::EaseOutCubic(t);
		const float offsetY = -remaining * static_cast<float>(hardDropFlight->droppedRows) * BlockSize;

		blockSprite.setTextureRect(
			{
				{ static_cast<int>(hardDropFlight->type) * SpriteSize, 0 },
				{ SpriteSize, SpriteSize }
			}
		);
		blockSprite.setColor(sf::Color::White);

		for (const sf::Vector2i& cell : hardDropFlight->cells)
		{
			if (cell.y < Board::BufferHeight)
				continue;

			blockSprite.setPosition(
				{
					BoardPosition.x + cell.x * BlockSize,
					RowTop(cell.y) + offsetY
				}
			);
			target.draw(blockSprite);
		}
	}

	// =====================================================
	// Shards -- the row-clear shatter, the T-spin swirl, the Perfect Clear
	// burst, impact dust. Fragments of the block spritesheet (textureIndex >=
	// 0) tumble and fade; plain color dots (textureIndex < 0) do the same
	// without a sprite. Drawn last so dust and debris read as being flung out
	// in front of the piece/board, not tucked behind it.
	// =====================================================

	for (const EffectsController::Shard& shard : effects.GetShards())
	{
		const float lifeT = std::clamp(shard.life / shard.maxLife, 0.f, 1.f);
		const auto alpha = static_cast<std::uint8_t>(lifeT * 255.f);

		if (shard.textureIndex >= 0)
		{
			blockSprite.setTextureRect(
				{
					{ shard.textureIndex * SpriteSize, 0 },
					{ SpriteSize, SpriteSize }
				}
			);
			blockSprite.setScale({ BlockSize / 16.f * shard.size, BlockSize / 16.f * shard.size });
			blockSprite.setOrigin({ SpriteSize * 0.5f, SpriteSize * 0.5f });
			blockSprite.setRotation(sf::degrees(shard.rotation));
			blockSprite.setPosition(shard.position);
			blockSprite.setColor(sf::Color(255, 255, 255, alpha));
			target.draw(blockSprite);
		}
		else
		{
			sf::RectangleShape dot({ shard.size * BlockSize, shard.size * BlockSize });
			dot.setOrigin({ dot.getSize().x * 0.5f, dot.getSize().y * 0.5f });
			dot.setRotation(sf::degrees(shard.rotation));
			dot.setPosition(shard.position);
			dot.setFillColor(sf::Color(shard.tint.r, shard.tint.g, shard.tint.b, alpha));
			target.draw(dot, sf::RenderStates(sf::BlendAdd));
		}
	}
}

float BoardRenderer::NextSlotCenterY(sf::FloatRect area, int slot)
{
	if (slot <= 0)
		return area.position.y + NextHeroSlotHeight * 0.5f;

	return area.position.y + NextHeroSlotHeight + static_cast<float>(slot - 1) * NextRestSlotHeight + NextRestSlotHeight * 0.5f;
}

float BoardRenderer::NextSlotBlockSize(int slot)
{
	return slot <= 0 ? NextHeroBlockSize : NextRestBlockSize;
}

sf::Color BoardRenderer::NextSlotTint(int slot, int count)
{
	if (slot <= 0 || count <= 2)
		return slot <= 0 ? sf::Color::White : sf::Color(NextMinBrightness, NextMinBrightness, NextMinBrightness);

	// Slot 1 (the piece right after the hero) stays bright; it fades toward
	// NextMinBrightness by the last slot.
	const float t = static_cast<float>(slot - 1) / static_cast<float>(count - 2);
	const auto level = static_cast<std::uint8_t>(Easing::Lerp(255.f, static_cast<float>(NextMinBrightness), t));

	return sf::Color(level, level, level);
}

void BoardRenderer::RenderNextPreview(sf::RenderTarget& target, const GameplaySession& session, sf::FloatRect area) const
{
	const int count = session.GetNextCount();
	if (count <= 0)
		return;

	const float centerX = area.position.x + area.size.x * 0.5f;
	const float ease = Easing::EaseOutCubic(nextSlideProgress);

	for (int slot = 0; slot < count; slot++)
	{
		// Mid-slide, every piece is still drawn one slot behind where the queue
		// just put it, and eases into its real slot -- a smooth slide up rather
		// than the queue snapping into place.
		const float y = Easing::Lerp(NextSlotCenterY(area, slot + 1), NextSlotCenterY(area, slot), ease);
		const float blockSize =
			Easing::Lerp(static_cast<float>(NextSlotBlockSize(slot + 1)), static_cast<float>(NextSlotBlockSize(slot)), ease);

		DrawPiecePreview(target, session.GetNextPiece(slot), blockSize, { centerX, y }, NextSlotTint(slot, count));
	}
}

void BoardRenderer::RenderHoldPreview(sf::RenderTarget& target, const GameplaySession& session, sf::FloatRect area) const
{
	// A flight animation is standing in for the HOLD box's contents right now.
	if (IsHoldFlightActive())
		return;

	if (!session.HasHeldPiece())
		return;

	const sf::Color tint = session.CanHold()
		? sf::Color::White
		: sf::Color(NextMinBrightness, NextMinBrightness, NextMinBrightness);

	const sf::Vector2f center
	{
		area.position.x + area.size.x * 0.5f,
		area.position.y + area.size.y * 0.5f
	};

	DrawPiecePreview(target, session.GetHeldPiece(), NextHeroBlockSize, center, tint);
}

void BoardRenderer::RenderHoldFlight(sf::RenderTarget& target) const
{
	const auto drawFlight = [&](const std::optional<PieceFlight>& flight)
		{
			if (!flight.has_value())
				return;

			const float t = Easing::EaseOutCubic(std::min(flight->timer / HoldFlightDuration, 1.f));
			const sf::Vector2f center = Easing::Lerp(flight->fromCenter, flight->toCenter, t);
			const float blockSize = Easing::Lerp(flight->fromBlockSize, flight->toBlockSize, t);

			DrawPiecePreview(target, Tetromino(flight->type, { 0, 0 }), blockSize, center);
		};

	drawFlight(outgoingFlight);
	drawFlight(incomingFlight);
}

void BoardRenderer::TriggerHoldSwap(const Tetromino& outgoingPiece, std::optional<Tetromino> incomingPiece,
	sf::FloatRect holdBoxArea)
{
	const sf::Vector2f holdCenter
	{
		holdBoxArea.position.x + holdBoxArea.size.x * 0.5f,
		holdBoxArea.position.y + holdBoxArea.size.y * 0.5f
	};

	outgoingFlight = PieceFlight{
		outgoingPiece.GetType(),
		BoardSpaceCenter(outgoingPiece), BlockSize,
		holdCenter, NextHeroBlockSize,
		0.f };

	if (incomingPiece.has_value())
	{
		incomingFlight = PieceFlight{
			incomingPiece->GetType(),
			holdCenter, NextHeroBlockSize,
			BoardSpaceCenter(*incomingPiece), BlockSize,
			0.f };
	}
	else
	{
		incomingFlight.reset();
	}
}

sf::Vector2f BoardRenderer::BoardSpaceCenter(const Tetromino& piece)
{
	const auto blocks = piece.GetBlockPositions();

	int minX = Board::Width;
	int maxX = 0;
	int minY = Board::Height;
	int maxY = 0;

	for (const sf::Vector2i& block : blocks)
	{
		minX = std::min(minX, block.x);
		maxX = std::max(maxX, block.x);
		minY = std::min(minY, block.y);
		maxY = std::max(maxY, block.y);
	}

	return
	{
		BoardPosition.x + static_cast<float>(minX + maxX + 1) * 0.5f * BlockSize,
		RowTop(minY) + static_cast<float>(maxY - minY + 1) * 0.5f * BlockSize
	};
}

void BoardRenderer::DrawPiecePreview(sf::RenderTarget& target, const Tetromino& piece,
	float blockSize, sf::Vector2f center, sf::Color tint) const
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

	// Different pieces fill different cells of the 4x4 shape matrix, so center the
	// piece's own bounding box on `center` rather than pinning its top-left there.
	int minBlockX = TetrominoShapes::MatrixSize;
	int maxBlockX = -1;
	int minBlockY = TetrominoShapes::MatrixSize;
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
		center.x - static_cast<float>(minBlockX + maxBlockX + 1) * 0.5f * blockSize,
		center.y - static_cast<float>(minBlockY + maxBlockY + 1) * 0.5f * blockSize
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
