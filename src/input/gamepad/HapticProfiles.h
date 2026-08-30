#pragma once

#include "GamepadHaptics.h"

// Named vibration + lightbar "feel" presets for Tessera's own events. Kept
// separate from GamepadHaptics so that class stays a generic, portable
// primitive with no opinion about what any particular game should feel like.
namespace HapticProfiles
{
	struct Rumble
	{
		float lowMotor = 0.f;   // heavier / rumblier motor
		float highMotor = 0.f;  // sharper / buzzier motor
		float duration = 0.f;
	};

	// --- Menus ---
	// A barely-there tick on every navigation step (fires far more often than
	// anything else, so it has to be the softest).
	inline constexpr Rumble MenuNavigation{ 0.05f, 0.12f, 0.05f };

	// --- Gameplay ---
	inline constexpr Rumble PieceLanded{ 0.20f, 0.40f, 0.06f };
	inline constexpr Rumble HardDrop{ 0.55f, 0.50f, 0.12f };
	inline constexpr Rumble WallHit{ 0.15f, 0.30f, 0.05f };
	inline constexpr Rumble RowCleared{ 0.40f, 0.50f, 0.16f };
	inline constexpr Rumble Tetris{ 0.75f, 0.70f, 0.28f };   // a four-line clear
	inline constexpr Rumble LevelUp{ 0.30f, 0.45f, 0.10f };
	inline constexpr Rumble GameOver{ 0.90f, 0.90f, 0.50f };

	// --- Lightbar (DualSense only). Pure primaries -- the diffuser tints
	//     anything off-axis, so e.g. a green with any blue reads as cyan. ---
	inline constexpr Haptics::RGBColor MenuLightbar{ 255, 255, 0 };     // yellow
	inline constexpr Haptics::RGBColor RowClearLightbar{ 0, 255, 0 };   // green
	inline constexpr Haptics::RGBColor GameOverLightbar{ 255, 0, 0 };   // red

	inline void Play(Haptics::GamepadHaptics& haptics, const Rumble& rumble)
	{
		haptics.PulseVibration(rumble.lowMotor, rumble.highMotor, rumble.duration);
	}
}
