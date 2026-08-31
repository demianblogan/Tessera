#pragma once

#include <optional>

#include <SFML/Graphics/Color.hpp>

namespace sf
{
	class Event;
	class RenderTarget;
}

struct Context;
class MenuShell;

// One screen inside the MenuShell: the main menu, Options, Credits, and later
// the in-game menus. Deliberately not a State -- the shell is the State, and it
// keeps the shared animated background, the header and the DualSense lightbar
// alive while screens are swapped underneath it.
class MenuScreen
{
public:
	explicit MenuScreen(MenuShell& shell);
	virtual ~MenuScreen() = default;

	MenuScreen(const MenuScreen&) = delete;
	MenuScreen& operator=(const MenuScreen&) = delete;

	virtual void HandleEvent(const sf::Event& event) = 0;
	virtual void Update(float deltaTime) = 0;

	// Drawn after the shell's background and before its version stamp.
	virtual void Render(sf::RenderTarget& target) = 0;

	// The lightbar colour to hold while this screen is active; std::nullopt
	// leaves the lightbar untouched (e.g. during an intro animation).
	[[nodiscard]] virtual std::optional<sf::Color> LightbarColour() const { return std::nullopt; }

	// Whether the game's cursor sprite is drawn over this screen.
	[[nodiscard]] virtual bool ShowsCursor() const { return true; }

protected:
	MenuShell& shell;
	Context& context;
};
