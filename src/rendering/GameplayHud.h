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

// The in-game HUD: square nine-slice frames hugging each side of the board --
// HOLD and LEVEL on the left, NEXT / SCORE / LINES / TIME on the right -- plus
// an always-on controls legend under the left column. GameplayState pushes the
// numbers each frame with Set() and calls the OnX hooks so the matching cell
// flashes; BoardRenderer draws the upcoming pieces inside NextPreviewArea().
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

	[[nodiscard]] bool NextVisible() const;
	[[nodiscard]] sf::FloatRect NextPreviewArea() const { return nextCellBounds; }

private:
	// One framed square: the decorative border, a dark inner fill, the caption
	// pinned near its top, and a decaying flash (0..1) for the "just changed" pop.
	struct Cell
	{
		sf::RectangleShape fill;
		UI::NineSliceFrame frame;
		sf::Text caption;
		float flash = 0.f;
		bool visible = true;
	};

	// A controls-legend entry: the action name over the key(s) bound to it.
	struct ControlEntry
	{
		sf::Text action;
		sf::Text keys;
	};

	Cell MakeCell(std::string_view captionKey, sf::FloatRect bounds);
	void DrawCell(sf::RenderTarget& target, const Cell& cell) const;
	void DrawValue(sf::RenderTarget& target, const sf::Text& value, float flash) const;

	Context& context;

	std::vector<Cell> cells;

	sf::Text scoreValue;
	sf::Text levelValue;
	sf::Text linesValue;
	sf::Text timeValue;

	UI::NineSliceFrame legendFrame;
	sf::RectangleShape legendFill;
	sf::Text legendTitle;
	std::vector<ControlEntry> legendEntries;
	bool showControls = true;

	sf::FloatRect nextCellBounds;
};
