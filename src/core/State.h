#pragma once

#include <memory>

#include "StateId.h"

class StateMachine;

namespace sf
{
    class Event;
    class RenderTarget;
}

class State
{
public:
    explicit State(StateMachine& stateMachine);
    virtual ~State() = default;

    State(const State&) = delete;
    State& operator=(const State&) = delete;
    State(State&&) = delete;
    State& operator=(State&&) = delete;

    [[nodiscard]] virtual StateId GetId() const = 0;

    virtual void HandleEvent(const sf::Event& event) = 0;
    virtual void Update(float deltaTime) = 0;
    virtual void Render(sf::RenderTarget& target) = 0;

    // When true, the state below this one is rendered as well (used for the
    // pause overlay).
    [[nodiscard]] virtual bool IsTransparent() const
    {
        return false;
    }

protected:
    // Queue a stack transition. All four are applied together, after the
    // current input / update phase, so a state may call them from inside its
    // own HandleEvent without being destroyed part-way through the call.
    void RequestPush(std::unique_ptr<State> state);
    void RequestPop();
    void RequestClear();
    void RequestChange(std::unique_ptr<State> state);

private:
    StateMachine& stateMachine;
};
