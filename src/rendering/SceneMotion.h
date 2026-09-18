#pragma once

#include <SFML/System/Vector2.hpp>

// A soft positional offset for the gameplay backdrop, driven by what the player
// does: each Nudge() adds an impulse, a critically-ish damped spring pulls it
// back to rest, and a slow idle drift plays underneath so the background is
// never perfectly still. GameplayState feeds it events and offsets the
// background sprite by Offset() -- the board and HUD are untouched, so the
// backdrop parallaxes against them.
class SceneMotion
{
public:
	void Nudge(sf::Vector2f impulse);
	void Update(float deltaTime);

	[[nodiscard]] sf::Vector2f GetOffset() const;

private:
	sf::Vector2f position{};
	sf::Vector2f velocity{};
	float idleTime = 0.f;
};
