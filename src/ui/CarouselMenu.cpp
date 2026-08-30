#include "CarouselMenu.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <utility>

#include <SFML/Graphics/RenderTarget.hpp>

namespace
{
	constexpr float Pi = 3.14159265f;
	constexpr float QuarterTurn = Pi * 0.5f;

	constexpr float RadiusX = 520.f;      // horizontal spread of the side entries
	constexpr float SideBaseY = 235.f;    // a side entry sits this far below the centre
	constexpr float DepthDropY = 110.f;   // front drops this much more, back rises this much

	constexpr float ScaleBack = 0.5f;
	constexpr float ScaleFront = 1.f;

	constexpr float RotateDuration = 0.26f;

	// Fly-in: the entries start as a straight horizontal row off to the right,
	// slide left behind the title, and each one curls onto its ring slot once
	// it clears the title's left edge.
	constexpr float IntroDuration = 0.9f;
	constexpr float RowHeadStartX = 880.f;   // leftmost entry starts this far right of centre
	constexpr float RowSpacing = 300.f;      // gap between entries in the row
	constexpr float WrapEdgeX = -380.f;      // x (relative to centre) where an entry starts curling
	constexpr float WrapSpan = 320.f;        // how much further it travels while curling
	constexpr float RowScale = 0.8f;
	constexpr float RowAlpha = 0.75f;
	constexpr float FrontThreshold = 0.55f;  // wrap progress past which a front entry draws in front

	[[nodiscard]] float Lerp(float a, float b, float t) noexcept
	{
		return a + (b - a) * t;
	}

	[[nodiscard]] float EaseOutCubic(float t) noexcept
	{
		const float inv = 1.f - std::clamp(t, 0.f, 1.f);
		return 1.f - inv * inv * inv;
	}

	[[nodiscard]] std::size_t PositiveMod(int value, std::size_t modulus) noexcept
	{
		const int m = static_cast<int>(modulus);
		return static_cast<std::size_t>(((value % m) + m) % m);
	}
}

namespace UI
{
	CarouselMenu::CarouselMenu(const sf::Font& fontRef, unsigned int size)
		: font(fontRef)
		, characterSize(size)
	{
	}

	void CarouselMenu::AddItem(const sf::String& text, std::function<void()> onActivate)
	{
		sf::Text label(font, text, characterSize);
		const sf::FloatRect bounds = label.getLocalBounds();
		label.setOrigin({ bounds.position.x + bounds.size.x * 0.5f, bounds.position.y + bounds.size.y * 0.5f });

		items.push_back({ std::move(label), std::move(onActivate) });
	}

	void CarouselMenu::SetCenter(sf::Vector2f newCenter)
	{
		center = newCenter;
	}

	void CarouselMenu::Begin()
	{
		started = true;
	}

	bool CarouselMenu::IsReady() const
	{
		return started && introTimer >= 1.f;
	}

	std::size_t CarouselMenu::FrontItem() const
	{
		return items.empty() ? 0 : PositiveMod(frontIndex, items.size());
	}

	void CarouselMenu::RotateLeft()
	{
		if (!IsReady())
		{
			return;
		}

		--frontIndex;
		rotateFrom = angle;
		rotateTo = static_cast<float>(frontIndex) * QuarterTurn;
		rotateTimer = 0.f;
	}

	void CarouselMenu::RotateRight()
	{
		if (!IsReady())
		{
			return;
		}

		++frontIndex;
		rotateFrom = angle;
		rotateTo = static_cast<float>(frontIndex) * QuarterTurn;
		rotateTimer = 0.f;
	}

	void CarouselMenu::Activate()
	{
		if (!IsReady() || items.empty())
		{
			return;
		}

		items[FrontItem()].activate();
	}

	void CarouselMenu::Update(float deltaTime)
	{
		if (!started)
		{
			return;
		}

		if (introTimer < 1.f)
		{
			introTimer = std::min(1.f, introTimer + deltaTime / IntroDuration);
		}

		if (rotateTimer < 1.f)
		{
			rotateTimer = std::min(1.f, rotateTimer + deltaTime / RotateDuration);
			angle = Lerp(rotateFrom, rotateTo, EaseOutCubic(rotateTimer));
		}
	}

	CarouselMenu::Placement CarouselMenu::PlacementOf(std::size_t index) const
	{
		// Resting place on the ring.
		const float a = static_cast<float>(index) * QuarterTurn - angle;
		const float ringDepth = std::cos(a);
		const float t = (ringDepth + 1.f) * 0.5f;   // 0 at the back, 1 at the front

		const sf::Vector2f ringPos{
			center.x + std::sin(a) * RadiusX,
			center.y + SideBaseY + ringDepth * DepthDropY };
		const float ringScale = Lerp(ScaleBack, ScaleFront, t);
		const float ringAlpha = 0.10f + 0.90f * std::pow(t, 1.6f);

		Placement placement;
		placement.depth = ringDepth;
		placement.position = ringPos;
		placement.scale = ringScale;
		placement.alpha = ringAlpha;

		if (introTimer >= 1.f)
		{
			return placement;
		}

		// The straight row, sliding left. Entry 0 leads; the rest trail to its
		// right. Total travel is set so the last entry finishes curling exactly
		// as introTimer reaches 1.
		const float lastLineX = center.x + RowHeadStartX + static_cast<float>(items.size() - 1) * RowSpacing;
		const float totalTravel = (lastLineX - (center.x + WrapEdgeX)) + WrapSpan;
		const float travel = introTimer * totalTravel;

		const float lineX = center.x + RowHeadStartX + static_cast<float>(index) * RowSpacing - travel;
		const sf::Vector2f linePos{ lineX, center.y };

		// 0 while still a straight row, 1 once fully curled onto the ring.
		const float wrap = std::clamp(((center.x + WrapEdgeX) - lineX) / WrapSpan, 0.f, 1.f);
		const float w = EaseOutCubic(wrap);

		// Quadratic curve bowed toward the title centre -- the "curl".
		const sf::Vector2f mid{ (linePos.x + ringPos.x) * 0.5f, (linePos.y + ringPos.y) * 0.5f };
		const sf::Vector2f ctrl{ Lerp(mid.x, center.x, 0.45f), Lerp(mid.y, center.y, 0.45f) };
		const float inv = 1.f - w;
		placement.position = {
			inv * inv * linePos.x + 2.f * inv * w * ctrl.x + w * w * ringPos.x,
			inv * inv * linePos.y + 2.f * inv * w * ctrl.y + w * w * ringPos.y };
		placement.scale = Lerp(RowScale, ringScale, w);
		placement.alpha = Lerp(RowAlpha, ringAlpha, w);
		placement.depth = w >= FrontThreshold ? Lerp(-1.f, ringDepth, w) : -1.f;

		return placement;
	}

	void CarouselMenu::Render(sf::RenderTarget& target, bool frontHalf) const
	{
		struct Drawn { std::size_t index; Placement placement; };
		std::vector<Drawn> drawList;

		for (std::size_t i = 0; i < items.size(); ++i)
		{
			const Placement placement = PlacementOf(i);

			// PlacementOf reports depth < 0 for anything still behind the title
			// (including entries mid-curl during the fly-in).
			const bool isFront = placement.depth >= 0.f;
			if (isFront == frontHalf)
			{
				drawList.push_back({ i, placement });
			}
		}

		std::sort(drawList.begin(), drawList.end(),
			[](const Drawn& lhs, const Drawn& rhs) { return lhs.placement.depth < rhs.placement.depth; });

		for (const Drawn& entry : drawList)
		{
			sf::Text label = items[entry.index].text;
			label.setPosition(entry.placement.position);
			label.setScale({ entry.placement.scale, entry.placement.scale });
			label.setFillColor(sf::Color(255, 255, 255,
				static_cast<std::uint8_t>(std::clamp(entry.placement.alpha, 0.f, 1.f) * 255.f)));
			target.draw(label);
		}
	}

	void CarouselMenu::RenderBack(sf::RenderTarget& target) const
	{
		Render(target, false);
	}

	void CarouselMenu::RenderFront(sf::RenderTarget& target) const
	{
		Render(target, true);
	}
}
