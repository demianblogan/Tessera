#include "EffectsController.h"

#include <algorithm>

#include "../utils/Random.h"

void EffectsController::TriggerShake(float duration, float intensity)
{
	shakeDuration = duration;
	shakeTimer = duration;
	shakeIntensity = intensity;
}

void EffectsController::TriggerLandingFlash(const std::array<sf::Vector2i, TetrominoShapes::BLOCK_COUNT>& blocks)
{
	landingFlashBlocks = blocks;
	landingFlashTimer = LandingFlashDuration;
}

void EffectsController::TriggerRowClear(const std::vector<int>& rows)
{
	for (int row : rows)
	{
		rowClearEffects.push_back({ .row = row, .timer = 0.f });
	}
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
