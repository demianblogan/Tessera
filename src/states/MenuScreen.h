#pragma once

#include <optional>

#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Vector2.hpp>

namespace sf
{
	class Event;
	class RenderTarget;
}

struct Context;
class ScreenHost;

// One screen inside a ScreenHost: the main menu, Options, Credits, mode select,
// the in-game pause menu. Deliberately not a State -- the host is the State, and
// it keeps the background, the header and the DualSense lightbar alive while
// screens are swapped underneath it.
class MenuScreen
{
public:
	explicit MenuScreen(ScreenHost& host);
	virtual ~MenuScreen() = default;

	MenuScreen(const MenuScreen&) = delete;
	MenuScreen& operator=(const MenuScreen&) = delete;

	virtual void HandleEvent(const sf::Event& event) = 0;
	virtual void Update(float deltaTime) = 0;

	// Drawn after the host's background and before its overlay.
	virtual void Render(sf::RenderTarget& target) = 0;

	// Transition hooks. A screen being replaced through a host transition is
	// sent StartExit() and then kept updating / rendering until ExitFinished()
	// reports true, at which point the host swaps it out. A screen that becomes
	// active through a transition is sent PlayIntro().
	// A brief press acknowledgement, played before StartExit().
	virtual void PlayActivatePulse() {}
	virtual void StartExit() {}
	[[nodiscard]] virtual bool ExitFinished() const { return true; }
	virtual void PlayIntro() {}

	// Where the host's header should sink to when this screen is the one a Back
	// transition returns to (the home screen). Defaults leave it in the header
	// slot; MainMenuScreen and the pause menu point it at the matching entry.
	[[nodiscard]] virtual sf::Vector2f HeaderReturnCentre() const { return { 960.f, 120.f }; }
	[[nodiscard]] virtual float HeaderReturnHeight() const { return 110.f; }

	// The lightbar colour to hold while this screen is active; std::nullopt
	// leaves the lightbar untouched (e.g. during an intro animation).
	[[nodiscard]] virtual std::optional<sf::Color> LightbarColour() const { return std::nullopt; }

	// Whether the game's cursor sprite is drawn over this screen.
	[[nodiscard]] virtual bool ShowsCursor() const { return true; }

protected:
	ScreenHost& host;
	Context& context;
};
