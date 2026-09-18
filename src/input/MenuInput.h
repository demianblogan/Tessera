#pragma once

namespace sf
{
	class Event;
}

class GamepadManager;

// Collapses a keyboard or gamepad event into a single menu-navigation intent,
// so every menu screen handles one enum instead of both raw scancodes and
// GamepadManager nav actions. Mouse selection is position-based and stays
// separate (MenuList::SelectAt).
namespace MenuInput
{
	enum class Action
	{
		None,
		Up,
		Down,
		Left,
		Right,
		Confirm,
		Back
	};

	// Checks event against the fixed menu keys (arrows, Enter, Escape) first;
	// if it isn't one of those, falls back to what gamepad.GetNavigationAction()
	// makes of the same event. Returns Action::None if neither source maps it
	// to a menu action.
	[[nodiscard]] Action ResolveAction(const sf::Event& event, const GamepadManager& gamepad);
}
