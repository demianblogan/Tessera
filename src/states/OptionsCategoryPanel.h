#pragma once

namespace sf
{
	class Event;
	class RenderTarget;
}

// One category's right-hand panel inside OptionsScreen (Graphics, Audio, ...).
// The screen tells it what it should be each frame; the panel eases its own
// opacity toward that so switching categories or opening one cross-fades
// instead of popping.
class OptionsCategoryPanel
{
public:
	virtual ~OptionsCategoryPanel() = default;

	enum class Visibility
	{
		Hidden,    // not the selected category, and not open
		Preview,   // selected but not open: dimmed, non-interactive
		Open       // opened: full and interactive
	};

	// Called when the category is opened / closed, so a panel can snapshot the
	// current settings or drop transient state.
	virtual void Open() {}
	virtual void Close() {}

	// Called every frame with what the screen wants this panel to be and the
	// screen's 0..1 preview cross-fade.
	virtual void SetVisibility(Visibility visibility, float previewFade) = 0;

	virtual void Update(float deltaTime) = 0;
	virtual void Render(sf::RenderTarget& target) = 0;   // at the panel's own opacity

	// While open. Return true if the event was consumed. A Back action that is
	// not consumed tells the screen to close the category.
	virtual bool HandleEvent(const sf::Event& event) = 0;

	// Blocks the category from closing (an open confirmation dialog, ...).
	[[nodiscard]] virtual bool WantsToStayOpen() const { return false; }

	// The panel asks the screen to close the category (e.g. after resolving its
	// own unsaved-changes dialog).
	[[nodiscard]] virtual bool WantsToClose() const { return false; }
};
