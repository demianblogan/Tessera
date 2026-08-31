#pragma once

#include <optional>

#include <SFML/Graphics/Color.hpp>

#include "../ui/MenuButtonColumn.h"
#include "MenuScreen.h"

namespace sf
{
	class Event;
	class RenderTarget;
}

// The Options sub-screen. A left-aligned column of category buttons that flies
// up from the bottom on entry. The "OPTIONS" header above it is the shell's,
// morphed from the menu entry.
//
// Phase 1: only "Back to Main Menu" is active -- the categories are disabled
// placeholders while the transition and layout are built. The right-hand
// preview / content panel and the compact list state come with the categories.
class OptionsScreen final : public MenuScreen
{
public:
	OptionsScreen(MenuShell& shell, sf::Color accent);

	void HandleEvent(const sf::Event& event) override;
	void Update(float deltaTime) override;
	void Render(sf::RenderTarget& target) override;

	void PlayIntro() override;
	void StartExit() override;
	[[nodiscard]] bool ExitFinished() const override;

	[[nodiscard]] std::optional<sf::Color> LightbarColour() const override { return accent; }

private:
	void Leave();

	sf::Color accent;
	UI::MenuButtonColumn column;
	bool leaving = false;
};
