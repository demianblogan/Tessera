#include "GamepadManager.h"

#include <cmath>

#include <SFML/Window/Event.hpp>
#include <SFML/Window/Joystick.hpp>

#include "gamepad/GamepadHaptics.h"
#include "gamepad/HapticPulse.h"

namespace
{
	// USB vendor IDs assigned by the USB-IF -- Microsoft's and Sony's fixed,
	// documented values, used only to pick the right button layout.
	constexpr unsigned int MicrosoftVendorID = 0x045Eu;
	constexpr unsigned int SonyVendorID = 0x054Cu;

	// Fraction of a stick's travel ignored before input registers, on SFML's
	// -100..100 axis scale.
	constexpr float StickDeadZone = 18.f;

	// How far past centre counts as "the player is actively using the gamepad".
	constexpr float UsageThreshold = 25.f;

	// How far a d-pad / stick has to move to count as a discrete direction.
	constexpr float DirectionThreshold = 55.f;

	// The bottom face button: A on an Xbox pad (button 0), Cross on a
	// DualShock / DualSense (button 1 -- button 0 there is Square). SFML
	// exposes raw indices whose meaning depends on the controller.
	constexpr unsigned int XboxHardDropButton = 0u;
	constexpr unsigned int PlayStationHardDropButton = 1u;

	// Shoulder buttons -- LB / RB (Xbox) and L1 / R1 (PlayStation) share these
	// raw indices. Rotation is on the bumpers, not the triggers: a bumper is a
	// crisp digital click, so the rotate feels immediate.
	constexpr unsigned int LeftBumperButton = 4u;
	constexpr unsigned int RightBumperButton = 5u;
}

namespace
{
	// A quick flash on the lightbar for every menu move.
	constexpr float MenuNavigationLightbarDuration = 0.14f;

	// Used when no HapticSettings has been supplied, so the class still gives
	// feedback standalone.
	constexpr HapticSettings::Rumble FallbackMenuRumble{ 0.05f, 0.12f, 0.05f };
	constexpr HapticSettings::Colour FallbackMenuLightbar{ 255, 255, 0 };
}

GamepadManager::GamepadManager()
{
	RefreshConnection();
}

void GamepadManager::SetHaptics(Haptics::GamepadHaptics* newHaptics) noexcept
{
	haptics = newHaptics;
}

void GamepadManager::SetHapticSettings(const HapticSettings* newHapticSettings) noexcept
{
	hapticSettings = newHapticSettings;
}

void GamepadManager::HandleEvent(const sf::Event& event)
{
	if (event.is<sf::Event::JoystickConnected>() || event.is<sf::Event::JoystickDisconnected>())
	{
		RefreshConnection();
	}

	if (event.is<sf::Event::MouseMoved>() ||
		event.is<sf::Event::MouseButtonPressed>() ||
		event.is<sf::Event::KeyPressed>())
	{
		isInUse = false;
	}
	else if (event.is<sf::Event::JoystickButtonPressed>())
	{
		isInUse = true;
	}
	else if (const auto* moved = event.getIf<sf::Event::JoystickMoved>())
	{
		if (IsActiveJoystick(moved->joystickId) && std::abs(moved->position) > UsageThreshold)
		{
			isInUse = true;
		}
	}
}

void GamepadManager::Update()
{
	// Re-scan when we have no pad (or the one we had went away). SFML only
	// refreshes its joystick registry during the window's event pump, so a
	// controller that was already plugged in at start-up -- when the one-shot
	// scan in the constructor ran -- would otherwise never be picked up.
	if (!activeJoystick.has_value() || !sf::Joystick::isConnected(*activeJoystick))
	{
		RefreshConnection();
	}

	const unsigned int hardDropButton = (layout == Layout::PlayStation) ? PlayStationHardDropButton : XboxHardDropButton;
	const bool hardDropDown = IsButtonPressed(hardDropButton);
	hardDropEdge = hardDropDown && !wasHardDropDown;
	wasHardDropDown = hardDropDown;

	const bool rightBumperDown = IsButtonPressed(RightBumperButton);
	rotateClockwiseEdge = rightBumperDown && !wasRightBumperDown;
	wasRightBumperDown = rightBumperDown;

	const bool leftBumperDown = IsButtonPressed(LeftBumperButton);
	rotateCounterClockwiseEdge = leftBumperDown && !wasLeftBumperDown;
	wasLeftBumperDown = leftBumperDown;
}

GamepadManager::NavigationAction GamepadManager::GetNavigationAction(const sf::Event& event) const
{
	const NavigationAction action = ComputeNavigationAction(event);

	if (action != NavigationAction::None && haptics != nullptr)
	{
		const HapticSettings::Rumble& rumble = hapticSettings != nullptr ? hapticSettings->menuNavigation : FallbackMenuRumble;
		const HapticSettings::Colour lightbar = hapticSettings != nullptr ? hapticSettings->menuLightbar : FallbackMenuLightbar;

		Haptics::Pulse(*haptics, rumble);
		Haptics::FlashLightbar(*haptics, lightbar, MenuNavigationLightbarDuration);
	}

	return action;
}

GamepadManager::NavigationAction GamepadManager::ComputeNavigationAction(const sf::Event& event) const
{
	if (const auto* moved = event.getIf<sf::Event::JoystickMoved>())
	{
		if (!IsActiveJoystick(moved->joystickId))
		{
			return NavigationAction::None;
		}

		const sf::Joystick::Axis axis = moved->axis;
		const float position = moved->position;

		if (axis == sf::Joystick::Axis::X || axis == sf::Joystick::Axis::PovX)
		{
			if (position < -DirectionThreshold) return NavigationAction::Left;
			if (position > DirectionThreshold) return NavigationAction::Right;
		}
		else if (axis == sf::Joystick::Axis::Y)
		{
			if (position < -DirectionThreshold) return NavigationAction::Up;
			if (position > DirectionThreshold) return NavigationAction::Down;
		}
		else if (axis == sf::Joystick::Axis::PovY)
		{
			// SFML's PovY is inverted relative to the Y axis.
			if (position > DirectionThreshold) return NavigationAction::Up;
			if (position < -DirectionThreshold) return NavigationAction::Down;
		}

		return NavigationAction::None;
	}

	const auto* pressed = event.getIf<sf::Event::JoystickButtonPressed>();
	if (pressed == nullptr || !IsActiveJoystick(pressed->joystickId))
	{
		return NavigationAction::None;
	}

	// Circle / A = confirm, Square / B = back -- each layout's own numbering.
	constexpr unsigned int PlayStationConfirm = 1u;
	constexpr unsigned int PlayStationBack = 2u;
	constexpr unsigned int XboxConfirm = 0u;
	constexpr unsigned int XboxBack = 1u;

	const unsigned int confirmButton = (layout == Layout::PlayStation) ? PlayStationConfirm : XboxConfirm;
	const unsigned int backButton = (layout == Layout::PlayStation) ? PlayStationBack : XboxBack;

	if (pressed->button == confirmButton) return NavigationAction::Confirm;
	if (pressed->button == backButton) return NavigationAction::Back;
	return NavigationAction::None;
}

bool GamepadManager::IsPausePressed(const sf::Event& event) const
{
	// Options on DualShock, Start / Menu on Xbox.
	constexpr unsigned int PlayStationPause = 9u;
	constexpr unsigned int XboxPause = 7u;

	return IsButtonInEvent(event, (layout == Layout::PlayStation) ? PlayStationPause : XboxPause);
}

int GamepadManager::GetHorizontalDirection() const
{
	const float value = ReadAxis(sf::Joystick::Axis::X) + ReadAxis(sf::Joystick::Axis::PovX);

	if (value < -DirectionThreshold) return -1;
	if (value > DirectionThreshold) return 1;
	return 0;
}

bool GamepadManager::IsSoftDropHeld() const
{
	// Left stick pushed down, or d-pad down (PovY is inverted).
	return ReadAxis(sf::Joystick::Axis::Y) > DirectionThreshold ||
		ReadAxis(sf::Joystick::Axis::PovY) < -DirectionThreshold;
}

bool GamepadManager::WasHardDropPressed() const noexcept
{
	return hardDropEdge;
}

bool GamepadManager::WasRotateClockwisePressed() const noexcept
{
	return rotateClockwiseEdge;
}

bool GamepadManager::WasRotateCounterClockwisePressed() const noexcept
{
	return rotateCounterClockwiseEdge;
}

bool GamepadManager::IsConnected() const noexcept
{
	return activeJoystick.has_value();
}

bool GamepadManager::IsInUse() const noexcept
{
	return isInUse;
}

GamepadManager::Layout GamepadManager::GetLayout() const noexcept
{
	return layout;
}

void GamepadManager::RefreshConnection()
{
	activeJoystick.reset();
	layout = Layout::Generic;

	for (unsigned int id = 0u; id < sf::Joystick::Count; id++)
	{
		if (!sf::Joystick::isConnected(id))
		{
			continue;
		}

		activeJoystick = id;

		const sf::Joystick::Identification identification = sf::Joystick::getIdentification(id);
		if (identification.vendorId == MicrosoftVendorID)
		{
			layout = Layout::Xbox;
		}
		else if (identification.vendorId == SonyVendorID)
		{
			layout = Layout::PlayStation;
		}

		return;
	}
}

bool GamepadManager::IsActiveJoystick(unsigned int joystickID) const noexcept
{
	return activeJoystick.has_value() && *activeJoystick == joystickID;
}

bool GamepadManager::IsButtonPressed(unsigned int button) const
{
	return activeJoystick.has_value() &&
		button < sf::Joystick::getButtonCount(*activeJoystick) &&
		sf::Joystick::isButtonPressed(*activeJoystick, button);
}

bool GamepadManager::IsButtonInEvent(const sf::Event& event, unsigned int button) const
{
	const auto* pressed = event.getIf<sf::Event::JoystickButtonPressed>();
	return pressed != nullptr && IsActiveJoystick(pressed->joystickId) && pressed->button == button;
}

float GamepadManager::ReadAxis(sf::Joystick::Axis axis) const
{
	if (!activeJoystick.has_value() || !sf::Joystick::hasAxis(*activeJoystick, axis))
	{
		return 0.f;
	}

	const float position = sf::Joystick::getAxisPosition(*activeJoystick, axis);
	return std::abs(position) <= StickDeadZone ? 0.f : position;
}
