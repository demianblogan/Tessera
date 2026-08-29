#pragma once

#include <unordered_map>
#include <utility>
#include <vector>

#include "InputBinding.h"

// Maps each value of a game-specific Action enum to the physical inputs that
// should fire it. Just the table -- InputHandler<Action> reads events / state
// against it.
//
//   enum class GameplayAction { HardDrop, Rotate };
//   ActionMap<GameplayAction> map;
//   map.AddBinding(GameplayAction::HardDrop,
//       InputBinding(sf::Keyboard::Scancode::Space, InputBinding::TriggerType::OnPress));
template <typename Action>
class ActionMap
{
public:
	void AddBinding(Action action, InputBinding binding)
	{
		actions[action].push_back(std::move(binding));
	}

	void ClearBindings(Action action)
	{
		actions.erase(action);
	}

	[[nodiscard]] const std::unordered_map<Action, std::vector<InputBinding>>& GetBindingsMap() const noexcept
	{
		return actions;
	}

private:
	std::unordered_map<Action, std::vector<InputBinding>> actions;
};
