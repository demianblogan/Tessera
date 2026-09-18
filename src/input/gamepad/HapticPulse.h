#pragma once

#include "../../haptics/HapticSettings.h"
#include "GamepadHaptics.h"

// Thin adapters between the plain data in HapticSettings and the GamepadHaptics
// primitive, so call sites read as one line: Haptics::TriggerPulse(haptics, settings.hardDrop).
namespace Haptics
{
	inline void TriggerPulse(GamepadHaptics& haptics, const HapticSettings::Rumble& rumble)
	{
		haptics.PulseVibration(rumble.lowMotor, rumble.highMotor, rumble.duration);
	}

	[[nodiscard]] inline RGBColor ConvertToRGB(HapticSettings::Color color) noexcept
	{
		return { color.r, color.g, color.b };
	}

	inline void FlashLightbar(GamepadHaptics& haptics, HapticSettings::Color color,
		float durationSeconds, int blinks = 1)
	{
		haptics.PulseLightbar(ConvertToRGB(color), durationSeconds, blinks);
	}
}
