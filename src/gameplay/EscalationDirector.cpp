#include "EscalationDirector.h"

#include <utility>

#include "../utils/Random.h"

namespace
{
	[[nodiscard]] EscalationDirector::Tier TierForElapsed(float elapsedSeconds)
	{
		using Tier = EscalationDirector::Tier;

		if (elapsedSeconds >= EscalationDirector::ChaosTierStart)
			return Tier::Chaos;
		if (elapsedSeconds >= EscalationDirector::GarbageTierStart)
			return Tier::Garbage;
		if (elapsedSeconds >= EscalationDirector::SpeedSurgeTierStart)
			return Tier::SpeedSurge;

		return Tier::Base;
	}
}

void EscalationDirector::Update(float deltaTime)
{
	elapsedSeconds += deltaTime;
	UpdateTier();
	UpdateSurge(deltaTime);
	UpdateGarbage(deltaTime);
}

void EscalationDirector::UpdateTier()
{
	const Tier newTier = TierForElapsed(elapsedSeconds);

	if (newTier != tier)
	{
		tier = newTier;
		pendingEvents.hasTierChanged = true;
		pendingEvents.tier = tier;

		if (tier == Tier::Chaos)
			chaosTierEnteredAt = elapsedSeconds;
	}
}

void EscalationDirector::UpdateSurge(float deltaTime)
{
	if (tier < Tier::SpeedSurge)
		return;

	if (isSurgeActive)
	{
		surgeTimer += deltaTime;

		if (surgeTimer >= SurgeDuration)
		{
			isSurgeActive = false;
			surgeTimer = 0.f;
			surgeCooldown = SurgeInterval;
			pendingEvents.hasSurgeEnded = true;
		}

		return;
	}

	surgeCooldown -= deltaTime;

	if (surgeCooldown <= 0.f)
	{
		isSurgeActive = true;
		surgeTimer = 0.f;
		pendingEvents.hasSurgeStarted = true;
	}
}

void EscalationDirector::UpdateGarbage(float deltaTime)
{
	if (tier < Tier::Garbage)
		return;

	// Negative means "not rolled yet" -- rolls the first interval the moment
	// the tier starts, rather than waiting a full Update() for it.
	if (garbageCooldown < 0.f)
		garbageCooldown = Random::Float(GarbageMinInterval, GarbageMaxInterval);

	garbageCooldown -= deltaTime;

	if (garbageCooldown <= 0.f)
	{
		pendingGarbageRows++;
		garbageCooldown = Random::Float(GarbageMinInterval, GarbageMaxInterval);
	}
}

bool EscalationDirector::ConsumePendingGarbageRow()
{
	if (pendingGarbageRows <= 0)
		return false;

	pendingGarbageRows--;
	return true;
}

bool EscalationDirector::ShouldSpawnGoldenPiece()
{
	if (tier != Tier::Chaos)
	{
		piecesSinceGolden = 0;
		return false;
	}

	// As with the first garbage row: don't leave the first golden piece to
	// however many pieces the player happens to spawn in a given stretch.
	if (!hasGrantedFirstGolden && elapsedSeconds - chaosTierEnteredAt >= FirstGoldenDelay)
	{
		hasGrantedFirstGolden = true;
		piecesSinceGolden = 0;
		return true;
	}

	piecesSinceGolden++;

	if (piecesSinceGolden >= PiecesPerGoldenPiece)
	{
		piecesSinceGolden = 0;
		return true;
	}

	return false;
}

EscalationDirector::Events EscalationDirector::ConsumeEvents()
{
	Events consumed = std::move(pendingEvents);
	pendingEvents = {};
	return consumed;
}

EscalationDirector::Tier EscalationDirector::CurrentTier() const
{
	return tier;
}

float EscalationDirector::FallSpeedMultiplier() const
{
	return isSurgeActive ? SurgeFallMultiplier : 1.f;
}
