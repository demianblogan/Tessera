#pragma once

#include <array>
#include <vector>

#include <SFML/System/Vector2.hpp>

#include "../gameplay/TetrominoShapes.h"

// The state machine behind Tessera's gameplay effects: screen shake, the flash
// when a piece locks, and the flash / sweep over rows that are clearing.
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

	struct RowClearEffect
	{
		int row = 0;
		float timer = 0.f;
	};

	// When false, TriggerShake() does nothing -- the "Screen Shake" gameplay
	// setting, read once when a game starts.
	void SetShakeEnabled(bool enabled) { shakeEnabled = enabled; }

	void TriggerShake(float duration, float intensity);
	void TriggerLandingFlash(const std::array<sf::Vector2i, TetrominoShapes::BLOCK_COUNT>& blocks);
	void TriggerRowClear(const std::vector<int>& rows);

	void Update(float deltaTime);

	[[nodiscard]] sf::Vector2f GetViewOffset() const { return shakeOffset; }

	[[nodiscard]] bool HasLandingFlash() const { return landingFlashTimer > 0.f; }
	[[nodiscard]] float GetLandingFlashProgress() const;
	[[nodiscard]] const std::array<sf::Vector2i, TetrominoShapes::BLOCK_COUNT>& GetLandingFlashBlocks() const { return landingFlashBlocks; }

	[[nodiscard]] const std::vector<RowClearEffect>& GetRowClearEffects() const { return rowClearEffects; }

private:
	bool shakeEnabled = true;
	float shakeTimer = 0.f;
	float shakeDuration = 0.f;
	float shakeIntensity = 0.f;
	sf::Vector2f shakeOffset{ 0.f, 0.f };

	std::array<sf::Vector2i, TetrominoShapes::BLOCK_COUNT> landingFlashBlocks{};
	float landingFlashTimer = 0.f;

	std::vector<RowClearEffect> rowClearEffects;
};
