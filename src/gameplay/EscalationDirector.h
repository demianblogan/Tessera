#pragma once

// Governs how the single endless mode gets harder over a long run -- not the
// per-level gravity curve (GameplaySession's own job), but periodic twists
// layered on top as tiers pass, each cumulative with the ones before it:
//
//   Base       [0:00, 0:30)  -- nothing extra, the guideline curve alone.
//   SpeedSurge [0:30, 0:55)  -- periodic short gravity spikes.
//   Garbage    [0:55, 1:35)  -- adds a rising garbage row every so often.
//   Chaos      [1:35, inf)   -- adds a rare golden piece worth double.
//
// Every tier's *first* effect is deterministic and time-gated (not left to
// however fast the player happens to clear lines or burn through spawns), so
// it reliably lands in a fixed window after entering the tier: a Speed Surge
// within 30-40s of the run starting, the first garbage row within 60-70s, and
// the first golden piece within 120-130s. Every effect after the first one
// falls back to its normal cadence (a fixed interval, a line-clear count, a
// spawn count).
//
// Headless and driven entirely by GameplaySession: it feeds elapsed time,
// line-clear counts and spawn notifications in, and reads back what to do
// this frame.
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

	static constexpr float SpeedSurgeTierStart = 30.f;
	static constexpr float GarbageTierStart = 55.f;
	static constexpr float ChaosTierStart = 95.f;

	// A surge fires this often once unlocked, doubling fall speed for its
	// duration. The very first one after entering the tier comes sooner
	// (FirstSurgeDelay: lands it at 30 + 8 = 0:38), so the tier starting is
	// actually felt close to when it starts.
	static constexpr float SurgeInterval = 35.f;
	static constexpr float FirstSurgeDelay = 8.f;
	static constexpr float SurgeDuration = 5.f;
	static constexpr float SurgeFallMultiplier = 2.f;

	// One garbage row per this many lines cleared, once unlocked. The first one
	// instead waits a fixed FirstGarbageDelay after entering the tier (lands it
	// at 55 + 10 = 1:05) -- clearing four real lines that fast isn't guaranteed.
	static constexpr int LinesPerGarbageRow = 4;
	static constexpr float FirstGarbageDelay = 10.f;

	// One golden piece every this many spawns, once unlocked. The first one
	// instead waits a fixed FirstGoldenDelay after entering the tier (lands it
	// at 95 + 30 = 2:05) -- spawning eight pieces that fast isn't guaranteed.
	static constexpr int PiecesPerGoldenPiece = 8;
	static constexpr float FirstGoldenDelay = 30.f;

	void Update(float deltaTime);

	// Call once per real line clear (i.e. once per resolved ClearingRows, with
	// however many rows it took), so Garbage-tier counting matches actual play.
	// Queues a garbage row for every LinesPerGarbageRow crossed -- a burst clear
	// can queue more than one, drained one at a time by ConsumePendingGarbageRow.
	// The very first garbage row ignores this and is granted once
	// FirstGarbageDelay has passed since entering the tier, regardless of
	// whether four lines have actually cleared yet.
	void NotifyLinesCleared(int rowCount);

	// True (at most once per call) if a queued garbage row is waiting; the
	// caller applies exactly one to the board right before the next spawn, so
	// it never appears out from under a piece mid-fall.
	[[nodiscard]] bool ConsumePendingGarbageRow();

	// Call right as a new piece is about to spawn (not for the very first piece
	// of a session, and not when Hold() swaps one in) -- true means this spawn
	// should be golden. A plain counter, not a coin flip: deterministic and
	// exactly PiecesPerGoldenPiece apart once Chaos is reached. The very first
	// one ignores the counter and is granted once FirstGoldenDelay has passed
	// since entering Chaos, regardless of how many pieces have spawned since.
	[[nodiscard]] bool ShouldSpawnGoldenPiece();

	[[nodiscard]] Events ConsumeEvents();

	[[nodiscard]] Tier CurrentTier() const { return tier; }
	[[nodiscard]] float FallSpeedMultiplier() const { return surgeActive ? SurgeFallMultiplier : 1.f; }

private:
	void UpdateTier();
	void UpdateSurge(float deltaTime);

	float elapsedSeconds = 0.f;
	Tier tier = Tier::Base;

	float surgeCooldown = FirstSurgeDelay;
	bool surgeActive = false;
	float surgeTimer = 0.f;

	// When Garbage/Chaos were entered, so NotifyLinesCleared() and
	// ShouldSpawnGoldenPiece() can time-gate each tier's first effect instead
	// of leaving it to the player's clear/spawn rate.
	float garbageTierEnteredAt = -1.f;
	float chaosTierEnteredAt = -1.f;
	bool firstGarbageGranted = false;
	bool firstGoldenGranted = false;

	int linesSinceGarbage = 0;
	int pendingGarbageRows = 0;
	int piecesSinceGolden = 0;

	Events pendingEvents;
};
