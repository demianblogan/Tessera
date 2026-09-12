#pragma once

#include <array>
#include <vector>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Vector2.hpp>

#include "../gameplay/TetrominoShapes.h"

// The state machine behind Tessera's gameplay effects: screen shake, the flash
// when a piece locks, the flash / sweep / shatter over rows that are
// clearing, a T-spin's swirl burst, and a Perfect Clear's board-wide flourish.
//
// It holds only timers and the data each effect needs; GameplayState pokes it
// with Trigger* calls and calls Update() every frame, and BoardRenderer reads
// it back to actually draw. No drawing happens here.
class EffectsController
{
public:
	// Kept in step with GameplaySession::RowClearDelay.
	static constexpr float RowClearDuration = 0.45f;
	static constexpr float LandingFlashDuration = 0.15f;
	static constexpr float PerfectClearFlashDuration = 0.9f;

	struct RowClearEffect
	{
		int row = 0;
		float timer = 0.f;
		// How special this clear is: 0 = Single .. 3 = Tetris (4+ rows). Scales
		// the flash/sweep's colour, width and lifetime.
		int rank = 0;
	};

	// A cleared cell's screen position and which cell of the block spritesheet
	// it was drawn from -- what TriggerRowClear shatters into shards.
	struct ClearedCell
	{
		sf::Vector2f position;
		int textureIndex = 0;
	};

	// One small fragment flung out by a clear. textureIndex >= 0 draws a
	// tumbling scrap of the block spritesheet (a real row-clear shatter);
	// textureIndex < 0 draws a plain additive dot in `tint` instead (the
	// T-spin swirl and the Perfect Clear burst, neither tied to a specific
	// block's art).
	struct Shard
	{
		sf::Vector2f position;
		sf::Vector2f velocity;
		float rotation = 0.f;
		float angularVelocity = 0.f;
		float life = 0.f;
		float maxLife = 1.f;
		float size = 1.f;
		int textureIndex = -1;
		sf::Color tint = sf::Color::White;
		// Chaos-tier ambient motes drift instead of falling -- skip gravity.
		bool floaty = false;
	};

	// When false, TriggerShake() does nothing -- the "Screen Shake" gameplay
	// setting, read once when a game starts.
	void SetShakeEnabled(bool enabled) { shakeEnabled = enabled; }

	void TriggerShake(float duration, float intensity);
	void TriggerLandingFlash(const std::array<sf::Vector2i, TetrominoShapes::BLOCK_COUNT>& blocks);

	// `rank` (0 Single .. 3 Tetris) scales the flash/sweep and the shatter
	// spawned from `cells` (every occupied cell in the clearing rows).
	void TriggerRowClear(const std::vector<int>& rows, int rank, const std::vector<ClearedCell>& cells);

	// A T-spin's own tell, independent of whether it cleared any lines: a
	// small purple swirl around the piece that just locked.
	void TriggerTSpinBurst(sf::Vector2f centre);

	// The board just went completely empty. `boardArea` is shattered into a
	// wide golden burst and the whole area gets a slow-fading flash.
	void TriggerPerfectClearBurst(sf::FloatRect boardArea);

	// `count` is GameplaySession::Events::comboCount (0 on the first clear of
	// a chain, incrementing with each one that directly follows); 0 or below
	// fades the glow out instead of holding it, for the lock that broke the
	// chain. Eased in Update(), so it rises and lingers rather than snapping.
	void SetCombo(int count);

	// Escalation tier ambience (see EscalationDirector::Tier): 0 Base,
	// 1 SpeedSurge, 2 Garbage, 3 Chaos. Called every frame -- the well's border
	// glow eases toward the target tier's colour/strength instead of snapping,
	// and `boardArea` is where Chaos's ambient motes spawn.
	void SetEscalationTier(int tierLevel, sf::FloatRect boardArea);

	// A Speed Surge started: a reddish board-wide wash for `duration` (pass
	// EscalationDirector::SurgeDuration), fading out over its second half.
	void TriggerSpeedSurgeGlow(float duration);

	// A garbage row was just pushed up from below: a brief red board flash,
	// distinct from the standing Garbage-tier border glow.
	void TriggerGarbageImpact();

	void Update(float deltaTime);

	[[nodiscard]] sf::Vector2f GetViewOffset() const { return shakeOffset; }

	[[nodiscard]] bool HasLandingFlash() const { return landingFlashTimer > 0.f; }
	[[nodiscard]] float GetLandingFlashProgress() const;
	[[nodiscard]] const std::array<sf::Vector2i, TetrominoShapes::BLOCK_COUNT>& GetLandingFlashBlocks() const { return landingFlashBlocks; }

	[[nodiscard]] const std::vector<RowClearEffect>& GetRowClearEffects() const { return rowClearEffects; }
	[[nodiscard]] const std::vector<Shard>& GetShards() const { return shards; }

	[[nodiscard]] bool HasPerfectClearFlash() const { return perfectClearFlashTimer > 0.f; }
	[[nodiscard]] float GetPerfectClearFlashProgress() const;

	// Eased combo level, roughly in [0, comboTarget]; >0 while a chain of
	// clears is holding or fading out. Drives the well's border glow.
	[[nodiscard]] float GetComboGlowLevel() const { return comboGlowLevel; }

	// 0..1, how strongly the current tier's ambience should show; the colour
	// to show it in (red past Garbage, gold once Chaos is reached).
	[[nodiscard]] float GetTierGlowLevel() const { return tierGlowLevel; }
	[[nodiscard]] sf::Color GetTierGlowColour() const { return tierGlowColour; }

	[[nodiscard]] bool HasSpeedSurgeGlow() const { return surgeGlowTimer > 0.f; }
	[[nodiscard]] float GetSpeedSurgeGlowProgress() const;

	[[nodiscard]] bool HasGarbageFlash() const { return garbageFlashTimer > 0.f; }
	[[nodiscard]] float GetGarbageFlashProgress() const;

private:
	bool shakeEnabled = true;
	float shakeTimer = 0.f;
	float shakeDuration = 0.f;
	float shakeIntensity = 0.f;
	sf::Vector2f shakeOffset{ 0.f, 0.f };

	std::array<sf::Vector2i, TetrominoShapes::BLOCK_COUNT> landingFlashBlocks{};
	float landingFlashTimer = 0.f;

	std::vector<RowClearEffect> rowClearEffects;
	std::vector<Shard> shards;
	float perfectClearFlashTimer = 0.f;

	float comboTargetLevel = 0.f;
	float comboGlowLevel = 0.f;

	static constexpr float GarbageFlashDuration = 0.3f;
	static constexpr float ChaosMotesPerSecond = 3.f;

	int escalationTierLevel = 0;
	sf::FloatRect ambientArea{};
	float tierGlowLevel = 0.f;
	sf::Color tierGlowColour{ 235, 90, 70 };
	float chaosSpawnCarry = 0.f;

	float surgeGlowTimer = 0.f;
	float surgeGlowDuration = 1.f;

	float garbageFlashTimer = 0.f;
};
