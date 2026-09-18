#pragma once

#include <optional>

#include <SFML/Window/Joystick.hpp>

namespace sf
{
	class Event;
}

namespace Haptics
{
	class GamepadHaptics;
}

class HapticSettings;

// Xbox / PlayStation controller support. Deliberately self-contained -- it does
// not go through ActionMap / InputBinding / InputHandler, which are built for
// discrete, rebindable keyboard/mouse input:
//   - a d-pad is a POV-hat axis, not a button;
//   - joystick button numbers aren't a stable named enum -- what index means
//     which face button depends on the controller vendor;
//   - the left stick is a continuous axis with a dead zone, not an OnPress.
// Menu navigation (NavigationAction) also has no keyboard/mouse equivalent.
class GamepadManager
{
public:
	enum class Layout
	{
		Xbox,
		PlayStation,
		Generic
	};

	enum class NavigationAction
	{
		None,
		Up,
		Down,
		Left,
		Right,
		Confirm,
		Back
	};

	GamepadManager();

	// Optional. When set, GetNavigationAction() also pulses a faint vibration
	// on every menu move / press it reports, so every menu gets the same
	// tactile feedback for free. Not owned; must outlive this object.
	void SetHaptics(Haptics::GamepadHaptics* haptics) noexcept;

	// Optional. Supplies the menu-navigation rumble/lightbar values used with the
	// haptics above. Not owned; must outlive this object. When unset, built-in
	// fallback values are used so the class still works standalone.
	void SetHapticSettings(const HapticSettings* hapticSettings) noexcept;

	// Feed every polled sf::Event so connection changes and "is the player
	// using the gamepad right now" tracking stay current.
	void HandleEvent(const sf::Event& event);

	// Poll once per frame, before states read the gameplay queries below --
	// computes the button press edges by comparing frames.
	void Update();

	// --- Menus (edge-triggered, from one event) ---
	[[nodiscard]] NavigationAction GetNavigationAction(const sf::Event& event) const;
	[[nodiscard]] bool IsPausePressed(const sf::Event& event) const;

	// --- Gameplay (polled) ---
	[[nodiscard]] int GetHorizontalDirection() const;   // -1 / 0 / +1  (d-pad X or left stick X)
	[[nodiscard]] bool IsSoftDropHeld() const;          // true for as long as d-pad down or left stick down is held

	// Edge-triggered, not a held state: true for exactly one Update() frame --
	// the frame the button transitions from up to down -- then false again even
	// while the player keeps holding it. Call once per frame from Update()'s
	// caller; polling it more than once a frame, or skipping a frame, misses
	// the press.
	[[nodiscard]] bool WasHardDropPressed() const noexcept;               // A / Cross
	[[nodiscard]] bool WasRotateClockwisePressed() const noexcept;        // right bumper
	[[nodiscard]] bool WasRotateCounterClockwisePressed() const noexcept; // left bumper
	[[nodiscard]] bool WasHoldPressed() const noexcept;                   // Y / Triangle

	[[nodiscard]] bool IsInUse() const noexcept;
	[[nodiscard]] Layout GetLayout() const noexcept;

private:
	void RefreshConnection();
	[[nodiscard]] bool IsActiveJoystick(unsigned int joystickID) const noexcept;
	[[nodiscard]] bool IsButtonPressed(unsigned int button) const;
	[[nodiscard]] bool IsButtonInEvent(const sf::Event& event, unsigned int button) const;
	[[nodiscard]] float ReadAxis(sf::Joystick::Axis axis) const;

	[[nodiscard]] NavigationAction ComputeNavigationAction(const sf::Event& event) const;

	std::optional<unsigned int> activeJoystick;
	Layout layout = Layout::Generic;
	bool isInUse = false;

	Haptics::GamepadHaptics* haptics = nullptr;
	const HapticSettings* hapticSettings = nullptr;

	bool wasHardDropDown = false;
	bool wasRightBumperDown = false;
	bool wasLeftBumperDown = false;
	bool wasHoldDown = false;
	bool isHardDropEdge = false;
	bool isRotateClockwiseEdge = false;
	bool isRotateCounterClockwiseEdge = false;
	bool isHoldEdge = false;
};
