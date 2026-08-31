#pragma once

#include "MenuScreen.h"

namespace sf
{
	class Event;
	class RenderTarget;
}

// Temporary stand-in used while the real sub-screens (Credits, Options) are
// being built. It draws nothing of its own -- only the shell's header shows --
// and Back or Confirm returns to the main menu.
class PlaceholderScreen final : public MenuScreen
{
public:
	explicit PlaceholderScreen(MenuShell& shell) : MenuScreen(shell) {}

	void HandleEvent(const sf::Event& event) override;
	void Update(float /*deltaTime*/) override {}
	void Render(sf::RenderTarget& /*target*/) override {}
};
