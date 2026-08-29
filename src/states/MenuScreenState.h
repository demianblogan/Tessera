#pragma once

#include <cstddef>
#include <functional>
#include <vector>

#include <SFML/System/String.hpp>
#include <SFML/System/Vector2.hpp>

#include "../core/Context.h"
#include "../core/State.h"
#include "../rendering/NeonGlow.h"
#include "../ui/Layout.h"
#include "../ui/MenuList.h"

namespace sf
{
	class RenderTarget;
}

namespace UI
{
	class Button;
}

// Shared skeleton for the plain list-of-buttons screens (main menu, pause,
// game over): a MenuList over buttons in a vertical Layout, keyboard / gamepad
// navigation via MenuInput, mouse hover + click, the neon selection glow, and
// the per-frame glow tick.
//
// A subclass builds its own rootLayout content (title, spacers, decorations) in
// its constructor, points `menuLayout` at the sub-layout the buttons go in,
// calls AddMenuItem once per button, then RefreshLayout(). It draws its own
// background in Render() and finishes with RenderMenu().
class MenuScreenState : public State
{
protected:
	Context& context;
	NeonGlow neonGlow;

	UI::Layout rootLayout{ UI::Layout::Orientation::Vertical };
	UI::Layout* menuLayout = nullptr;
	UI::MenuList menuList;

	sf::Vector2f menuButtonSize{ 500.f, 120.f };
	unsigned int menuButtonTextSize = 80;

	explicit MenuScreenState(Context& context);

	// Appends a styled button to `menuLayout` and the MenuList, wired to run
	// `onActivate` when it is confirmed. The returned reference lets the caller
	// tweak the button further (size, disabled style, ...).
	UI::Button& AddMenuItem(const sf::String& text, std::function<void()> onActivate);

	void RefreshLayout();
	void RenderMenu(sf::RenderTarget& target);

	// The Back / Escape / gamepad-B action. Default: do nothing.
	virtual void OnBack() {}

	// A chance to consume an event the base doesn't handle (e.g. text entry),
	// checked after navigation and before mouse handling. Return true if consumed.
	[[nodiscard]] virtual bool HandleExtraEvent(const sf::Event& event);

public:
	void HandleEvent(const sf::Event& event) override;
	void Update(float deltaTime) override;

private:
	std::vector<std::function<void()>> activations;
};
