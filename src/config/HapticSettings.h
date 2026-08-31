#pragma once

#include <cstdint>
#include <filesystem>

// Design-time "feel" tuning, loaded once from assets/data/haptics.json. Like
// AudioBalance, this is authoring data, not a player setting -- the player-facing
// vibration toggle lands later with the Options screen. It exists so the rumble
// strengths, the DualSense lightbar colours, the active-piece glow tint and the
// horizontal-move repeat timing can all be adjusted without a rebuild: edit the
// JSON, restart.
//
// A missing file, a missing section or a missing key falls back to the
// compiled-in defaults below, so the game still runs with no JSON at all.
class HapticSettings
{
public:
	// One vibration burst. lowMotor is the heavier/rumblier motor, highMotor the
	// sharper/buzzier one; both 0..1. duration is the linear fade-out in seconds.
	struct Rumble
	{
		float lowMotor = 0.f;
		float highMotor = 0.f;
		float duration = 0.f;
	};

	// Plain 0..255 RGB, matching Haptics::RGBColor without dragging in the HID
	// headers that GamepadHaptics.h pulls.
	struct Colour
	{
		std::uint8_t r = 0;
		std::uint8_t g = 0;
		std::uint8_t b = 0;
	};

	HapticSettings() = default;
	explicit HapticSettings(const std::filesystem::path& path);

	// --- Rumble presets -------------------------------------------------------

	// Menus. The navigation tick fires far more often than anything else, so it
	// is the softest.
	Rumble menuNavigation{ 0.05f, 0.12f, 0.05f };
	// The title letters: a short pulse whose strength grows across "TESSERA"
	// from `titleLetterBase` (first letter) to base + grow (last letter). The
	// grow entry's duration is unused.
	Rumble titleLetterBase{ 0.12f, 0.24f, 0.05f };
	Rumble titleLetterGrow{ 0.40f, 0.50f, 0.f };
	// Each ring entry as it flies in.
	Rumble menuEntryFlyIn{ 0.28f, 0.42f, 0.06f };

	// Gameplay.
	Rumble pieceLanded{ 0.20f, 0.40f, 0.06f };
	Rumble hardDrop{ 0.55f, 0.50f, 0.12f };
	Rumble wallHit{ 0.15f, 0.30f, 0.05f };
	Rumble rowCleared{ 0.40f, 0.50f, 0.16f };
	Rumble tetris{ 0.75f, 0.70f, 0.28f };   // a four-line clear
	Rumble levelUp{ 0.30f, 0.45f, 0.10f };
	Rumble gameOver{ 0.90f, 0.90f, 0.50f };

	// --- DualSense lightbar (pure primaries -- the diffuser tints anything
	//     off-axis, so e.g. a green with any blue reads as cyan) --------------

	Colour menuLightbar{ 255, 255, 0 };       // yellow
	Colour rowClearLightbar{ 0, 255, 0 };     // green
	Colour gameOverLightbar{ 255, 0, 0 };     // red

	// --- Other feel ---------------------------------------------------------

	// The even neon edge-glow around the active tetromino.
	Colour activePieceGlow{ 120, 210, 255 };

	// Horizontal-move repeat (the classic DAS / ARR), in seconds: how long a
	// direction is held before auto-repeat starts, then the gap between steps.
	float delayedAutoShift = 0.17f;
	float autoRepeatRate = 0.03f;
};
