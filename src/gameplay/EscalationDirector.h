#pragma once

// Governs how the single endless mode gets harder over a long run -- not the
// per-level gravity curve (GameplaySession's own job), but periodic twists
// layered on top as tiers pass, each cumulative with the ones before it:
//
//   Base       [0:00, 0:30)  -- nothing extra, the guideline curve alone.
//   SpeedSurge [0:30, 0:55)  -- periodic short gravity spikes.
//   Garbage    [0:55, 1:35)  -- a rising garbage row on a random timer.
//   Chaos      [1:35, inf)   -- adds a rare golden piece worth double.
//
// Every tier's *first* effect is deterministic and time-gated (not left to
// however fast the player happens to burn through spawns), so it reliably
// lands in a fixed window after entering the tier: a Speed Surge within
// 30-40s of the run starting, and the first golden piece within 120-130s.
// The first garbage row instead follows the same random 10-30s timer as
// every one after it (see GarbageMinInterval/GarbageMaxInterval) -- tying it
// to line clears instead read as arbitrary to the player (sometimes right
// after a clear, sometimes long after, with no felt rhythm either way).
//
// Headless and driven entirely by GameplaySession: it feeds elapsed time,
// line-clear counts and spawn notifications in, and reads back what to do
// this frame.
class EscalationDirector
{
public:
	enum class Tier
	{
		Base,
		SpeedSurge,
		Garbage,
		Chaos
	};

	struct Events
	{
		bool hasTierChanged = false;
		Tier tier = Tier::Base;

		bool hasSurgeStarted = false;
		bool hasSurgeEnded = false;
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

	// A garbage row arrives on a random timer once unlocked, re-rolled after
	// every row (including the first) -- a plain countdown, not tied to how
	// the player is actually playing.
	static constexpr float GarbageMinInterval = 10.f;
	static constexpr float GarbageMaxInterval = 30.f;

	// One golden piece every this many spawns, once unlocked. The first one
	// instead waits a fixed FirstGoldenDelay after entering the tier (lands it
	// at 95 + 30 = 2:05) -- spawning eight pieces that fast isn't guaranteed.
	static constexpr int PiecesPerGoldenPiece = 8;
	static constexpr float FirstGoldenDelay = 30.f;

	void Update(float deltaTime);

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

	[[nodiscard]] Tier CurrentTier() const;
	[[nodiscard]] float FallSpeedMultiplier() const;

private:
	void UpdateTier();
	void UpdateSurge(float deltaTime);
	void UpdateGarbage(float deltaTime);

	float elapsedSeconds = 0.f;
	Tier tier = Tier::Base;

	float surgeCooldown = FirstSurgeDelay;
	bool isSurgeActive = false;
	float surgeTimer = 0.f;

	// When Chaos was entered, so ShouldSpawnGoldenPiece() can time-gate the
	// tier's first effect instead of leaving it to the player's spawn rate.
	float chaosTierEnteredAt = -1.f;
	bool hasGrantedFirstGolden = false;

	// Counts down to the next garbage row once Garbage is reached; re-rolled
	// (GarbageMinInterval..GarbageMaxInterval) after every row, including the
	// first -- set negative so UpdateGarbage() rolls it the moment the tier
	// starts, rather than on the next Update() after that.
	float garbageCooldown = -1.f;
	int pendingGarbageRows = 0;
	int piecesSinceGolden = 0;

	Events pendingEvents;
};
