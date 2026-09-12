#include "EffectsController.h"

#include <algorithm>
#include <cmath>

#include "../utils/Random.h"

namespace
{
	constexpr float Pi = 3.14159265f;
	constexpr float ShardGravity = 260.f;

	const sf::Color TSpinColour{ 200, 120, 255 };
	const sf::Color PerfectClearColour{ 255, 215, 90 };
}

void EffectsController::TriggerShake(float duration, float intensity)
{
	if (!shakeEnabled)
	{
		return;
	}

	shakeDuration = duration;
	shakeTimer = duration;
	shakeIntensity = intensity;
}

void EffectsController::TriggerLandingFlash(const std::array<sf::Vector2i, TetrominoShapes::BLOCK_COUNT>& blocks)
{
	landingFlashBlocks = blocks;
	landingFlashTimer = LandingFlashDuration;
}

void EffectsController::TriggerRowClear(const std::vector<int>& rows, int rank, const std::vector<ClearedCell>& cells)
{
	for (int row : rows)
	{
		rowClearEffects.push_back({ .row = row, .timer = 0.f, .rank = rank });
	}

	// Higher ranks throw more shards, faster and longer-lived, so a Tetris
	// reads as a real shattering rather than the same handful of specks a
	// Single gets.
	const int shardsPerCell = 2 + rank;
	const float baseSpeed = 70.f + static_cast<float>(rank) * 35.f;

	for (const ClearedCell& cell : cells)
	{
		for (int i = 0; i < shardsPerCell; ++i)
		{
			const float angle = Random::Float(0.f, 2.f * Pi);
			const float speed = baseSpeed * Random::Float(0.5f, 1.3f);

			Shard shard;
			shard.position = cell.position;
			shard.velocity = { std::cos(angle) * speed, std::sin(angle) * speed - 40.f };
			shard.rotation = Random::Float(0.f, 360.f);
			shard.angularVelocity = Random::Float(-260.f, 260.f);
			shard.maxLife = 0.4f + static_cast<float>(rank) * 0.12f + Random::Float(0.f, 0.15f);
			shard.life = shard.maxLife;
			shard.size = Random::Float(0.35f, 0.55f);
			shard.textureIndex = cell.textureIndex;
			shards.push_back(shard);
		}
	}
}

void EffectsController::TriggerTSpinBurst(sf::Vector2f centre)
{
	constexpr int Count = 26;

	for (int i = 0; i < Count; ++i)
	{
		const float angle = (static_cast<float>(i) / static_cast<float>(Count)) * 2.f * Pi + Random::Float(-0.2f, 0.2f);
		const float radius = Random::Float(6.f, 18.f);
		const float speed = Random::Float(90.f, 190.f);

		// Mostly tangential (a spiral around the piece), with a light outward push.
		const sf::Vector2f radial{ std::cos(angle), std::sin(angle) };
		const sf::Vector2f tangent{ -radial.y, radial.x };

		Shard shard;
		shard.position = centre + radial * radius;
		shard.velocity = tangent * speed + radial * (speed * 0.35f);
		shard.maxLife = Random::Float(0.4f, 0.65f);
		shard.life = shard.maxLife;
		shard.size = Random::Float(0.14f, 0.24f);
		shard.textureIndex = -1;
		shard.tint = TSpinColour;
		shards.push_back(shard);
	}
}

void EffectsController::TriggerPerfectClearBurst(sf::FloatRect boardArea)
{
	constexpr int Count = 90;

	for (int i = 0; i < Count; ++i)
	{
		const sf::Vector2f position{
			boardArea.position.x + Random::Float(0.f, boardArea.size.x),
			boardArea.position.y + Random::Float(0.f, boardArea.size.y) };

		const float angle = Random::Float(0.f, 2.f * Pi);
		const float speed = Random::Float(60.f, 220.f);

		Shard shard;
		shard.position = position;
		shard.velocity = { std::cos(angle) * speed, std::sin(angle) * speed - 60.f };
		shard.maxLife = Random::Float(0.6f, 1.1f);
		shard.life = shard.maxLife;
		shard.size = Random::Float(0.18f, 0.34f);
		shard.textureIndex = -1;
		shard.tint = PerfectClearColour;
		shards.push_back(shard);
	}

	perfectClearFlashTimer = PerfectClearFlashDuration;
}

void EffectsController::SetCombo(int count)
{
	comboTargetLevel = static_cast<float>(std::max(0, count));
}

void EffectsController::SetEscalationTier(int tierLevel, sf::FloatRect boardArea)
{
	escalationTierLevel = tierLevel;
	ambientArea = boardArea;
}

void EffectsController::TriggerSpeedSurgeGlow(float duration)
{
	surgeGlowDuration = std::max(duration, 0.01f);
	surgeGlowTimer = surgeGlowDuration;
}

void EffectsController::TriggerGarbageWave()
{
	garbageWaveTimer = GarbageWaveDuration;
}

void EffectsController::Update(float deltaTime)
{
	// =====================================================
	// Landing flash
	// =====================================================

	if (landingFlashTimer > 0.f)
	{
		landingFlashTimer -= deltaTime;
	}

	// =====================================================
	// Row-clear flash / sweep
	// =====================================================

	for (RowClearEffect& effect : rowClearEffects)
	{
		effect.timer += deltaTime;
	}

	std::erase_if(
		rowClearEffects,
		[](const RowClearEffect& effect) { return effect.timer >= RowClearDuration; }
	);

	// =====================================================
	// Shards (row-clear shatter, T-spin swirl, Perfect Clear burst)
	// =====================================================

	for (Shard& shard : shards)
	{
		if (!shard.floaty)
		{
			shard.velocity.y += ShardGravity * deltaTime;
		}
		shard.position += shard.velocity * deltaTime;
		shard.rotation += shard.angularVelocity * deltaTime;
		shard.life -= deltaTime;
	}

	std::erase_if(shards, [](const Shard& shard) { return shard.life <= 0.f; });

	if (perfectClearFlashTimer > 0.f)
	{
		perfectClearFlashTimer = std::max(0.f, perfectClearFlashTimer - deltaTime);
	}

	// =====================================================
	// Combo glow -- rises quickly as the chain builds, lingers and fades
	// slowly once it breaks, rather than snapping to the new (lower) target.
	// =====================================================

	{
		const float speed = comboTargetLevel > comboGlowLevel ? 14.f : 2.5f;
		const float step = speed * deltaTime;
		if (comboGlowLevel < comboTargetLevel)
		{
			comboGlowLevel = std::min(comboTargetLevel, comboGlowLevel + step);
		}
		else
		{
			comboGlowLevel = std::max(comboTargetLevel, comboGlowLevel - step);
		}
	}

	// =====================================================
	// Chaos ambience -- slow-drifting golden motes across the board, the
	// tier's own standing tell (Garbage's is the one-off wave below instead;
	// a screen-wide glow held for the rest of the run read as an eyesore, not
	// tension).
	// =====================================================

	if (escalationTierLevel >= 3 && ambientArea.size.x > 0.f)
	{
		chaosSpawnCarry += deltaTime * ChaosMotesPerSecond;
		while (chaosSpawnCarry >= 1.f)
		{
			chaosSpawnCarry -= 1.f;

			Shard mote;
			mote.position = { Random::Float(ambientArea.position.x, ambientArea.position.x + ambientArea.size.x),
				ambientArea.position.y - 12.f };
			mote.velocity = { Random::Float(-8.f, 8.f), Random::Float(16.f, 30.f) };
			mote.maxLife = Random::Float(3.f, 5.f);
			mote.life = mote.maxLife;
			mote.size = Random::Float(0.08f, 0.16f);
			mote.textureIndex = -1;
			mote.tint = sf::Color(255, 210, 90);
			mote.floaty = true;
			shards.push_back(mote);
		}
	}

	if (surgeGlowTimer > 0.f)
	{
		surgeGlowTimer = std::max(0.f, surgeGlowTimer - deltaTime);
	}

	if (garbageWaveTimer > 0.f)
	{
		garbageWaveTimer = std::max(0.f, garbageWaveTimer - deltaTime);
	}

	// =====================================================
	// Screen shake
	//
	// The random offset is computed here, in Update, and only applied in the
	// renderer. Rendering must stay a pure function of state: pulling random
	// numbers inside Render made frame output depend on how many times Render
	// happened to run and perturbed every other consumer of Random.
	// =====================================================

	if (shakeTimer > 0.f)
	{
		shakeTimer -= deltaTime;

		const float progress = shakeDuration > 0.f ? std::max(0.f, shakeTimer / shakeDuration) : 0.f;
		const float currentIntensity = shakeIntensity * progress;

		shakeOffset =
		{
			Random::Float(-currentIntensity, currentIntensity),
			Random::Float(-currentIntensity, currentIntensity)
		};
	}
	else
	{
		shakeOffset = { 0.f, 0.f };
	}
}

float EffectsController::GetLandingFlashProgress() const
{
	return std::clamp(landingFlashTimer / LandingFlashDuration, 0.f, 1.f);
}

float EffectsController::GetPerfectClearFlashProgress() const
{
	return std::clamp(perfectClearFlashTimer / PerfectClearFlashDuration, 0.f, 1.f);
}

float EffectsController::GetSpeedSurgeGlowProgress() const
{
	return std::clamp(surgeGlowTimer / surgeGlowDuration, 0.f, 1.f);
}

float EffectsController::GetGarbageWaveProgress() const
{
	return 1.f - std::clamp(garbageWaveTimer / GarbageWaveDuration, 0.f, 1.f);
}
