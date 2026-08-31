#pragma once

namespace sf
{
	class Event;
	class RenderTarget;
}

// One category's right-hand panel inside OptionsScreen (Graphics, Audio, ...).
// It is shown dimmed as a preview while its category is merely selected, and
// full and interactive once the category is opened.
class OptionsCategoryPanel
{
public:
	virtual ~OptionsCategoryPanel() = default;

	// Called when the category is opened / closed, so a panel can snapshot the
	// current settings or drop transient state.
	virtual void Open() {}
	virtual void Close() {}

	virtual void Update(float deltaTime) = 0;

	// Dimmed, non-interactive; `alpha` (0..1) is the screen's preview fade.
	virtual void RenderPreview(sf::RenderTarget& target, float alpha) = 0;

	// Full and interactive.
	virtual void RenderContent(sf::RenderTarget& target) = 0;

	// While open. Return true if the event was consumed. A Back action that is
	// not consumed tells the screen to close the category.
	virtual bool HandleEvent(const sf::Event& event) = 0;

	// Blocks the category from closing (an open confirmation dialog, ...).
	[[nodiscard]] virtual bool WantsToStayOpen() const { return false; }
};
