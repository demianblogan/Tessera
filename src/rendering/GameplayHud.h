#pragma once

#include <string_view>
#include <vector>

#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/System/Vector2.hpp>

#include "../ui/NineSliceFrame.h"

struct Context;

namespace sf
{
	class RenderTarget;
}

// The in-game HUD: two grouped panels flanking the well instead of one framed
// square per stat, so the eye only travels to two places instead of scattering
// across six. Left panel: HOLD (the piece box, drawn once the hold mechanic
// lands) over LEVEL and TIME. Right panel: NEXT (the piece queue) over SCORE
// and LINES. A quiet, frameless controls legend sits in the space left under
// the (shorter) left panel -- it's a reference, not something read every
// frame, so it carries the least visual weight. GameplayState pushes the
// numbers each frame with Set() and calls the OnX hooks so the matching row
// flashes; BoardRenderer draws the next queue inside NextPreviewArea().
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

	// A controls-legend entry: the action name over the key(s) bound to it.
	struct ControlEntry
	{
		sf::Text action;
		sf::Text keys;
	};

	[[nodiscard]] StatRow MakeStatRow(std::string_view labelKey, std::string_view initialValue,
		float centreX, float rowTop) const;

	void DrawStatRow(sf::RenderTarget& target, const StatRow& row) const;
	void DrawValue(sf::RenderTarget& target, const sf::Text& value, float flash) const;

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

	// Controls legend: no frame, sits under the shorter left panel.
	sf::Text legendTitle;
	std::vector<ControlEntry> legendEntries;
	bool showControls = true;
};
