#include "MenuInput.h"

#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>

#include "GamepadManager.h"

namespace MenuInput
{
	Action Resolve(const sf::Event& event, const GamepadManager& gamepad)
	{
		if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>())
		{
			switch (keyPressed->scancode)
			{
			case sf::Keyboard::Scancode::Up:     return Action::Up;
			case sf::Keyboard::Scancode::Down:    return Action::Down;
			case sf::Keyboard::Scancode::Left:    return Action::Left;
			case sf::Keyboard::Scancode::Right:   return Action::Right;
			case sf::Keyboard::Scancode::Enter:   return Action::Confirm;
			case sf::Keyboard::Scancode::Escape:  return Action::Back;
			default:                              return Action::None;
			}
		}

		switch (gamepad.GetNavigationAction(event))
		{
		case GamepadManager::NavigationAction::Up:      return Action::Up;
		case GamepadManager::NavigationAction::Down:    return Action::Down;
		case GamepadManager::NavigationAction::Left:    return Action::Left;
		case GamepadManager::NavigationAction::Right:   return Action::Right;
		case GamepadManager::NavigationAction::Confirm: return Action::Confirm;
		case GamepadManager::NavigationAction::Back:    return Action::Back;
		default:                                        return Action::None;
		}
	}
}
