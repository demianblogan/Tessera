#pragma once

#include <vector>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/System/String.hpp>
#include <SFML/System/Vector2.hpp>

#include "../primitives/NeonGlow.h"
#include "../primitives/PixelDust.h"

struct Context;

namespace sf
{
	class RenderTarget;
}

// Short-lived on-board popups for scoring highlights (TETRIS, T-SPIN DOUBLE,
// BACK-TO-BACK, PERFECT CLEAR, a combo count): a burst of color and
// particles greets them, the text punches in oversized with a springy
// overshoot and a neon bloom (the same bloom the active piece and the main
// menu title use), settles, rises, then fades. Purely a presentation detail:
// GameplayState decides *what* happened and hands it text plus a rank; this
// only knows how to animate them.
class BoardCallouts
{
public:
	// One line of a callout: its text, color and size, decided by the caller
	// (a Tetris reads very differently from a Perfect Clear).
	struct Line
	{
		sf::String text;
		sf::Color color;
		unsigned int size = 40;
	};

	explicit BoardCallouts(Context& context);

	// Shows a new callout, stacked top to bottom in the order given, replacing
	// whatever was still showing. `rank` (0 = least special, higher = rarer)
	// stretches how long it stays up; `accentColor` drives the flash, glow and
	// particle burst that greet it. The very top ranks also get a brief
	// chromatic-split punch.
	void Show(std::vector<Line> lines, int rank, sf::Color accentColor);

	void Update(float deltaTime);

	// Not const: the neon glow it draws with keeps its own GPU-side scratch
	// buffers, the same way BoardRenderer's does.
	void Render(sf::RenderTarget& target);

private:
	static constexpr float BaseDuration = 1.2f;
	static constexpr float DurationPerRank = 0.2f;
	static constexpr float HoldFraction = 0.4f;   // stays fully visible this long before fading
	static constexpr float RiseDistance = 40.f;
	static constexpr float LineSpacing = 100.f;

	// The punch-in: scales down from PopStartScale to 1 with a springy
	// overshoot (Easing::EaseOutBack), settling well before the hold ends.
	static constexpr float PopDuration = 0.4f;
	static constexpr float PopStartScale = 2.3f;

	// A quick two-tone chromatic split during the punch, for ranks at or above
	// this (Tetris, back-to-back, Perfect Clear -- not a plain Double).
	static constexpr int ChromaticRankThreshold = 3;
	static constexpr float ChromaticOffset = 5.f;

	// The flash burst and particle scatter that greet a callout -- much
	// shorter-lived than the text itself.
	static constexpr float FlashDuration = 0.3f;
	static constexpr float FlashBaseRadius = 70.f;
	static constexpr float FlashRadiusPerRank = 16.f;
	// The circle grows from FlashRadiusStartFraction to
	// FlashRadiusStartFraction + FlashRadiusGrowth of its final radius as it fades.
	static constexpr float FlashRadiusStartFraction = 0.7f;
	static constexpr float FlashRadiusGrowth = 0.5f;
	static constexpr float FlashMaxAlpha = 150.f;

	static constexpr int DustBaseCount = 20;
	static constexpr int DustCountPerRank = 7;

	// How much whiter the text flashes at the moment of impact (punch == 1),
	// easing back to its base color as the pop settles.
	static constexpr float PunchBrightenAmount = 0.5f;

	static constexpr float TextLetterSpacing = 1.2f;
	static constexpr float TextOutlineThickness = 3.f;

	// Fixed so the neon glow never has to resize its internal buffers between
	// lines of different lengths (that thrashes -- see NeonGlow's own notes).
	// Wide enough for the longest realistic line ("BACK-TO-BACK T-SPIN TRIPLE"
	// at the top rank's font size) with room to spare.
	static constexpr sf::Vector2f GlowBoxSize{ 1750.f, 190.f };

	struct ActiveLine
	{
		sf::Text text;
		sf::Color baseColor;
	};

	Context& context;

	std::vector<ActiveLine> activeLines;
	bool isShowing = false;
	float timer = 0.f;
	float duration = 0.f;
	bool isChromatic = false;

	sf::Color accentColor = sf::Color::White;
	float flashTimer = FlashDuration;   // >= FlashDuration means the burst is over
	float flashRadius = 0.f;

	PixelDust dust;
	NeonGlow glow;
};
