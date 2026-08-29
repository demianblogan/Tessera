#pragma once

namespace sf { class Event; }
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

	[[nodiscard]] Action Resolve(const sf::Event& event, const GamepadManager& gamepad);
}
