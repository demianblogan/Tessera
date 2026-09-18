#pragma once

#include <memory>

#include "../core/State.h"

struct Context;

namespace sf
{
	class Event;
	class RenderTarget;
	class RenderTexture;
}

class GameplayState;

// The menu -> gameplay transition. The main menu is captured whole -- ring,
// title, ambient and all -- the instant "Play" is pressed. This state holds
// that snapshot over the arriving (still frozen) gameplay scene, runs it
// through mosaic.frag so it "solidifies" into large pixels top-down, then
// slides the solid sheet down and off the screen like a door, revealing the
// gameplay behind it. Then it hands the gameplay state over.
class PlayTransitionState final : public State
{
public:
	PlayTransitionState(Context& context, std::unique_ptr<sf::RenderTexture> menuSnapshot);
	~PlayTransitionState() override;

	void HandleEvent(const sf::Event& event) override;
	void Update(float deltaTime) override;
	void Render(sf::RenderTarget& target) override;

	[[nodiscard]] bool IsCursorVisible() const override;

private:
	Context& context;
	std::unique_ptr<sf::RenderTexture> menuSnapshot;
	std::unique_ptr<GameplayState> gameplay;

	float timer = 0.f;
	bool hasHandedOver = false;
};
