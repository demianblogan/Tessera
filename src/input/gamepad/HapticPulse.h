#pragma once

#include "../../config/HapticSettings.h"
#include "GamepadHaptics.h"

// Thin adapters between the plain data in HapticSettings and the GamepadHaptics
// primitive, so call sites read as one line: Haptics::Pulse(haptics, settings.hardDrop).
namespace Haptics
{
	inline void Pulse(GamepadHaptics& haptics, const HapticSettings::Rumble& rumble)
	{
		haptics.PulseVibration(rumble.lowMotor, rumble.highMotor, rumble.duration);
	}

	[[nodiscard]] inline RGBColor ToRgb(HapticSettings::Colour colour) noexcept
	{
		return { colour.r, colour.g, colour.b };
	}

	inline void FlashLightbar(GamepadHaptics& haptics, HapticSettings::Colour colour,
		float durationSeconds, int blinks = 1)
	{
		haptics.PulseLightbar(ToRgb(colour), durationSeconds, blinks);
	}
}
