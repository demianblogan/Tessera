#pragma once

// Turns a "this direction is held" state into a stream of 1-cell move steps
// with the classic falling-block timing: one step immediately on press, then a
// pause (Delayed Auto Shift), then repeats at a fixed rate (Auto Repeat Rate).
// Reversing direction restarts the charge. Used for left/right movement; soft
// drop uses its own simpler timer in the gameplay loop.
class DirectionalRepeater
{
public:
	struct Timing
	{
		float delayedAutoShift = 0.17f;   // seconds held before auto-repeat begins
		float autoRepeatRate = 0.03f;     // seconds between auto-repeat steps
	};

	explicit DirectionalRepeater(Timing timing = {}) noexcept;

	// Call once per frame with the current held direction (-1, 0, +1). Returns
	// the signed number of cells to move this frame.
	[[nodiscard]] int Update(int heldDirection, float deltaTime);

	void Reset() noexcept;

private:
	Timing timing;

	// The direction currently "charged" (-1, 0, +1). A change from one held
	// direction straight to the other -- not just release -- counts as a fresh
	// press and restarts the charge below.
	int activeDirection = 0;

	// How long activeDirection has been held without a reset, counted up to
	// timing.delayedAutoShift before auto-repeat is allowed to start.
	float heldTime = 0.f;

	// Time banked toward the next auto-repeat step once the DAS delay has
	// passed. Accumulating instead of stepping once per frame means a single
	// long frame (a stall, a lost window focus) still produces the right
	// number of steps instead of dropping them.
	float repeatAccumulator = 0.f;
};
