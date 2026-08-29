#pragma once

#include <functional>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

#include "ActionMap.h"

// Subscribe callbacks to game-specific actions and let this route input to
// them. HandleEvent(event) drives OnPress / OnRelease bindings; Update() drives
// WhileHeld bindings by polling live key/button state.
template <typename Action>
class InputHandler
{
public:
	using Callback = std::function<void()>;

	explicit InputHandler(const ActionMap<Action>& map)
		: actionMap(map)
	{
	}

	void Subscribe(Action action, Callback callback)
	{
		callbacks[action].push_back(std::move(callback));
	}

	void UnsubscribeAll(Action action)
	{
		callbacks.erase(action);
	}

	void HandleEvent(const sf::Event& event)
	{
		for (const auto& [action, bindings] : actionMap.GetBindingsMap())
		{
			for (const InputBinding& binding : bindings)
			{
				if (MatchesEvent(binding, event))
				{
					Invoke(action);
				}
			}
		}
	}

	void Update()
	{
		for (const auto& [action, bindings] : actionMap.GetBindingsMap())
		{
			for (const InputBinding& binding : bindings)
			{
				if (IsHeld(binding))
				{
					Invoke(action);
				}
			}
		}
	}

private:
	void Invoke(Action action)
	{
		const auto iterator = callbacks.find(action);
		if (iterator == callbacks.end())
		{
			return;
		}

		for (const Callback& callback : iterator->second)
		{
			callback();
		}
	}

	[[nodiscard]] static bool MatchesEvent(const InputBinding& binding, const sf::Event& event)
	{
		using Trigger = InputBinding::TriggerType;

		const Trigger trigger = binding.GetTriggerType();
		if (trigger == Trigger::WhileHeld)
		{
			return false;
		}

		const auto visitor = [trigger, &event](auto value) -> bool
			{
				using Value = decltype(value);

				if constexpr (std::is_same_v<Value, sf::Keyboard::Scancode>)
				{
					if (trigger == Trigger::OnPress)
					{
						const auto* pressed = event.getIf<sf::Event::KeyPressed>();
						return pressed != nullptr && pressed->scancode == value;
					}

					const auto* released = event.getIf<sf::Event::KeyReleased>();
					return released != nullptr && released->scancode == value;
				}
				else
				{
					if (trigger == Trigger::OnPress)
					{
						const auto* pressed = event.getIf<sf::Event::MouseButtonPressed>();
						return pressed != nullptr && pressed->button == value;
					}

					const auto* released = event.getIf<sf::Event::MouseButtonReleased>();
					return released != nullptr && released->button == value;
				}
			};

		return std::visit(visitor, binding.GetInputValue());
	}

	[[nodiscard]] static bool IsHeld(const InputBinding& binding)
	{
		if (binding.GetTriggerType() != InputBinding::TriggerType::WhileHeld)
		{
			return false;
		}

		const auto visitor = [](auto value) -> bool
			{
				using Value = decltype(value);

				if constexpr (std::is_same_v<Value, sf::Keyboard::Scancode>)
				{
					return sf::Keyboard::isKeyPressed(value);
				}
				else
				{
					return sf::Mouse::isButtonPressed(value);
				}
			};

		return std::visit(visitor, binding.GetInputValue());
	}

	const ActionMap<Action>& actionMap;
	std::unordered_map<Action, std::vector<Callback>> callbacks;
};
