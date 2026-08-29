#include "InputBinding.h"

InputBinding::InputBinding(sf::Keyboard::Scancode key, TriggerType triggerType) noexcept
	: inputValue(key), triggerType(triggerType)
{
}

InputBinding::InputBinding(sf::Mouse::Button button, TriggerType triggerType) noexcept
	: inputValue(button), triggerType(triggerType)
{
}

const InputBinding::InputValue& InputBinding::GetInputValue() const noexcept
{
	return inputValue;
}

InputBinding::TriggerType InputBinding::GetTriggerType() const noexcept
{
	return triggerType;
}
