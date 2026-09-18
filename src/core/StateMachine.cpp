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
		return;

	// Snapshot of every state already on the stack, taken before this batch of
	// transitions runs. Used below to tell a state that was already on the
	// stack and is merely revealed by a Pop (which should get OnResume()) apart
	// from a state freshly Pushed in this same batch (which shouldn't -- it
	// just ran its own constructor and has nothing to "resume").
	std::vector<State*> statesBeforeTransitions;
	statesBeforeTransitions.reserve(states.size());
	for (const std::unique_ptr<State>& state : states)
		statesBeforeTransitions.push_back(state.get());

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
		const bool isStateRevealed =
			!statesBeforeTransitions.empty() &&
			statesBeforeTransitions.back() != top &&
			std::find(statesBeforeTransitions.begin(), statesBeforeTransitions.end(), top) != statesBeforeTransitions.end();

		if (isStateRevealed)
			top->OnResume();
	}
}

State* StateMachine::GetCurrentState()
{
	if (states.empty())
		return nullptr;

	return states.back().get();
}

void StateMachine::RenderStates(sf::RenderTarget& target)
{
	for (const std::unique_ptr<State>& state : states)
		state->Render(target);
}
