#include "EscalationDirector.h"

#include <utility>

namespace
{
	[[nodiscard]] EscalationDirector::Tier TierForElapsed(float elapsedSeconds)
	{
		using Tier = EscalationDirector::Tier;

		if (elapsedSeconds >= EscalationDirector::ChaosTierStart) { return Tier::Chaos; }
		if (elapsedSeconds >= EscalationDirector::GarbageTierStart) { return Tier::Garbage; }
		if (elapsedSeconds >= EscalationDirector::SpeedSurgeTierStart) { return Tier::SpeedSurge; }
		return Tier::Base;
	}
}

void EscalationDirector::Update(float deltaTime)
{
	elapsedSeconds += deltaTime;
	UpdateTier();
	UpdateSurge(deltaTime);
}

void EscalationDirector::UpdateTier()
{
	const Tier newTier = TierForElapsed(elapsedSeconds);

	if (newTier != tier)
	{
		tier = newTier;
		pendingEvents.tierChanged = true;
		pendingEvents.tier = tier;

		if (tier == Tier::Garbage) { garbageTierEnteredAt = elapsedSeconds; }
		if (tier == Tier::Chaos) { chaosTierEnteredAt = elapsedSeconds; }
	}
}

void EscalationDirector::UpdateSurge(float deltaTime)
{
	if (tier < Tier::SpeedSurge)
	{
		return;
	}

	if (surgeActive)
	{
		surgeTimer += deltaTime;

		if (surgeTimer >= SurgeDuration)
		{
			surgeActive = false;
			surgeTimer = 0.f;
			surgeCooldown = SurgeInterval;
			pendingEvents.surgeEnded = true;
		}

		return;
	}

	surgeCooldown -= deltaTime;

	if (surgeCooldown <= 0.f)
	{
		surgeActive = true;
		surgeTimer = 0.f;
		pendingEvents.surgeStarted = true;
	}
}

void EscalationDirector::NotifyLinesCleared(int rowCount)
{
	if (tier < Tier::Garbage || rowCount <= 0)
	{
		return;
	}

	linesSinceGarbage += rowCount;

	while (linesSinceGarbage >= LinesPerGarbageRow)
	{
		linesSinceGarbage -= LinesPerGarbageRow;
		++pendingGarbageRows;
	}
}

bool EscalationDirector::ConsumePendingGarbageRow()
{
	// The first garbage row is time-gated rather than earned by actual clears --
	// clearing four lines within FirstGarbageDelay of entering the tier isn't
	// guaranteed, but the tier's own arrival should still be felt on schedule.
	if (tier >= Tier::Garbage && !firstGarbageGranted
		&& elapsedSeconds - garbageTierEnteredAt >= FirstGarbageDelay)
	{
		firstGarbageGranted = true;
		return true;
	}

	if (pendingGarbageRows <= 0)
	{
		return false;
	}

	--pendingGarbageRows;
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
	if (!firstGoldenGranted && elapsedSeconds - chaosTierEnteredAt >= FirstGoldenDelay)
	{
		firstGoldenGranted = true;
		piecesSinceGolden = 0;
		return true;
	}

	++piecesSinceGolden;

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
