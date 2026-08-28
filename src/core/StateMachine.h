#pragma once

#include <memory>
#include <vector>

#include "State.h"

namespace sf
{
    class RenderTarget;
}

// Every transition (push / pop / clear / change) is queued and only carried out
// by ApplyPendingChanges(), which the main loop calls between phases. This is
// what lets a State request a transition from inside its own event handler
// without the stack destroying that State part-way through the call.
class StateMachine
{
public:
    void PushState(std::unique_ptr<State> state);
    void PopState();
    void ClearStates();
    void ChangeState(std::unique_ptr<State> state);

    [[nodiscard]] bool HasPendingChanges() const noexcept;
    void ApplyPendingChanges();

    [[nodiscard]] State* GetCurrentState();

    void RenderStates(sf::RenderTarget& target);
    void RenderStatesExceptTop(sf::RenderTarget& target);
    void RenderTopState(sf::RenderTarget& target);

private:
    enum class TransitionType
    {
        Push,
        Pop,
        Clear
    };

    struct PendingTransition
    {
        TransitionType type;
        std::unique_ptr<State> state;
    };

    std::vector<std::unique_ptr<State>> states;
    std::vector<PendingTransition> pendingTransitions;
};
