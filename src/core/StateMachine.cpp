#include "StateMachine.h"

#include <algorithm>

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
	if (pendingTransitions.empty())
	{
		return;
	}

	std::vector<State*> before;
	before.reserve(states.size());
	for (const std::unique_ptr<State>& state : states)
	{
		before.push_back(state.get());
	}

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

	// A state that was already on the stack and is now back on top was revealed
	// by a pop (not freshly pushed), so let it resume.
	if (!states.empty())
	{
		State* top = states.back().get();
		const bool revealed = !before.empty() && before.back() != top
			&& std::find(before.begin(), before.end(), top) != before.end();

		if (revealed)
		{
			top->OnResume();
		}
	}
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
	for (const std::unique_ptr<State>& state : states)
	{
		state->Render(target);
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
