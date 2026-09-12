#pragma once

#include <string_view>
#include <vector>

#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/System/Vector2.hpp>

#include "../settings/GameSettings.h"
#include "../ui/NineSliceFrame.h"

struct Context;

namespace sf
{
	class RenderTarget;
}

// The in-game HUD: two grouped panels flanking the well, plus a horizontal
// controls strip under it -- three places to look instead of one framed
// square per stat scattered all around the well. Left panel: HOLD (the piece
// box, drawn once the hold mechanic lands) over LEVEL and TIME. Right panel:
// NEXT (the piece queue) over SCORE and LINES. Both sit flush with the well's
// top edge. The controls legend spans the full width below the well -- it's a
// reference, glanced at rarely, so it sits apart from the two stat panels
// rather than competing with them. GameplayState pushes the numbers each
// frame with Set() and calls the OnX hooks so the matching row flashes;
// BoardRenderer draws the next queue inside NextPreviewArea().
class GameplayHud
{
public:
	explicit GameplayHud(Context& context);

	// The individually toggleable pieces of the HUD.
	enum class Element { Hold, Next, Score, Lines, Level, Time, ControlsLegend };

	void Set(int score, int level, int lines, float seconds);
	void Update(float deltaTime);

	void OnRowsCleared();   // flashes SCORE and LINES
	void OnLevelUp();       // flashes LEVEL

	void SetVisible(Element element, bool visible);
	void Render(sf::RenderTarget& target) const;

	// The active language changed -- re-fetch every caption and force the
	// controls legend to rebuild (its own change-check only watches bindings).
	void RefreshText();

	// Rebuilds the controls-legend text (key names, and whether Hold is listed
	// at all) if `controls` or `holdEnabled` differ from what it was last built
	// with -- a no-op most frames, so it is cheap to call every Update().
	void RefreshControlsLegend(const ControlSettings& controls, bool holdEnabled);

	[[nodiscard]] bool HoldVisible() const { return holdVisible; }
	[[nodiscard]] sf::FloatRect HoldPreviewArea() const { return holdBoxBounds; }

	[[nodiscard]] bool NextVisible() const { return nextVisible; }
	[[nodiscard]] sf::FloatRect NextPreviewArea() const { return nextBoxBounds; }

private:
	// One stat inside a panel (LEVEL, TIME, SCORE, LINES): a small caption over
	// a bigger value, both centred on the row -- the same shape the old single
	// stat cells used, just stacked two-to-a-panel instead of one-per-square.
	struct StatRow
	{
		sf::Text label;
		sf::Text value;
		float flash = 0.f;
		bool visible = true;
	};

	// A controls-legend entry: the action name over the key(s) bound to it, or
	// -- in gamepad mode -- over one or two button-prompt icons instead.
	struct ControlEntry
	{
		sf::Text label;
		sf::Text value;               // keyboard mode
		std::vector<sf::Sprite> icons; // gamepad mode
	};

	// Which set of values the legend below is showing right now.
	enum class PromptMode { Keyboard, Xbox, PlayStation };

	[[nodiscard]] StatRow MakeStatRow(std::string_view labelKey, std::string_view initialValue,
		float centreX, float rowTop) const;

	void DrawStatRow(sf::RenderTarget& target, const StatRow& row) const;
	void DrawValue(sf::RenderTarget& target, const sf::Text& value, float flash) const;
	[[nodiscard]] PromptMode CurrentPromptMode() const;
	void BuildControlsLegend(const ControlSettings& controls, bool holdEnabled, PromptMode mode);

	Context& context;

	// Left panel: HOLD placeholder + LEVEL + TIME.
	sf::RectangleShape leftFill;
	UI::NineSliceFrame leftFrame;
	sf::Text holdCaption;
	sf::FloatRect holdBoxBounds;
	sf::RectangleShape holdPlaceholder;   // empty outline until the hold mechanic lands
	sf::RectangleShape leftDivider;
	StatRow levelRow;
	StatRow timeRow;
	bool holdVisible = true;

	// Right panel: NEXT queue + SCORE + LINES.
	sf::RectangleShape rightFill;
	UI::NineSliceFrame rightFrame;
	sf::Text nextCaption;
	sf::FloatRect nextBoxBounds;
	sf::RectangleShape rightDivider;
	StatRow scoreRow;
	StatRow linesRow;
	bool nextVisible = true;

	// Controls legend: a horizontal strip under the well.
	sf::RectangleShape controlsFill;
	UI::NineSliceFrame controlsFrame;
	std::vector<ControlEntry> controlsEntries;
	bool showControls = true;

	// The bindings/flag the legend above was last built for, so
	// RefreshControlsLegend() can skip the rebuild on the (near-universal)
	// frame where nothing changed.
	ControlSettings legendControls;
	bool legendHoldEnabled = true;
	PromptMode legendPromptMode = PromptMode::Keyboard;
};
