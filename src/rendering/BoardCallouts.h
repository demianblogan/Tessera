#pragma once

#include <vector>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/System/String.hpp>

#include "../ui/PixelDust.h"

struct Context;

namespace sf
{
	class RenderTarget;
}

// Short-lived on-board popups for scoring highlights (TETRIS, T-SPIN DOUBLE,
// BACK-TO-BACK, PERFECT CLEAR, a combo count): a burst of colour and
// particles greets them, the text pops in bright and settles, rises, then
// fades. Purely a presentation detail: GameplayState decides *what* happened
// and hands it text plus a rank; this only knows how to animate them.
class BoardCallouts
{
public:
	// One line of a callout: its text, colour and size, decided by the caller
	// (a Tetris reads very differently from a Perfect Clear).
	struct Line
	{
		sf::String text;
		sf::Color colour;
		unsigned int size = 40;
	};

	explicit BoardCallouts(Context& context);

	// Shows a new callout, stacked top to bottom in the order given, replacing
	// whatever was still showing. `rank` (0 = least special, higher = rarer)
	// stretches how long it stays up; `accentColour` drives the flash and
	// particle burst that greet it.
	void Show(std::vector<Line> lines, int rank, sf::Color accentColour);

	void Update(float deltaTime);
	void Render(sf::RenderTarget& target) const;

private:
	static constexpr float BaseDuration = 1.1f;
	static constexpr float DurationPerRank = 0.2f;
	static constexpr float HoldFraction = 0.35f;   // stays fully visible this long before fading
	static constexpr float RiseDistance = 30.f;
	static constexpr float LineSpacing = 54.f;

	// The text's own "just appeared" pop: brightens toward white and scales up
	// slightly, settling back to its real colour/size over this long.
	static constexpr float TextPopDuration = 0.22f;

	// The flash burst and particle scatter that greet a callout -- much
	// shorter-lived than the text itself.
	static constexpr float FlashDuration = 0.3f;

	struct ActiveLine
	{
		sf::Text text;
		sf::Color baseColour;
	};

	Context& context;

	std::vector<ActiveLine> activeLines;
	bool showing = false;
	float timer = 0.f;
	float duration = 0.f;

	sf::Color accentColour = sf::Color::White;
	float flashTimer = FlashDuration;   // >= FlashDuration means the burst is over
	float flashRadius = 0.f;

	UI::PixelDust dust;
};
