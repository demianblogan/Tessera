#pragma once

enum class BlockRenderStyle
{
    WithOutline,
    WithoutOutline
};

// Highest step the sound / music sliders go to (0..MaxVolumeStep).
inline constexpr unsigned int MaxVolumeStep = 10;

// Upper bound accepted for a saved frame-rate limit. 0 means "unlimited".
inline constexpr unsigned int MaxFrameRateLimit = 1000;

struct GameSettings
{
    // Bumped whenever the on-disk settings layout changes. A file written by a
    // different version is preserved as .corrupt and replaced with defaults.
    static constexpr int FormatVersion = 1;

    // --- Graphics:

    bool verticalSyncEnabled = true;
    unsigned int frameRateLimit = 0;
    BlockRenderStyle blockRenderStyle = BlockRenderStyle::WithOutline;

	// --- Audio:

    unsigned int soundVolume = MaxVolumeStep;
    unsigned int musicVolume = MaxVolumeStep;
};