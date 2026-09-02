#pragma once

#include <memory>

#include "MenuScreenState.h"

namespace sf
{
	class Event;
	class RenderTexture;
}

class PauseState final : public MenuScreenState
{
public:
	// `frozenFrame` is a snapshot of the gameplay frame taken the moment the
	// game was paused; the pause screen "solidifies" it into large pixels
	// behind the menu. May be null (capture failed) -- then a plain dim overlay
	// is used instead.
	PauseState(Context& context, std::unique_ptr<sf::RenderTexture> frozenFrame);
	~PauseState() override;

	void HandleEvent(const sf::Event& event) override;
	void Update(float deltaTime) override;
	void Render(sf::RenderTarget& target) override;

	[[nodiscard]] Backdrop GetBackdrop() const override;

private:
	void OnBack() override;
	void BeginResume();
	void RenderFrozenBackdrop(sf::RenderTarget& target);

	std::unique_ptr<sf::RenderTexture> frozenFrame;

	// 0 = the frame is crisp, 1 = fully solidified. Sweeps 0 -> 1 on entry
	// (top-down) and 1 -> 0 on Resume (bottom-up), then the state pops.
	float reveal = 0.f;
	bool resuming = false;
};
