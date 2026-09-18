#include "DirectionalRepeater.h"

namespace
{
	// Ceiling on how many cells a single Update() call can report, whether from
	// a burst of banked auto-repeat steps after a long frame or from the
	// "instant" auto-repeat rate below. The board is only 10 cells wide, so
	// this is already far more than a single frame could ever need to apply.
	constexpr int MaxStepsPerFrame = 16;
}

DirectionalRepeater::DirectionalRepeater(Timing timing) noexcept
	: timing(timing)
{}

int DirectionalRepeater::Update(int heldDirection, float deltaTime)
{
	// Nothing held -- drop any charge so the next press starts clean.
	if (heldDirection == 0)
	{
		Reset();
		return 0;
	}

	// Fresh press, or a reversal straight from one direction to the other:
	// one immediate step, then start charging DAS from zero.
	if (heldDirection != activeDirection)
	{
		activeDirection = heldDirection;
		heldTime = 0.f;
		repeatAccumulator = 0.f;
		return heldDirection;
	}

	// Still charging toward the DAS delay -- not moving yet.
	heldTime += deltaTime;
	if (heldTime < timing.delayedAutoShift)
		return 0;

	// An auto-repeat rate of zero has no meaningful "steps per second" -- treat
	// it as "instant": shift as far as the caller allows in one frame.
	if (timing.autoRepeatRate <= 0.f)
		return activeDirection * MaxStepsPerFrame;

	// Past the DAS delay: bank this frame's time and convert whole
	// autoRepeatRate intervals into steps, carrying any leftover forward so
	// repeat timing stays accurate however deltaTime happens to land.
	repeatAccumulator += deltaTime;

	int steps = 0;
	while (repeatAccumulator >= timing.autoRepeatRate && steps < MaxStepsPerFrame)
	{
		repeatAccumulator -= timing.autoRepeatRate;
		steps++;
	}

	return steps * activeDirection;
}

void DirectionalRepeater::Reset() noexcept
{
	activeDirection = 0;
	heldTime = 0.f;
	repeatAccumulator = 0.f;
}
