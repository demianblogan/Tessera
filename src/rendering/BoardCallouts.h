#pragma once

#include <vector>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/System/String.hpp>

struct Context;

namespace sf
{
	class RenderTarget;
}

// Short-lived on-board popups for scoring highlights (TETRIS, T-SPIN DOUBLE,
// BACK-TO-BACK, PERFECT CLEAR, a combo count) -- rises and fades over the
// well. Purely a presentation detail: GameplayState decides *what* happened
// and hands it a few lines of text to show; this only knows how to animate them.
class BoardCallouts
{
public:
	// One line of a callout: its text, colour and size, all decided by the
	// caller (a Tetris clear vs. a Perfect Clear reads very differently).
	struct Line
	{
		sf::String text;
		sf::Color colour;
		unsigned int size = 40;
	};

	explicit BoardCallouts(Context& context);

	// Shows a new callout, stacked top to bottom in the order given. Replaces
	// whatever was still fading from a previous one.
	void Show(std::vector<Line> lines);

	void Update(float deltaTime);
	void Render(sf::RenderTarget& target) const;

private:
	static constexpr float Duration = 1.1f;
	static constexpr float HoldFraction = 0.35f;   // stays fully visible this long before fading
	static constexpr float RiseDistance = 46.f;
	static constexpr float LineSpacing = 50.f;

	Context& context;
	std::vector<sf::Text> activeLines;
	float timer = Duration;   // >= Duration means nothing is showing
};
