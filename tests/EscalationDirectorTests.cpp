#include "doctest/doctest.h"

#include "gameplay/EscalationDirector.h"

TEST_CASE("the run starts in the Base tier with no fall-speed boost")
{
	EscalationDirector director;

	CHECK(director.CurrentTier() == EscalationDirector::Tier::Base);
	CHECK(director.FallSpeedMultiplier() == doctest::Approx(1.f));
}

TEST_CASE("the tier advances at each of its time thresholds, reported once as an event")
{
	EscalationDirector director;

	director.Update(EscalationDirector::SpeedSurgeTierStart - 1.f);
	CHECK(director.CurrentTier() == EscalationDirector::Tier::Base);
	CHECK_FALSE(director.ConsumeEvents().tierChanged);

	director.Update(2.f);   // crosses into SpeedSurge
	const EscalationDirector::Events afterSurge = director.ConsumeEvents();
	CHECK(director.CurrentTier() == EscalationDirector::Tier::SpeedSurge);
	CHECK(afterSurge.tierChanged);
	CHECK(afterSurge.tier == EscalationDirector::Tier::SpeedSurge);

	// No further change mid-tier.
	director.Update(1.f);
	CHECK_FALSE(director.ConsumeEvents().tierChanged);

	director.Update(EscalationDirector::GarbageTierStart - EscalationDirector::SpeedSurgeTierStart - 2.f);
	CHECK(director.CurrentTier() == EscalationDirector::Tier::Garbage);

	director.Update(EscalationDirector::ChaosTierStart - EscalationDirector::GarbageTierStart);
	CHECK(director.CurrentTier() == EscalationDirector::Tier::Chaos);
}

TEST_CASE("Speed Surge only fires once the SpeedSurge tier is reached")
{
	EscalationDirector director;

	// Still Base -- right up to the tier boundary, nothing should happen.
	director.Update(EscalationDirector::SpeedSurgeTierStart - 1.f);
	CHECK_FALSE(director.FallSpeedMultiplier() > 1.f);

	(void)director.ConsumeEvents();
}

TEST_CASE("Speed Surge fires periodically once unlocked, doubles fall speed while active, then ends")
{
	EscalationDirector director;

	// Approach the tier boundary with a big jump (harmless -- Base tier never
	// touches the surge cooldown), then cross it with a single small step, the
	// way a real frame would -- a big step straddling the boundary would hand
	// its whole delta to the surge countdown under the tier it lands in.
	director.Update(EscalationDirector::SpeedSurgeTierStart - 0.02f);
	(void)director.ConsumeEvents();
	director.Update(0.02f);
	(void)director.ConsumeEvents();

	// Just short of the first (shorter) delay: not yet.
	director.Update(EscalationDirector::FirstSurgeDelay - 0.1f);
	CHECK_FALSE(director.ConsumeEvents().surgeStarted);
	CHECK(director.FallSpeedMultiplier() == doctest::Approx(1.f));

	// Crossing it starts the surge.
	director.Update(0.2f);
	CHECK(director.ConsumeEvents().surgeStarted);
	CHECK(director.FallSpeedMultiplier() == doctest::Approx(EscalationDirector::SurgeFallMultiplier));

	// It ends after SurgeDuration.
	director.Update(EscalationDirector::SurgeDuration + 0.1f);
	CHECK(director.ConsumeEvents().surgeEnded);
	CHECK(director.FallSpeedMultiplier() == doctest::Approx(1.f));

	// The next one waits a full SurgeInterval, not the shorter first delay.
	director.Update(EscalationDirector::SurgeInterval - 0.1f);
	CHECK_FALSE(director.ConsumeEvents().surgeStarted);

	director.Update(0.2f);
	CHECK(director.ConsumeEvents().surgeStarted);
}

TEST_CASE("no garbage row is queued before the Garbage tier, however long the run goes")
{
	EscalationDirector director;
	director.Update(EscalationDirector::GarbageTierStart - 1.f);
	(void)director.ConsumeEvents();
	CHECK_FALSE(director.ConsumePendingGarbageRow());
}

TEST_CASE("a garbage row is queued within GarbageMaxInterval of entering the Garbage tier")
{
	EscalationDirector director;

	// Approach the tier boundary in two steps, the second one small: a single
	// big jump across it would hand UpdateGarbage() that whole jump's delta
	// time as if it were all spent inside the tier, overcounting the first
	// interval and firing it early.
	director.Update(EscalationDirector::GarbageTierStart - 1.f);
	director.Update(1.01f);
	(void)director.ConsumeEvents();

	director.Update(EscalationDirector::GarbageMaxInterval + 0.1f);

	CHECK(director.ConsumePendingGarbageRow());
	CHECK_FALSE(director.ConsumePendingGarbageRow());   // only the one row
}

TEST_CASE("garbage rows keep coming on a re-rolled timer, one at a time")
{
	EscalationDirector director;
	director.Update(EscalationDirector::GarbageTierStart - 1.f);
	director.Update(1.01f);
	(void)director.ConsumeEvents();
	(void)director.ConsumePendingGarbageRow();   // drain whatever entering the tier itself queued

	// The interval is randomised, so check one row at a time across several
	// intervals' worth of elapsed time, rather than asserting an exact count.
	for (int i = 0; i < 5; ++i)
	{
		director.Update(EscalationDirector::GarbageMaxInterval + 0.1f);
		CHECK(director.ConsumePendingGarbageRow());
		CHECK_FALSE(director.ConsumePendingGarbageRow());
	}
}

TEST_CASE("golden pieces are only offered every PiecesPerGoldenPiece spawns in the Chaos tier")
{
	EscalationDirector director;

	// Before Chaos: never golden, however many times asked.
	director.Update(EscalationDirector::ChaosTierStart - 1.f);
	(void)director.ConsumeEvents();
	for (int i = 0; i < EscalationDirector::PiecesPerGoldenPiece * 2; i++)
	{
		CHECK_FALSE(director.ShouldSpawnGoldenPiece());
	}

	director.Update(2.f);   // now Chaos
	(void)director.ConsumeEvents();

	for (int i = 0; i < EscalationDirector::PiecesPerGoldenPiece - 1; i++)
	{
		CHECK_FALSE(director.ShouldSpawnGoldenPiece());
	}
	CHECK(director.ShouldSpawnGoldenPiece());

	// The counter restarts after firing.
	for (int i = 0; i < EscalationDirector::PiecesPerGoldenPiece - 1; i++)
	{
		CHECK_FALSE(director.ShouldSpawnGoldenPiece());
	}
	CHECK(director.ShouldSpawnGoldenPiece());
}

TEST_CASE("the first golden piece is granted on a timer even with barely any spawns")
{
	EscalationDirector director;
	director.Update(EscalationDirector::ChaosTierStart + 0.01f);
	(void)director.ConsumeEvents();

	// A couple of spawns, nowhere near PiecesPerGoldenPiece -- still no.
	CHECK_FALSE(director.ShouldSpawnGoldenPiece());
	CHECK_FALSE(director.ShouldSpawnGoldenPiece());

	// Just short of the deadline, nothing yet.
	director.Update(EscalationDirector::FirstGoldenDelay - 0.1f);
	CHECK_FALSE(director.ShouldSpawnGoldenPiece());

	// Crossing it grants the very next spawn, however few pieces that took.
	director.Update(0.2f);
	CHECK(director.ShouldSpawnGoldenPiece());

	// It only ever fires once this way -- the counter takes over from here.
	for (int i = 0; i < EscalationDirector::PiecesPerGoldenPiece - 1; i++)
	{
		CHECK_FALSE(director.ShouldSpawnGoldenPiece());
	}
	CHECK(director.ShouldSpawnGoldenPiece());
}
