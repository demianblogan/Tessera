#include "StateMachine.h"

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderWindow.hpp>

void StateMachine::PushState(std::unique_ptr<State> state)
{
	pendingTransitions.push_back({ .type = TransitionType::Push, .state = std::move(state) });
}

void StateMachine::PopState()
{
	pendingTransitions.push_back({ .type = TransitionType::Pop, .state = nullptr });
}

void StateMachine::ClearStates()
{
	pendingTransitions.push_back({ .type = TransitionType::Clear, .state = nullptr });
}

void StateMachine::ChangeState(std::unique_ptr<State> state)
{
	PopState();
	PushState(std::move(state));
}

bool StateMachine::HasPendingChanges() const noexcept
{
	return !pendingTransitions.empty();
}

void StateMachine::ApplyPendingChanges()
{
	for (PendingTransition& transition : pendingTransitions)
	{
		switch (transition.type)
		{
		case TransitionType::Push:
			states.push_back(std::move(transition.state));
			break;

		case TransitionType::Pop:
			if (!states.empty())
			{
				states.pop_back();
			}
			break;

		case TransitionType::Clear:
			states.clear();
			break;
		}
	}

	pendingTransitions.clear();
}

State* StateMachine::GetCurrentState()
{
	if (states.empty())
	{
		return nullptr;
	}

	return states.back().get();
}

void StateMachine::RenderStates(sf::RenderTarget& target)
{
	if (states.empty())
	{
		return;
	}

	int startIndex = static_cast<int>(states.size()) - 1;

	while (startIndex > 0 && states[startIndex]->IsTransparent())
	{
		startIndex--;
	}

	for (int i = startIndex; i < static_cast<int>(states.size()); i++)
	{
		states[i]->Render(target);
	}
}

void StateMachine::RenderStatesExceptTop(sf::RenderTarget& target)
{
	if (states.size() <= 1)
	{
		return;
	}

	for (std::size_t i = 0; i < states.size() - 1; i++)
	{
		states[i]->Render(target);
	}
}

void StateMachine::RenderTopState(sf::RenderTarget& target)
{
	if (states.empty())
	{
		return;
	}

	states.back()->Render(target);
}
