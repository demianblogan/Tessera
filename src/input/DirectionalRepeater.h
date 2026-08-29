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
	int activeDirection = 0;
	float heldTime = 0.f;
	float repeatAccumulator = 0.f;
};
