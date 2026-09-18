#pragma once

#include <SFML/Window/Keyboard.hpp>

#include "../display/DisplaySettings.h"
#include "../localization/Language.h"

// Keyboard bindings for gameplay. Physical scancodes, so they survive a layout
// change. The rebindable actions are edited by the Controls > Keyboard panel
// and persisted (`hold` is wired and saved from v1.6.0 on, but its own
// Keyboard-panel row lands later). `pause` is fixed to Escape (the universal
// menu/exit key) and is neither shown nor saved. Gamepad bindings are fixed
// (GamepadManager).
struct ControlSettings
{
	sf::Keyboard::Scancode moveTetrominoLeft = sf::Keyboard::Scancode::Left;
	sf::Keyboard::Scancode moveTetrominoRight = sf::Keyboard::Scancode::Right;
	sf::Keyboard::Scancode softDropTetromino = sf::Keyboard::Scancode::Down;
	sf::Keyboard::Scancode hardDropTetromino = sf::Keyboard::Scancode::Space;
	sf::Keyboard::Scancode rotateTetrominoClockwise = sf::Keyboard::Scancode::E;
	sf::Keyboard::Scancode rotateTetrominoCounterClockwise = sf::Keyboard::Scancode::Q;
	sf::Keyboard::Scancode holdTetromino = sf::Keyboard::Scancode::C;
	sf::Keyboard::Scancode pauseGame = sf::Keyboard::Scancode::Escape;
};

// Highest step the sound / music sliders go to (0..MaxVolumeStep).
inline constexpr unsigned int MaxVolumeStep = 10;

// Valid range for the next-queue-length setting: 1 (always show at least the
// piece about to spawn) to 5 -- the NEXT HUD panel was laid out to fit at
// most 5 upcoming-piece icons, so a 6th would overflow it. Kept in step with
// GameplaySession::MinNextQueueLength / MaxNextQueueLength, which clamp the
// same value again at the gameplay-rules layer.
inline constexpr unsigned int MinNextQueueLength = 1;
inline constexpr unsigned int MaxNextQueueLength = 5;

struct GameSettings
{
	// Bumped whenever the on-disk settings layout changes. A file written by a
	// different version is preserved as .corrupt and replaced with defaults.
	static constexpr int FormatVersion = 10;

	// --- Graphics:
	Display::Settings display;
	bool isVerticalSyncEnabled = true;
	bool needToShowFPS = false;
	bool isCRTFilterEnabled = true;

	// --- Audio:
	unsigned int soundVolume = MaxVolumeStep;
	unsigned int musicVolume = MaxVolumeStep;

	// --- Controls:
	ControlSettings controls;

	// --- Gameplay:
	bool isGamepadVibrationEnabled = true;
	bool isGamepadLightbarEnabled = true;
	bool isScreenShakeEnabled = true;
	bool isGhostPieceEnabled = true;
	bool isHoldTetrominoEnabled = true;
	unsigned int nextQueueLength = MaxNextQueueLength; // clamped to [MinNextQueueLength, MaxNextQueueLength]

	// The guideline "7-bag" randomiser: each bag is a shuffled set of all
	// seven piece types, dealt out before the next bag is shuffled, so a
	// piece can repeat at most once in a row and every type is guaranteed to
	// reappear within 7 spawns. False deals pure uniform-random types
	// instead, which can streak or drought any piece for an arbitrarily long
	// stretch.
	bool isSevenBagEnabled = true;

	// --- HUD: which in-game panels are shown.
	bool isHoldTetrominoPanelVisible = true;
	bool isNextTetrominoPanelVisible = true;
	bool isScorePanelVisible = true;
	bool isLinesPanelVisible = true;
	bool isLevelPanelVisible = true;
	bool isTimePanelVisible = true;
	bool isControlsLegendPanelVisible = true;

	// --- Language:
	Language language = Language::English;

	// False only until the first-run picker has run once; distinguishes
	// "never chosen" from "explicitly chose English".
	bool isLanguageChosen = false;
};
