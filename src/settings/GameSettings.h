#pragma once

#include <SFML/Window/Keyboard.hpp>

#include "../display/DisplayMode.h"

// Keyboard bindings for gameplay. Physical scancodes, so they survive a layout
// change. The rebindable actions are edited by the Controls > Keyboard panel
// and persisted (`hold` is wired and saved from v1.6.0 on, but its own
// Keyboard-panel row lands later). `pause` is fixed to Escape (the universal
// menu/exit key) and is neither shown nor saved. Gamepad bindings are fixed
// (GamepadManager).
struct ControlSettings
{
    sf::Keyboard::Scancode moveLeft = sf::Keyboard::Scancode::Left;
    sf::Keyboard::Scancode moveRight = sf::Keyboard::Scancode::Right;
    sf::Keyboard::Scancode softDrop = sf::Keyboard::Scancode::Down;
    sf::Keyboard::Scancode hardDrop = sf::Keyboard::Scancode::Space;
    sf::Keyboard::Scancode rotateClockwise = sf::Keyboard::Scancode::E;
    sf::Keyboard::Scancode rotateCounterClockwise = sf::Keyboard::Scancode::Q;
    sf::Keyboard::Scancode hold = sf::Keyboard::Scancode::C;
    sf::Keyboard::Scancode pause = sf::Keyboard::Scancode::Escape;
};

// Highest step the sound / music sliders go to (0..MaxVolumeStep).
inline constexpr unsigned int MaxVolumeStep = 10;

struct GameSettings
{
    // Bumped whenever the on-disk settings layout changes. A file written by a
    // different version is preserved as .corrupt and replaced with defaults.
    static constexpr int FormatVersion = 8;

    // --- Graphics:

    Display::Mode display;
    bool verticalSyncEnabled = true;
    bool showFps = false;
    bool crtFilterEnabled = true;

    // --- Audio:

    unsigned int soundVolume = MaxVolumeStep;
    unsigned int musicVolume = MaxVolumeStep;

    // --- Controls:

    ControlSettings controls;

    // --- Gameplay:

    bool gamepadVibrationEnabled = true;
    bool gamepadLightbarEnabled = true;
    bool screenShakeEnabled = true;

    // --- HUD: which in-game panels are shown.

    bool hudHold = true;
    bool hudNext = true;
    bool hudScore = true;
    bool hudLines = true;
    bool hudLevel = true;
    bool hudTime = true;
    bool hudControlsLegend = true;
};
