#include "BoardCallouts.h"

#include <algorithm>
#include <cstdint>
#include <utility>

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

#include "BoardRenderer.h"
#include "../core/Context.h"
#include "../gameplay/Board.h"
#include "../resources/Assets.h"
#include "../ui/Easing.h"

namespace
{
	// Centred over the well -- clear of the HOLD/NEXT panels either side.
	constexpr sf::Vector2f Anchor{
		BoardRenderer::BoardPosition.x + static_cast<float>(Board::WIDTH) * BoardRenderer::BlockSize * 0.5f,
		BoardRenderer::BoardPosition.y + static_cast<float>(Board::VisibleHeight) * BoardRenderer::BlockSize * 0.5f
	};

	void CentreText(sf::Text& text, sf::Vector2f centre)
	{
		const sf::FloatRect bounds = text.getLocalBounds();
		text.setOrigin({ bounds.position.x + bounds.size.x * 0.5f, bounds.position.y + bounds.size.y * 0.5f });
		text.setPosition(centre);
	}

	[[nodiscard]] sf::Color Brighten(sf::Color colour, float amount)
	{
		return sf::Color(
			static_cast<std::uint8_t>(UI::Easing::Lerp(static_cast<float>(colour.r), 255.f, amount)),
			static_cast<std::uint8_t>(UI::Easing::Lerp(static_cast<float>(colour.g), 255.f, amount)),
			static_cast<std::uint8_t>(UI::Easing::Lerp(static_cast<float>(colour.b), 255.f, amount)),
			colour.a);
	}
}

BoardCallouts::BoardCallouts(Context& context)
	: context(context)
	, glow(context.shaders.Get(Assets::ShaderID::NeonDilate), context.shaders.Get(Assets::ShaderID::NeonBlur))
{
	// No code
}

void BoardCallouts::Show(std::vector<Line> lines, int rank, sf::Color accent)
{
	const int clampedRank = std::max(0, rank);

	activeLines.clear();
	activeLines.reserve(lines.size());

	const sf::Font& font = context.fonts.Get(Assets::FontID::Main);

	for (const Line& line : lines)
	{
		sf::Text text(font, line.text, line.size);
		text.setFillColor(line.colour);
		text.setLetterSpacing(1.2f);
		text.setOutlineColor(sf::Color::Black);
		text.setOutlineThickness(3.f);
		activeLines.push_back({ std::move(text), line.colour });
	}

	showing = true;
	timer = 0.f;
	duration = BaseDuration + static_cast<float>(clampedRank) * DurationPerRank;
	chromatic = clampedRank >= ChromaticRankThreshold;

	accentColour = accent;
	flashTimer = 0.f;
	flashRadius = 70.f + static_cast<float>(clampedRank) * 16.f;

	dust.Emit(Anchor, { flashRadius, flashRadius }, accent, 20 + clampedRank * 7);
}

void BoardCallouts::Update(float deltaTime)
{
	if (showing)
	{
		timer += deltaTime;
		if (timer >= duration)
		{
			showing = false;
			activeLines.clear();
		}
	}

	if (flashTimer < FlashDuration)
	{
		flashTimer += deltaTime;
	}

	dust.Update(deltaTime);
	glow.Update(deltaTime);
}

void BoardCallouts::Render(sf::RenderTarget& target)
{
	// The flash burst sits behind everything, and finishes well before the
	// text or the particles do.
	if (flashTimer < FlashDuration)
	{
		const float ease = UI::Easing::EaseOutCubic(flashTimer / FlashDuration);
		const float radius = flashRadius * (0.7f + 0.5f * ease);
		const auto alpha = static_cast<std::uint8_t>((1.f - ease) * 150.f);

		sf::CircleShape burst(radius);
		burst.setOrigin({ radius, radius });
		burst.setPosition(Anchor);
		burst.setFillColor(sf::Color(accentColour.r, accentColour.g, accentColour.b, alpha));
		target.draw(burst);
	}

	dust.Render(target);

	if (!showing || activeLines.empty())
	{
		return;
	}

	const float t = timer / duration;
	const float yOffset = -RiseDistance * UI::Easing::EaseOutCubic(std::min(t, 1.f));

	float fadeAlpha = 1.f;
	if (t > HoldFraction)
	{
		fadeAlpha = 1.f - (t - HoldFraction) / (1.f - HoldFraction);
	}
	fadeAlpha = UI::Easing::Clamp01(fadeAlpha);

	// The punch-in: a springy overshoot down to full size (EaseOutBack pushes
	// past 1 partway through, which -- interpolating *toward* 1 from above --
	// reads as the text overshooting small and popping back up to size).
	const float popT = UI::Easing::Clamp01(timer / PopDuration);
	const float scale = UI::Easing::Lerp(PopStartScale, 1.f, UI::Easing::EaseOutBack(popT));
	const float punch = 1.f - popT;   // 1 at the moment of impact, 0 once settled

	const float totalHeight = LineSpacing * static_cast<float>(activeLines.size() - 1);
	float y = Anchor.y + yOffset - totalHeight * 0.5f;

	for (const ActiveLine& line : activeLines)
	{
		sf::Text drawn = line.text;

		sf::Color fillColour = Brighten(line.baseColour, punch * 0.5f);
		fillColour.a = static_cast<std::uint8_t>(fadeAlpha * 255.f);
		drawn.setFillColor(fillColour);

		sf::Color outlineColour = drawn.getOutlineColor();
		outlineColour.a = fillColour.a;
		drawn.setOutlineColor(outlineColour);

		drawn.setScale({ scale, scale });
		CentreText(drawn, { Anchor.x, y });

		const sf::FloatRect glowArea{
			{ Anchor.x - GlowBoxSize.x * 0.5f, y - GlowBoxSize.y * 0.5f },
			GlowBoxSize
		};

		glow.Draw(target, glowArea,
			[&](sf::RenderTarget& buffer, const sf::RenderStates& states) { buffer.draw(drawn, states); },
			accentColour, false);

		// A brief chromatic split on the way in, for the rarest callouts only.
		if (chromatic && punch > 0.02f)
		{
			const auto splitAlpha = static_cast<std::uint8_t>(punch * 140.f);

			sf::Text cyanGhost = drawn;
			cyanGhost.move({ -ChromaticOffset * punch, 0.f });
			cyanGhost.setFillColor(sf::Color(80, 220, 255, splitAlpha));
			cyanGhost.setOutlineColor(sf::Color(0, 0, 0, 0));
			target.draw(cyanGhost);

			sf::Text redGhost = drawn;
			redGhost.move({ ChromaticOffset * punch, 0.f });
			redGhost.setFillColor(sf::Color(255, 90, 90, splitAlpha));
			redGhost.setOutlineColor(sf::Color(0, 0, 0, 0));
			target.draw(redGhost);
		}

		target.draw(drawn);

		y += LineSpacing;
	}
}
