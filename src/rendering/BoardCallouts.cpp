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
		text.setOutlineThickness(2.f);
		activeLines.push_back({ std::move(text), line.colour });
	}

	showing = true;
	timer = 0.f;
	duration = BaseDuration + static_cast<float>(clampedRank) * DurationPerRank;

	accentColour = accent;
	flashTimer = 0.f;
	flashRadius = 60.f + static_cast<float>(clampedRank) * 14.f;

	dust.Emit(Anchor, { flashRadius, flashRadius }, accent, 16 + clampedRank * 6);
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
}

void BoardCallouts::Render(sf::RenderTarget& target) const
{
	// The flash burst sits behind the text and particles, and finishes well
	// before either does.
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

	float alpha = 1.f;
	if (t > HoldFraction)
	{
		alpha = 1.f - (t - HoldFraction) / (1.f - HoldFraction);
	}
	alpha = UI::Easing::Clamp01(alpha);

	// Every line pops in bright (its own colour, flashed toward white) and
	// eases back to its real colour and size over the first instant on screen.
	const float pop = 1.f - UI::Easing::Clamp01(timer / TextPopDuration);
	const float scale = 1.f + 0.25f * pop;

	const float totalHeight = LineSpacing * static_cast<float>(activeLines.size() - 1);
	float y = Anchor.y + yOffset - totalHeight * 0.5f;

	for (const ActiveLine& line : activeLines)
	{
		sf::Text drawn = line.text;

		sf::Color fillColour = Brighten(line.baseColour, pop * 0.85f);
		fillColour.a = static_cast<std::uint8_t>(alpha * 255.f);
		drawn.setFillColor(fillColour);

		sf::Color outlineColour = drawn.getOutlineColor();
		outlineColour.a = fillColour.a;
		drawn.setOutlineColor(outlineColour);

		drawn.setScale({ scale, scale });
		CentreText(drawn, { Anchor.x, y });
		target.draw(drawn);

		y += LineSpacing;
	}
}
