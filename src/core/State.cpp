#include "State.h"

#include "StateMachine.h"

State::State(StateMachine& stateMachine)
	: stateMachine(stateMachine)
{
}

void State::RequestPush(std::unique_ptr<State> state)
{
	stateMachine.PushState(std::move(state));
}

void State::RequestPop()
{
	stateMachine.PopState();
}

void State::RequestClear()
{
	stateMachine.ClearStates();
}

void State::RequestChange(std::unique_ptr<State> state)
{
	stateMachine.ChangeState(std::move(state));
}
