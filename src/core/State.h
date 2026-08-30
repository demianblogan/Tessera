#pragma once

#include <memory>

class StateMachine;

namespace sf
{
    class Event;
    class RenderTarget;
}

class State
{
public:
    // What the application should put behind this state. Opaque states cover
    // the screen themselves; BlurredPrevious asks for the state below to be
    // rendered and blurred first (the pause overlay).
    enum class Backdrop
    {
        Opaque,
        BlurredPrevious
    };

    explicit State(StateMachine& stateMachine);
    virtual ~State() = default;

    State(const State&) = delete;
    State& operator=(const State&) = delete;
    State(State&&) = delete;
    State& operator=(State&&) = delete;

    virtual void HandleEvent(const sf::Event& event) = 0;
    virtual void Update(float deltaTime) = 0;
    virtual void Render(sf::RenderTarget& target) = 0;

    [[nodiscard]] virtual Backdrop GetBackdrop() const
    {
        return Backdrop::Opaque;
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
