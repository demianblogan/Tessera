#pragma once

#include <memory>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/System/String.hpp>

#include "../core/State.h"
#include "../ui/MenuAurora.h"
#include "../ui/MenuBackdrop.h"
#include "../ui/MenuHeader.h"
#include "../ui/MenuSparks.h"

struct Context;
class MenuScreen;

namespace sf
{
	class Event;
}

// The single State for the whole menu system. It owns the shared, persistent
// pieces -- the animated background (aurora, drifting tetrominoes, sparks), the
// version stamp, the DualSense lightbar handoff -- and hosts one MenuScreen at
// a time, swapping them without tearing the background down. That continuity is
// what lets a screen transition animate one element into the next.
class MenuShell final : public State
{
public:
	explicit MenuShell(Context& context);
	~MenuShell() override;

	void HandleEvent(const sf::Event& event) override;
	void Update(float deltaTime) override;
	void Render(sf::RenderTarget& target) override;
	[[nodiscard]] bool ShowsCursor() const override;

	[[nodiscard]] Context& GetContext() { return context; }
	[[nodiscard]] UI::MenuBackdrop& Backdrop() { return backdrop; }

	// Swap the active screen. Deferred to the end of the current event / update
	// step so a screen can call it from inside its own methods without being
	// destroyed part-way through.
	void ShowScreen(std::unique_ptr<MenuScreen> screen);

	// Leave the menu system entirely (Start Game, and later a quit-to-desktop
	// confirmation, ...).
	void ExitTo(std::unique_ptr<State> state);

	// Animate from the main menu into a sub-screen: the current screen plays its
	// exit, the header (`label` in `colour`) rises from the activated entry
	// (`fromCentre` / `fromHeight`) into the header slot, then `next` takes over.
	// BeginBack() reverses it back to a freshly-built main menu.
	void BeginForward(std::unique_ptr<MenuScreen> next, const sf::String& label, sf::Color colour,
		sf::Vector2f fromCentre, float fromHeight);
	void BeginBack();
	[[nodiscard]] bool IsTransitioning() const;

private:
	enum class Phase { Steady, Forward, Back };

	void ApplyPendingScreen();
	void AdvanceTransition(float deltaTime);

	Context& context;

	sf::Sprite backgroundSprite;
	UI::MenuAurora aurora;
	UI::MenuBackdrop backdrop;
	UI::MenuSparks sparks;
	sf::Text versionText;

	UI::MenuHeader header;

	std::unique_ptr<MenuScreen> screen;
	std::unique_ptr<MenuScreen> pendingScreen;
	bool screenChangePending = false;

	// --- Screen transition ---
	Phase phase = Phase::Steady;
	bool onSubScreen = false;
	bool mainRebuilt = false;             // Back: has the main menu been put back yet
	std::unique_ptr<MenuScreen> nextScreen;
};
