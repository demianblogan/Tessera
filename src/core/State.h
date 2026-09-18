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
    explicit State(StateMachine& stateMachine);
    virtual ~State() = default;

    State(const State&) = delete;
    State& operator=(const State&) = delete;
    State(State&&) = delete;
    State& operator=(State&&) = delete;

    virtual void HandleEvent(const sf::Event& event) = 0;
    virtual void Update(float deltaTime) = 0;
    virtual void Render(sf::RenderTarget& target) = 0;

    // Called when this state becomes the top of the stack again because the
    // state above it was popped (e.g. the pause screen closing). Lets a state
    // pick up settings changed while it was covered.
    virtual void OnResume();

    // Whether the game's mouse cursor is drawn over this state. The company
    // splash hides it so nothing sits on top of the logo.
    [[nodiscard]] virtual bool IsCursorVisible() const;

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
