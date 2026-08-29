#pragma once

#include <variant>

#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

// One physical input (a keyboard key or a mouse button) plus how it should be
// read. Gamepads deliberately do not go through here -- see GamepadManager for
// why analog sticks, POV-hat d-pads and vendor-specific button indices don't
// fit a discrete keyboard/mouse binding.
class InputBinding
{
public:
	enum class TriggerType
	{
		OnPress,   // fires once, on the press
		OnRelease, // fires once, on the release
		WhileHeld  // fires every frame the input is down
	};

	// Scancode, not Key: physical key position, so a binding survives a
	// keyboard-layout change.
	using InputValue = std::variant<sf::Keyboard::Scancode, sf::Mouse::Button>;

	InputBinding(sf::Keyboard::Scancode key, TriggerType triggerType) noexcept;
	InputBinding(sf::Mouse::Button button, TriggerType triggerType) noexcept;

	[[nodiscard]] const InputValue& GetInputValue() const noexcept;
	[[nodiscard]] TriggerType GetTriggerType() const noexcept;

private:
	InputValue inputValue;
	TriggerType triggerType;
};
