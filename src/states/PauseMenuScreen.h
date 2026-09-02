#pragma once

#include <cstddef>
#include <optional>

#include <SFML/Graphics/Color.hpp>

#include "../ui/MenuButtonColumn.h"
#include "MenuScreen.h"

namespace sf
{
	class Event;
	class RenderTarget;
}

class PauseState;

// The home screen of the in-game pause host: a left column of options
// (Resume / Restart / Options / Back to Main Menu) styled like the Options
// category column. It slides in from the left as the frame solidifies and
// slides back out on Resume. The "PAUSE" header above it belongs to the host.
class PauseMenuScreen final : public MenuScreen
{
public:
	PauseMenuScreen(PauseState& owner, std::size_t focusRow);

	void HandleEvent(const sf::Event& event) override;
	void Update(float deltaTime) override;
	void Render(sf::RenderTarget& target) override;

	void PlayIntro() override;
	void StartExit() override;
	[[nodiscard]] bool ExitFinished() const override;

	[[nodiscard]] std::optional<sf::Color> LightbarColour() const override;

	[[nodiscard]] static std::size_t OptionsRow();

private:
	void ApplySlide();

	PauseState& pause;
	UI::MenuButtonColumn column;

	bool introStarted = false;
	bool exiting = false;
	float slideT = 0.f;   // 0 = fully off to the left, 1 = resting in place
};
