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
// machine. It also hands the DualSense lightbar the active screen's color and
// draws a pluggable background behind every screen.
//
// Concrete hosts (MenuShellState for the main-menu system, and the in-game pause
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
	[[nodiscard]] bool IsCursorVisible() const override;

	[[nodiscard]] Context& GetContext();

	// Animate from the home screen into a sub-screen: the current screen plays
	// its exit, the header (`label` in `color`) rises from the activated entry
	// (`fromCenter` / `fromHeight`) into the header slot, then `next` takes over.
	// BeginBack() reverses it back to a freshly-built home screen.
	void BeginForward(std::unique_ptr<MenuScreen> next, const sf::String& label, sf::Color color,
		sf::Vector2f fromCenter, float fromHeight, std::size_t entryIndex);
	void BeginBack();

	// Re-labels the header without touching its current rise/sink pose -- a
	// screen calls this on its own header text when the language changes while
	// it's the one showing.
	void SetHeaderText(const sf::String& text);

	// A home screen reports navigation (ring rotation etc.) so the host can react
	// -- MenuShellState shoves its drifting-tetromino backdrop. Default: nothing.
	virtual void OnNavigate(float direction);

	// A home screen asks the host to leave for gameplay through the play
	// transition. Only MenuShellState acts on it.
	virtual void BeginPlay();

	// A Back transition normally sinks the header away toward the returned-to
	// entry. A host whose home screen carries its own persistent header (the
	// pause "PAUSE") returns true here and re-raises it in OnHomeRebuilt().
	[[nodiscard]] virtual bool HasPersistentHeader() const;
	virtual void OnHomeRebuilt();

protected:
	// The concrete host builds its first screen here (called from its ctor).
	void SetInitialScreen(std::unique_ptr<MenuScreen> screen);

	[[nodiscard]] MenuScreen* GetCurrentScreen();

	// The header. A concrete host may drive it directly for a title that is not
	// tied to a forward transition (the pause "PAUSE").
	[[nodiscard]] UI::MenuHeader& GetHeader();

	virtual void UpdateBackground(float deltaTime) = 0;
	virtual void RenderBackground(sf::RenderTarget& target) = 0;
	// Drawn on top of the active screen and the header (MenuShellState's version stamp).
	virtual void RenderOverlay(sf::RenderTarget& target);
	// The screen a Back transition returns to, focused on `returnEntryIndex`.
	[[nodiscard]] virtual std::unique_ptr<MenuScreen> BuildHomeScreen(std::size_t returnEntryIndex) = 0;

	Context& context;

private:
	enum class Phase { Steady, Forward, Back };

	void AdvanceTransition(float deltaTime);

	UI::MenuHeader header;

	std::unique_ptr<MenuScreen> screen;

	Phase phase = Phase::Steady;
	bool isOnSubScreen = false;
	bool hasRebuiltMain = false;             // Back: has the home screen been put back yet
	std::unique_ptr<MenuScreen> nextScreen;

	// Forward: a short lead where only the press pulse plays, then the exit /
	// header rise begin.
	bool hasForwardStarted = false;
	float forwardTimer = 0.f;
	sf::String pendingLabel;
	sf::Color pendingColor{ sf::Color::White };
	sf::Vector2f pendingFromCenter;
	float pendingFromHeight = 0.f;
	std::size_t returnEntryIndex = 0;   // entry to refocus on when going back
};
