#include "EffectsController.h"

#include <algorithm>
#include <cmath>
#include <numbers>

#include "../utils/Random.h"

namespace
{
	constexpr float Pi = std::numbers::pi_v<float>;
	constexpr float ShardGravity = 260.f;
	constexpr float FullCircleDegrees = 360.f;

	const sf::Color TSpinColor{ 200, 120, 255 };
	const sf::Color PerfectClearColor{ 255, 215, 90 };
	const sf::Color ChaosMoteColor{ 255, 210, 90 };

	// TriggerRowClear: higher ranks throw more shards, faster and longer-lived,
	// so a Tetris reads as a real shattering rather than the same handful of
	// specks a Single gets.
	constexpr int RowClearShardsPerCellBase = 2;
	constexpr float RowClearShardSpeedBase = 70.f;
	constexpr float RowClearShardSpeedPerRank = 35.f;
	constexpr float RowClearShardSpeedVarianceMin = 0.5f;
	constexpr float RowClearShardSpeedVarianceMax = 1.3f;
	constexpr float RowClearShardUpwardBias = 40.f;
	constexpr float RowClearShardMaxAngularVelocity = 260.f;
	constexpr float RowClearShardLifetimeBase = 0.4f;
	constexpr float RowClearShardLifetimePerRank = 0.12f;
	constexpr float RowClearShardLifetimeVariance = 0.15f;
	constexpr float RowClearShardMinSize = 0.35f;
	constexpr float RowClearShardMaxSize = 0.55f;

	// TriggerImpactDust (hard-drop / wall dust).
	constexpr int ImpactDustMinSpawnPoints = 1;
	constexpr int ImpactDustMaxSpawnPoints = 3;
	constexpr float ImpactDustMinDirectionLength = 0.001f;   // below this, burstDirection is treated as zero
	constexpr float ImpactDustMinSpeed = 90.f;
	constexpr float ImpactDustMaxSpeed = 160.f;
	constexpr float ImpactDustMaxScatter = 25.f;
	constexpr float ImpactDustMinLifetime = 0.3f;
	constexpr float ImpactDustMaxLifetime = 0.45f;
	constexpr float ImpactDustMinSize = 0.1f;
	constexpr float ImpactDustMaxSize = 0.18f;

	// TriggerTSpinBurst.
	constexpr float TSpinBurstAngleJitter = 0.2f;
	constexpr float TSpinBurstMinRadius = 6.f;
	constexpr float TSpinBurstMaxRadius = 18.f;
	constexpr float TSpinBurstMinSpeed = 90.f;
	constexpr float TSpinBurstMaxSpeed = 190.f;
	constexpr float TSpinBurstOutwardFraction = 0.35f;
	constexpr float TSpinBurstMinLifetime = 0.4f;
	constexpr float TSpinBurstMaxLifetime = 0.65f;
	constexpr float TSpinBurstMinSize = 0.14f;
	constexpr float TSpinBurstMaxSize = 0.24f;

	// TriggerPerfectClearBurst.
	constexpr float PerfectClearMinSpeed = 60.f;
	constexpr float PerfectClearMaxSpeed = 220.f;
	constexpr float PerfectClearUpwardBias = 60.f;
	constexpr float PerfectClearMinLifetime = 0.6f;
	constexpr float PerfectClearMaxLifetime = 1.1f;
	constexpr float PerfectClearMinSize = 0.18f;
	constexpr float PerfectClearMaxSize = 0.34f;

	constexpr float MinSurgeGlowDuration = 0.01f;

	// Update: combo glow rise/fall speed (see the comment at its call site).
	constexpr float ComboGlowRiseSpeed = 14.f;
	constexpr float ComboGlowFallSpeed = 2.5f;

	// Update: Chaos-tier ambient motes.
	constexpr int ChaosTierLevel = 3;
	constexpr float ChaosMoteSpawnMarginAboveBoard = 12.f;
	constexpr float ChaosMoteMaxHorizontalDrift = 8.f;
	constexpr float ChaosMoteMinFallSpeed = 16.f;
	constexpr float ChaosMoteMaxFallSpeed = 30.f;
	constexpr float ChaosMoteMinLifetime = 3.f;
	constexpr float ChaosMoteMaxLifetime = 5.f;
	constexpr float ChaosMoteMinSize = 0.08f;
	constexpr float ChaosMoteMaxSize = 0.16f;
}

void EffectsController::SetShakeEnabled(bool isEnabled)
{
	isShakeEnabled = isEnabled;
}

void EffectsController::TriggerShake(float duration, float intensity)
{
	if (!isShakeEnabled)
		return;

	shakeDuration = duration;
	shakeTimer = duration;
	shakeIntensity = intensity;
}

void EffectsController::TriggerLandingFlash(const std::array<sf::Vector2i, TetrominoShapes::BlockCount>& blocks)
{
	landingFlashBlocks = blocks;
	landingFlashTimer = LandingFlashDuration;
}

void EffectsController::TriggerRowClear(const std::vector<int>& rows, int rank, const std::vector<ClearedCell>& cells)
{
	for (int row : rows)
		rowClearEffects.push_back({ .row = row, .timer = 0.f, .rank = rank });

	// Higher ranks throw more shards, faster and longer-lived, so a Tetris
	// reads as a real shattering rather than the same handful of specks a
	// Single gets.
	const int shardsPerCell = RowClearShardsPerCellBase + rank;
	const float baseSpeed = RowClearShardSpeedBase + static_cast<float>(rank) * RowClearShardSpeedPerRank;

	for (const ClearedCell& cell : cells)
	{
		for (int i = 0; i < shardsPerCell; ++i)
		{
			const float angle = Random::Float(0.f, 2.f * Pi);
			const float speed = baseSpeed * Random::Float(RowClearShardSpeedVarianceMin, RowClearShardSpeedVarianceMax);

			Shard shard;
			shard.position = cell.position;
			shard.velocity = { std::cos(angle) * speed, std::sin(angle) * speed - RowClearShardUpwardBias };
			shard.rotation = Random::Float(0.f, FullCircleDegrees);
			shard.angularVelocity = Random::Float(-RowClearShardMaxAngularVelocity, RowClearShardMaxAngularVelocity);
			shard.maxLife = RowClearShardLifetimeBase + static_cast<float>(rank) * RowClearShardLifetimePerRank
				+ Random::Float(0.f, RowClearShardLifetimeVariance);
			shard.life = shard.maxLife;
			shard.size = Random::Float(RowClearShardMinSize, RowClearShardMaxSize);
			shard.textureIndex = cell.textureIndex;
			shards.push_back(shard);
		}
	}
}

void EffectsController::TriggerImpactDust(const std::vector<sf::Vector2f>& impactPoints, sf::Vector2f burstDirection)
{
	constexpr int PerSpawnPoint = 2;
	// How far along the edge (perpendicular to the burst) a spawn point can
	// land from the cell's own reference point -- a small range rather than
	// one fixed spot, so repeated impacts on the same cell don't look identical.
	constexpr float SpawnSpread = 16.f;
	const sf::Color dustColor(80, 80, 80);

	const float length = std::sqrt(burstDirection.x * burstDirection.x + burstDirection.y * burstDirection.y);
	const sf::Vector2f direction = length > ImpactDustMinDirectionLength ? burstDirection / length : sf::Vector2f{ 0.f, -1.f };
	const sf::Vector2f perpendicular{ -direction.y, direction.x };

	for (const sf::Vector2f& point : impactPoints)
	{
		// 1-3 spawn points per cell, each jittered along the impacted edge,
		// rather than every impact spraying from the exact same spot.
		const int spawnPoints = Random::Int(ImpactDustMinSpawnPoints, ImpactDustMaxSpawnPoints);

		for (int spawn = 0; spawn < spawnPoints; spawn++)
		{
			const sf::Vector2f spawnPosition = point + perpendicular * Random::Float(-SpawnSpread * 0.5f, SpawnSpread * 0.5f);

			for (int i = 0; i < PerSpawnPoint; i++)
			{
				// Mostly along `direction` with only a slight sideways scatter --
				// an impact bounce, not a burst in every direction -- then
				// gravity (applied generically to every non-floaty shard) arcs
				// it back down.
				const float speed = Random::Float(ImpactDustMinSpeed, ImpactDustMaxSpeed);
				const float scatter = Random::Float(-ImpactDustMaxScatter, ImpactDustMaxScatter);

				Shard shard;
				shard.position = spawnPosition;
				shard.velocity = direction * speed + perpendicular * scatter;
				shard.maxLife = Random::Float(ImpactDustMinLifetime, ImpactDustMaxLifetime);
				shard.life = shard.maxLife;
				shard.size = Random::Float(ImpactDustMinSize, ImpactDustMaxSize);
				shard.textureIndex = -1;
				shard.tint = dustColor;
				shards.push_back(shard);
			}
		}
	}
}

void EffectsController::TriggerHardDropDust(const std::vector<sf::Vector2f>& impactPoints)
{
	TriggerImpactDust(impactPoints, { 0.f, -1.f });
}

void EffectsController::TriggerWallDust(const std::vector<sf::Vector2f>& impactPoints, int wallDirection)
{
	TriggerImpactDust(impactPoints, { -static_cast<float>(wallDirection), 0.f });
}

void EffectsController::TriggerTSpinBurst(sf::Vector2f center)
{
	constexpr int Count = 26;

	for (int i = 0; i < Count; ++i)
	{
		const float angle = (static_cast<float>(i) / static_cast<float>(Count)) * 2.f * Pi
			+ Random::Float(-TSpinBurstAngleJitter, TSpinBurstAngleJitter);
		const float radius = Random::Float(TSpinBurstMinRadius, TSpinBurstMaxRadius);
		const float speed = Random::Float(TSpinBurstMinSpeed, TSpinBurstMaxSpeed);

		// Mostly tangential (a spiral around the piece), with a light outward push.
		const sf::Vector2f radial{ std::cos(angle), std::sin(angle) };
		const sf::Vector2f tangent{ -radial.y, radial.x };

		Shard shard;
		shard.position = center + radial * radius;
		shard.velocity = tangent * speed + radial * (speed * TSpinBurstOutwardFraction);
		shard.maxLife = Random::Float(TSpinBurstMinLifetime, TSpinBurstMaxLifetime);
		shard.life = shard.maxLife;
		shard.size = Random::Float(TSpinBurstMinSize, TSpinBurstMaxSize);
		shard.textureIndex = -1;
		shard.tint = TSpinColor;
		shards.push_back(shard);
	}
}

void EffectsController::TriggerPerfectClearBurst(sf::FloatRect boardArea)
{
	constexpr int Count = 90;

	for (int i = 0; i < Count; i++)
	{
		const sf::Vector2f position
		{
			boardArea.position.x + Random::Float(0.f, boardArea.size.x),
			boardArea.position.y + Random::Float(0.f, boardArea.size.y)
		};

		const float angle = Random::Float(0.f, 2.f * Pi);
		const float speed = Random::Float(PerfectClearMinSpeed, PerfectClearMaxSpeed);

		Shard shard;
		shard.position = position;
		shard.velocity = { std::cos(angle) * speed, std::sin(angle) * speed - PerfectClearUpwardBias };
		shard.maxLife = Random::Float(PerfectClearMinLifetime, PerfectClearMaxLifetime);
		shard.life = shard.maxLife;
		shard.size = Random::Float(PerfectClearMinSize, PerfectClearMaxSize);
		shard.textureIndex = -1;
		shard.tint = PerfectClearColor;
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
	surgeGlowDuration = std::max(duration, MinSurgeGlowDuration);
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
		landingFlashTimer -= deltaTime;

	// =====================================================
	// Row-clear flash / sweep
	// =====================================================

	for (RowClearEffect& effect : rowClearEffects)
		effect.timer += deltaTime;

	std::erase_if(
		rowClearEffects,
		[](const RowClearEffect& effect)
		{
			return effect.timer >= RowClearDuration;
		}
	);

	// =====================================================
	// Shards (row-clear shatter, T-spin swirl, Perfect Clear burst)
	// =====================================================

	for (Shard& shard : shards)
	{
		if (!shard.isFloaty)
			shard.velocity.y += ShardGravity * deltaTime;

		shard.position += shard.velocity * deltaTime;
		shard.rotation += shard.angularVelocity * deltaTime;
		shard.life -= deltaTime;
	}

	std::erase_if(
		shards,
		[](const Shard& shard)
		{
			return shard.life <= 0.f;
		});

	if (perfectClearFlashTimer > 0.f)
		perfectClearFlashTimer = std::max(0.f, perfectClearFlashTimer - deltaTime);

	// =====================================================
	// Combo glow -- rises quickly as the chain builds, lingers and fades
	// slowly once it breaks, rather than snapping to the new (lower) target.
	// =====================================================

	{
		const float speed = comboTargetLevel > comboGlowLevel ? ComboGlowRiseSpeed : ComboGlowFallSpeed;
		const float step = speed * deltaTime;
		if (comboGlowLevel < comboTargetLevel)
			comboGlowLevel = std::min(comboTargetLevel, comboGlowLevel + step);
		else
			comboGlowLevel = std::max(comboTargetLevel, comboGlowLevel - step);
	}

	// =====================================================
	// Chaos ambience -- slow-drifting golden motes across the board, the
	// tier's own standing tell (Garbage's is the one-off wave below instead;
	// a screen-wide glow held for the rest of the run read as an eyesore, not
	// tension).
	// =====================================================

	if (escalationTierLevel >= ChaosTierLevel && ambientArea.size.x > 0.f)
	{
		chaosSpawnCarry += deltaTime * ChaosMotesPerSecond;
		while (chaosSpawnCarry >= 1.f)
		{
			chaosSpawnCarry -= 1.f;

			Shard mote;
			mote.position =
			{
				Random::Float(ambientArea.position.x, ambientArea.position.x + ambientArea.size.x),
				ambientArea.position.y - ChaosMoteSpawnMarginAboveBoard
			};
			mote.velocity =
			{
				Random::Float(-ChaosMoteMaxHorizontalDrift, ChaosMoteMaxHorizontalDrift),
				Random::Float(ChaosMoteMinFallSpeed, ChaosMoteMaxFallSpeed)
			};
			mote.maxLife = Random::Float(ChaosMoteMinLifetime, ChaosMoteMaxLifetime);
			mote.life = mote.maxLife;
			mote.size = Random::Float(ChaosMoteMinSize, ChaosMoteMaxSize);
			mote.textureIndex = -1;
			mote.tint = ChaosMoteColor;
			mote.isFloaty = true;

			shards.push_back(mote);
		}
	}

	if (surgeGlowTimer > 0.f)
		surgeGlowTimer = std::max(0.f, surgeGlowTimer - deltaTime);

	if (garbageWaveTimer > 0.f)
		garbageWaveTimer = std::max(0.f, garbageWaveTimer - deltaTime);

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

sf::Vector2f EffectsController::GetViewOffset() const
{
	return shakeOffset;
}

bool EffectsController::HasLandingFlash() const
{
	return landingFlashTimer > 0.f;
}

float EffectsController::GetLandingFlashProgress() const
{
	return std::clamp(landingFlashTimer / LandingFlashDuration, 0.f, 1.f);
}

const std::array<sf::Vector2i, TetrominoShapes::BlockCount>& EffectsController::GetLandingFlashBlocks() const
{
	return landingFlashBlocks;
}

const std::vector<EffectsController::RowClearEffect>& EffectsController::GetRowClearEffects() const
{
	return rowClearEffects;
}

const std::vector<EffectsController::Shard>& EffectsController::GetShards() const
{
	return shards;
}

bool EffectsController::HasPerfectClearFlash() const
{
	return perfectClearFlashTimer > 0.f;
}

float EffectsController::GetPerfectClearFlashProgress() const
{
	return std::clamp(perfectClearFlashTimer / PerfectClearFlashDuration, 0.f, 1.f);
}

float EffectsController::GetComboGlowLevel() const
{
	return comboGlowLevel;
}

bool EffectsController::HasSpeedSurgeGlow() const
{
	return surgeGlowTimer > 0.f;
}

float EffectsController::GetSpeedSurgeGlowProgress() const
{
	return std::clamp(surgeGlowTimer / surgeGlowDuration, 0.f, 1.f);
}

bool EffectsController::HasGarbageWave() const
{
	return garbageWaveTimer > 0.f;
}

float EffectsController::GetGarbageWaveProgress() const
{
	return 1.f - std::clamp(garbageWaveTimer / GarbageWaveDuration, 0.f, 1.f);
}
