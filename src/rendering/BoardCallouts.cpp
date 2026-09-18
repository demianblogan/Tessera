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
#include "../utils/Easing.h"

namespace
{
	// Centered over the well -- clear of the HOLD/NEXT panels either side.
	constexpr sf::Vector2f Anchor
	{
		BoardRenderer::BoardPosition.x + static_cast<float>(Board::Width) * BoardRenderer::BlockSize * 0.5f,
		BoardRenderer::BoardPosition.y + static_cast<float>(Board::VisibleHeight) * BoardRenderer::BlockSize * 0.5f
	};

	void CenterText(sf::Text& text, sf::Vector2f center)
	{
		const sf::FloatRect bounds = text.getLocalBounds();
		text.setOrigin({ bounds.position.x + bounds.size.x * 0.5f, bounds.position.y + bounds.size.y * 0.5f });
		text.setPosition(center);
	}

	[[nodiscard]] sf::Color Brighten(sf::Color color, float amount)
	{
		return sf::Color(
			static_cast<std::uint8_t>(Easing::Lerp(static_cast<float>(color.r), 255.f, amount)),
			static_cast<std::uint8_t>(Easing::Lerp(static_cast<float>(color.g), 255.f, amount)),
			static_cast<std::uint8_t>(Easing::Lerp(static_cast<float>(color.b), 255.f, amount)),
			color.a);
	}
}

BoardCallouts::BoardCallouts(Context& context)
	: context(context)
	, glow(context.shaders.Get(Assets::ShaderID::NeonDilate), context.shaders.Get(Assets::ShaderID::NeonBlur))
{}

void BoardCallouts::Show(std::vector<Line> lines, int rank, sf::Color accent)
{
	const int clampedRank = std::max(0, rank);

	activeLines.clear();
	activeLines.reserve(lines.size());

	const sf::Font& font = context.fonts.Get(Assets::FontID::Main);

	for (const Line& line : lines)
	{
		sf::Text text(font, line.text, line.size);
		text.setFillColor(line.color);
		text.setLetterSpacing(TextLetterSpacing);
		text.setOutlineColor(sf::Color::Black);
		text.setOutlineThickness(TextOutlineThickness);
		activeLines.push_back({ std::move(text), line.color });
	}

	isShowing = true;
	timer = 0.f;
	duration = BaseDuration + static_cast<float>(clampedRank) * DurationPerRank;
	isChromatic = clampedRank >= ChromaticRankThreshold;

	accentColor = accent;
	flashTimer = 0.f;
	flashRadius = FlashBaseRadius + static_cast<float>(clampedRank) * FlashRadiusPerRank;

	dust.Emit(Anchor, { flashRadius, flashRadius }, accent, DustBaseCount + clampedRank * DustCountPerRank);
}

void BoardCallouts::Update(float deltaTime)
{
	if (isShowing)
	{
		timer += deltaTime;
		if (timer >= duration)
		{
			isShowing = false;
			activeLines.clear();
		}
	}

	if (flashTimer < FlashDuration)
		flashTimer += deltaTime;

	dust.Update(deltaTime);
	glow.Update(deltaTime);
}

void BoardCallouts::Render(sf::RenderTarget& target)
{
	// The flash burst sits behind everything, and finishes well before the
	// text or the particles do.
	if (flashTimer < FlashDuration)
	{
		const float ease = Easing::EaseOutCubic(flashTimer / FlashDuration);
		const float radius = flashRadius * (FlashRadiusStartFraction + FlashRadiusGrowth * ease);
		const auto alpha = static_cast<std::uint8_t>((1.f - ease) * FlashMaxAlpha);

		sf::CircleShape burst(radius);
		burst.setOrigin({ radius, radius });
		burst.setPosition(Anchor);
		burst.setFillColor(sf::Color(accentColor.r, accentColor.g, accentColor.b, alpha));

		target.draw(burst);
	}

	dust.Render(target);

	if (!isShowing || activeLines.empty())
		return;

	const float progress = timer / duration;
	const float yOffset = -RiseDistance * Easing::EaseOutCubic(std::min(progress, 1.f));

	float fadeAlpha = 1.f;
	if (progress > HoldFraction)
		fadeAlpha = 1.f - (progress - HoldFraction) / (1.f - HoldFraction);

	fadeAlpha = Easing::Clamp01(fadeAlpha);

	// The punch-in: a springy overshoot down to full size (EaseOutBack pushes
	// past 1 partway through, which -- interpolating *toward* 1 from above --
	// reads as the text overshooting small and popping back up to size).
	const float popT = Easing::Clamp01(timer / PopDuration);
	const float scale = Easing::Lerp(PopStartScale, 1.f, Easing::EaseOutBack(popT));
	const float punch = 1.f - popT;   // 1 at the moment of impact, 0 once settled

	const float totalHeight = LineSpacing * static_cast<float>(activeLines.size() - 1);
	float y = Anchor.y + yOffset - totalHeight * 0.5f;

	for (const ActiveLine& line : activeLines)
	{
		sf::Text drawn = line.text;

		sf::Color fillColor = Brighten(line.baseColor, punch * PunchBrightenAmount);
		fillColor.a = static_cast<std::uint8_t>(fadeAlpha * 255.f);
		drawn.setFillColor(fillColor);

		sf::Color outlineColor = drawn.getOutlineColor();
		outlineColor.a = fillColor.a;
		drawn.setOutlineColor(outlineColor);

		drawn.setScale({ scale, scale });
		CenterText(drawn, { Anchor.x, y });

		const sf::FloatRect glowArea
		{
			{ Anchor.x - GlowBoxSize.x * 0.5f, y - GlowBoxSize.y * 0.5f },
			GlowBoxSize
		};

		glow.Draw(target, glowArea,
			[&](sf::RenderTarget& buffer, const sf::RenderStates& states) { buffer.draw(drawn, states); },
			accentColor, false);

		// A brief chromatic split on the way in, for the rarest callouts only.
		if (isChromatic && punch > 0.02f)
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
