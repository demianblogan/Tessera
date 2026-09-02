#pragma once

#include <cstddef>
#include <memory>

#include <SFML/Graphics/Color.hpp>
#include <SFML/System/String.hpp>
#include <SFML/System/Vector2.hpp>

#include "../core/State.h"
#include "../ui/MenuHeader.h"

struct Context;
class MenuScreen;

namespace sf
{
	class Event;
	class RenderTarget;
}

// Hosts one MenuScreen at a time and owns the pieces that have to outlive a
// screen swap so a transition can animate one element into the next: the header
// that a menu entry appears to become, and the forward / back transition state
// machine. It also hands the DualSense lightbar the active screen's colour and
// draws a pluggable background behind every screen.
//
// Concrete hosts (MenuShell for the main-menu system, and the in-game pause
// host) supply the background and say what the "home" screen is -- the screen a
// Back transition returns to.
class ScreenHost : public State
{
public:
	explicit ScreenHost(Context& context);
	~ScreenHost() override;

	void HandleEvent(const sf::Event& event) override;
	void Update(float deltaTime) override;
	void Render(sf::RenderTarget& target) override;
	[[nodiscard]] bool ShowsCursor() const override;

	[[nodiscard]] Context& GetContext() { return context; }

	// Swap the active screen with no transition. Deferred to the end of the
	// current event / update step so a screen can call it from inside its own
	// methods without being destroyed part-way through.
	void ShowScreen(std::unique_ptr<MenuScreen> screen);

	// Leave the host entirely (Start Game, quit-to-desktop, back to gameplay...).
	void ExitTo(std::unique_ptr<State> state);

	// Animate from the home screen into a sub-screen: the current screen plays
	// its exit, the header (`label` in `colour`) rises from the activated entry
	// (`fromCentre` / `fromHeight`) into the header slot, then `next` takes over.
	// BeginBack() reverses it back to a freshly-built home screen.
	void BeginForward(std::unique_ptr<MenuScreen> next, const sf::String& label, sf::Color colour,
		sf::Vector2f fromCentre, float fromHeight, std::size_t entryIndex);
	void BeginBack();
	[[nodiscard]] bool IsTransitioning() const;

	// A home screen reports navigation (ring rotation etc.) so the host can react
	// -- MenuShell shoves its drifting-tetromino backdrop. Default: nothing.
	virtual void OnNavigate(float /*direction*/) {}

protected:
	// The concrete host builds its first screen here (called from its ctor).
	void SetInitialScreen(std::unique_ptr<MenuScreen> screen);

	virtual void UpdateBackground(float deltaTime) = 0;
	virtual void RenderBackground(sf::RenderTarget& target) = 0;
	// Drawn on top of the active screen and the header (MenuShell's version stamp).
	virtual void RenderOverlay(sf::RenderTarget& /*target*/) {}
	// The screen a Back transition returns to, focused on `returnEntryIndex`.
	[[nodiscard]] virtual std::unique_ptr<MenuScreen> BuildHomeScreen(std::size_t returnEntryIndex) = 0;

	Context& context;

private:
	enum class Phase { Steady, Forward, Back };

	void ApplyPendingScreen();
	void AdvanceTransition(float deltaTime);

	UI::MenuHeader header;

	std::unique_ptr<MenuScreen> screen;
	std::unique_ptr<MenuScreen> pendingScreen;
	bool screenChangePending = false;

	Phase phase = Phase::Steady;
	bool onSubScreen = false;
	bool mainRebuilt = false;             // Back: has the home screen been put back yet
	std::unique_ptr<MenuScreen> nextScreen;

	// Forward: a short lead where only the press pulse plays, then the exit /
	// header rise begin.
	bool forwardStarted = false;
	float forwardTimer = 0.f;
	sf::String pendingLabel;
	sf::Color pendingColour{ sf::Color::White };
	sf::Vector2f pendingFromCentre;
	float pendingFromHeight = 0.f;
	std::size_t returnEntryIndex = 0;   // entry to refocus on when going back
};
