#pragma once

#include "StateId.h"

namespace sf
{
    class RenderTarget;
    class RenderWindow;
}

class State
{
public:
    virtual ~State() = default;

    [[nodiscard]] virtual StateId GetId() const = 0;

    virtual void ProcessEvents(sf::RenderWindow& window) = 0;
    virtual void Update(float deltaTime) = 0;
    virtual void Render(sf::RenderTarget& target) = 0;

    [[nodiscard]] virtual bool IsTransparent() const
    {
        return false;
    }
};
