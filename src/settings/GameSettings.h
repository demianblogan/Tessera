#pragma once

#include <SFML/Window/Keyboard.hpp>

enum class BlockRenderStyle
{
    WithOutline,
    WithoutOutline
};

// Keyboard bindings for gameplay. Physical scancodes, so they survive a layout
// change. Not persisted yet -- the rebinding UI and its save/load land with the
// full Options screen (v1.4.0). Gamepad bindings are fixed and handled by
// GamepadManager.
struct ControlSettings
{
    sf::Keyboard::Scancode moveLeft = sf::Keyboard::Scancode::Left;
    sf::Keyboard::Scancode moveRight = sf::Keyboard::Scancode::Right;
    sf::Keyboard::Scancode softDrop = sf::Keyboard::Scancode::Down;
    sf::Keyboard::Scancode hardDrop = sf::Keyboard::Scancode::Space;
    sf::Keyboard::Scancode rotateClockwise = sf::Keyboard::Scancode::Up;
    sf::Keyboard::Scancode rotateCounterClockwise = sf::Keyboard::Scancode::Z;
    sf::Keyboard::Scancode pause = sf::Keyboard::Scancode::Escape;
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

    // --- Controls (not persisted yet):

    ControlSettings controls;
};