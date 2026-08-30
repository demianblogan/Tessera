#pragma once

#include <SFML/Graphics/Sprite.hpp>

#include "../core/State.h"

struct Context;

namespace sf
{
	class Event;
}

// The publisher logo shown once at start-up, before the main menu: fades in,
// holds, fades out, with a short audio sting. Any key / button / click skips
// straight to the fade-out. Extends State directly (no menu chrome).
class CompanySplashState final : public State
{
public:
	explicit CompanySplashState(Context& context);

	void HandleEvent(const sf::Event& event) override;
	void Update(float deltaTime) override;
	void Render(sf::RenderTarget& target) override;

	[[nodiscard]] bool UsesCrtEffect() const override { return false; }

private:
	static constexpr float FadeInDuration = 0.5f;
	static constexpr float HoldDuration = 2.f;
	static constexpr float FadeOutDuration = 0.5f;

	[[nodiscard]] static bool IsSkipEvent(const sf::Event& event);
	void Finish();
	void UpdateOpacity();

	Context& context;

	sf::Sprite logo;
	float elapsedTime = 0.f;
	bool isFinishing = false;
};
