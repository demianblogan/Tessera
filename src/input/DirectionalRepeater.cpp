#include "DirectionalRepeater.h"

DirectionalRepeater::DirectionalRepeater(Timing timing) noexcept
	: timing(timing)
{
}

int DirectionalRepeater::Update(int heldDirection, float deltaTime)
{
	if (heldDirection == 0)
	{
		Reset();
		return 0;
	}

	if (heldDirection != activeDirection)
	{
		// Fresh press or a reversal: one immediate step, then start charging.
		activeDirection = heldDirection;
		heldTime = 0.f;
		repeatAccumulator = 0.f;
		return heldDirection;
	}

	heldTime += deltaTime;
	if (heldTime < timing.delayedAutoShift)
	{
		return 0;
	}

	if (timing.autoRepeatRate <= 0.f)
	{
		// "Instant" auto-repeat: shift as far as the caller allows this frame.
		return activeDirection * 16;
	}

	repeatAccumulator += deltaTime;

	int steps = 0;
	while (repeatAccumulator >= timing.autoRepeatRate && steps < 16)
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
