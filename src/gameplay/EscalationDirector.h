#pragma once

// Governs how the single endless mode gets harder over a long run -- not the
// per-level gravity curve (GameplaySession's own job), but periodic twists
// layered on top as tiers pass, each cumulative with the ones before it:
//
//   Base       [0:00, 1:00)  -- nothing extra, the guideline curve alone.
//   SpeedSurge [1:00, 3:00)  -- periodic short gravity spikes.
//   Garbage    [3:00, 5:00)  -- adds a rising garbage row every so often.
//   Chaos      [5:00, inf)   -- adds a rare golden piece worth double.
//
// Headless and driven entirely by GameplaySession: it feeds elapsed time,
// line-clear counts and spawn notifications in, and reads back what to do
// this frame. Timing is deterministic (real time and simple counters, no
// randomness) so a run's escalation schedule is reproducible and testable.
class EscalationDirector
{
public:
	enum class Tier { Base, SpeedSurge, Garbage, Chaos };

	struct Events
	{
		bool tierChanged = false;
		Tier tier = Tier::Base;

		bool surgeStarted = false;
		bool surgeEnded = false;
	};

	static constexpr float SpeedSurgeTierStart = 60.f;
	static constexpr float GarbageTierStart = 180.f;
	static constexpr float ChaosTierStart = 300.f;

	// A surge fires this often once unlocked, doubling fall speed for its
	// duration. The first one waits a full interval too, so a run doesn't open
	// its new tier with an immediate spike.
	static constexpr float SurgeInterval = 35.f;
	static constexpr float SurgeDuration = 2.f;
	static constexpr float SurgeFallMultiplier = 2.f;

	// One garbage row per this many lines cleared, once unlocked.
	static constexpr int LinesPerGarbageRow = 4;

	// One golden piece every this many spawns, once unlocked.
	static constexpr int PiecesPerGoldenPiece = 8;

	void Update(float deltaTime);

	// Call once per real line clear (i.e. once per resolved ClearingRows, with
	// however many rows it took), so Garbage-tier counting matches actual play.
	// Queues a garbage row for every LinesPerGarbageRow crossed -- a burst clear
	// can queue more than one, drained one at a time by ConsumePendingGarbageRow.
	void NotifyLinesCleared(int rowCount);

	// True (at most once per call) if a queued garbage row is waiting; the
	// caller applies exactly one to the board right before the next spawn, so
	// it never appears out from under a piece mid-fall.
	[[nodiscard]] bool ConsumePendingGarbageRow();

	// Call right as a new piece is about to spawn (not for the very first piece
	// of a session, and not when Hold() swaps one in) -- true means this spawn
	// should be golden. A plain counter, not a coin flip: deterministic and
	// exactly PiecesPerGoldenPiece apart once Chaos is reached.
	[[nodiscard]] bool ShouldSpawnGoldenPiece();

	[[nodiscard]] Events ConsumeEvents();

	[[nodiscard]] Tier CurrentTier() const { return tier; }
	[[nodiscard]] float FallSpeedMultiplier() const { return surgeActive ? SurgeFallMultiplier : 1.f; }

private:
	void UpdateTier();
	void UpdateSurge(float deltaTime);

	float elapsedSeconds = 0.f;
	Tier tier = Tier::Base;

	float surgeCooldown = SurgeInterval;
	bool surgeActive = false;
	float surgeTimer = 0.f;

	int linesSinceGarbage = 0;
	int pendingGarbageRows = 0;
	int piecesSinceGolden = 0;

	Events pendingEvents;
};
