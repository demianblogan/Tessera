#include "BoardCallouts.h"

#include <algorithm>
#include <cstdint>
#include <utility>

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
	// Centred over the well, in its upper third -- clear of the HOLD/NEXT
	// panels either side, and away from wherever the active piece usually is.
	constexpr sf::Vector2f Anchor{
		BoardRenderer::BoardPosition.x + static_cast<float>(Board::WIDTH) * BoardRenderer::BlockSize * 0.5f,
		BoardRenderer::BoardPosition.y + static_cast<float>(Board::VisibleHeight) * BoardRenderer::BlockSize * 0.32f
	};

	void CentreText(sf::Text& text, sf::Vector2f centre)
	{
		const sf::FloatRect bounds = text.getLocalBounds();
		text.setOrigin({ bounds.position.x + bounds.size.x * 0.5f, bounds.position.y + bounds.size.y * 0.5f });
		text.setPosition(centre);
	}
}

BoardCallouts::BoardCallouts(Context& context)
	: context(context)
{
	// No code
}

void BoardCallouts::Show(std::vector<Line> lines)
{
	activeLines.clear();
	activeLines.reserve(lines.size());

	const sf::Font& font = context.fonts.Get(Assets::FontID::Main);

	for (const Line& line : lines)
	{
		sf::Text text(font, line.text, line.size);
		text.setFillColor(line.colour);
		text.setLetterSpacing(1.2f);
		text.setOutlineColor(sf::Color(0, 0, 0, line.colour.a));
		text.setOutlineThickness(2.f);
		activeLines.push_back(std::move(text));
	}

	timer = 0.f;
}

void BoardCallouts::Update(float deltaTime)
{
	if (timer < Duration)
	{
		timer += deltaTime;
	}
}

void BoardCallouts::Render(sf::RenderTarget& target) const
{
	if (timer >= Duration || activeLines.empty())
	{
		return;
	}

	const float t = timer / Duration;
	const float yOffset = -RiseDistance * UI::Easing::EaseOutCubic(t);

	float alpha = 1.f;
	if (t > HoldFraction)
	{
		alpha = 1.f - (t - HoldFraction) / (1.f - HoldFraction);
	}
	alpha = UI::Easing::Clamp01(alpha);

	const float totalHeight = LineSpacing * static_cast<float>(activeLines.size() - 1);
	float y = Anchor.y + yOffset - totalHeight * 0.5f;

	for (const sf::Text& line : activeLines)
	{
		sf::Text fading = line;

		sf::Color fillColour = fading.getFillColor();
		fillColour.a = static_cast<std::uint8_t>(alpha * 255.f);
		fading.setFillColor(fillColour);

		sf::Color outlineColour = fading.getOutlineColor();
		outlineColour.a = fillColour.a;
		fading.setOutlineColor(outlineColour);

		CentreText(fading, { Anchor.x, y });
		target.draw(fading);

		y += LineSpacing;
	}
}
