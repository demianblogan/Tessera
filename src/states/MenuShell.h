#pragma once

#include <memory>

#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Text.hpp>

#include "../core/State.h"
#include "../ui/MenuAurora.h"
#include "../ui/MenuBackdrop.h"
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

private:
	void ApplyPendingScreen();

	Context& context;

	sf::Sprite backgroundSprite;
	UI::MenuAurora aurora;
	UI::MenuBackdrop backdrop;
	UI::MenuSparks sparks;
	sf::Text versionText;

	std::unique_ptr<MenuScreen> screen;
	std::unique_ptr<MenuScreen> pendingScreen;
	bool screenChangePending = false;
};
