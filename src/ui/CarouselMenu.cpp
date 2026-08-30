#include "CarouselMenu.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <utility>

#include <SFML/Graphics/ConvexShape.hpp>
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

	// Fly-in: the entries feed in as one straight row from off-screen right,
	// travelling behind the title, and each one merges onto the ring exactly
	// like a car joining a roundabout -- same speed, same spacing, following
	// the one ahead. The whole thing is parameterised by an "unrolled angle":
	// a <= 0 is a point on the ring, a > 0 is the straight tangent continuing
	// off to the right (lifting up to title level as it goes).
	constexpr float IntroDuration = 1.5f;
	constexpr float IntroWindup = 7.2f;       // radians the lead entry travels in from
	constexpr float RowScale = 0.8f;
	constexpr float RowAlpha = 0.75f;
	constexpr float LiftSpan = QuarterTurn;   // over how much of the tangent the row rises to title level

	// Click arrows either side of the front entry.
	constexpr float ArrowOffsetX = 360.f;    // from the front slot centre
	constexpr sf::Vector2f ArrowHalfSize{ 46.f, 58.f };
	constexpr float ArrowHitPadding = 16.f;

	// Where each entry (by index) ends up, as an unrolled angle. The row winds
	// on through negative angles, so the lead entry -- the one bound for the
	// slot furthest around -- has the most negative target.
	//                                    Start   Options  Quit    Records
	constexpr float IntroTarget[4] = { 0.f, -3.f * QuarterTurn, -2.f * QuarterTurn, -1.f * QuarterTurn };
	constexpr float IntroLeadTarget = -3.f * QuarterTurn;

	[[nodiscard]] float Lerp(float a, float b, float t) noexcept
	{
		return a + (b - a) * t;
	}

	[[nodiscard]] float EaseOutCubic(float t) noexcept
	{
		const float inv = 1.f - std::clamp(t, 0.f, 1.f);
		return 1.f - inv * inv * inv;
	}

	[[nodiscard]] float SmoothStep(float t) noexcept
	{
		t = std::clamp(t, 0.f, 1.f);
		return t * t * (3.f - 2.f * t);
	}

	[[nodiscard]] sf::Vector2f EllipsePos(sf::Vector2f center, float a) noexcept
	{
		return { center.x + std::sin(a) * RadiusX, center.y + SideBaseY + std::cos(a) * DepthDropY };
	}

	// Position at unrolled angle `a`: on the ring for a <= 0, on the straight
	// tangent (rising to title level) for a > 0. C1-continuous at a = 0.
	[[nodiscard]] sf::Vector2f PathPos(sf::Vector2f center, float a) noexcept
	{
		if (a <= 0.f)
		{
			return EllipsePos(center, a);
		}

		const sf::Vector2f base = EllipsePos(center, 0.f);
		const float lift = SmoothStep(a / LiftSpan);
		return { base.x + a * RadiusX, Lerp(base.y, center.y, lift) };
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

		if (introTimer >= 1.f || items.size() != 4)
		{
			return placement;
		}

		// Fly-in. Every entry rides the same path at the same rate; the lead
		// entry's unrolled angle sweeps linearly from off-screen right down to
		// its target, and each other entry sits a fixed offset behind it.
		const float head = Lerp(IntroLeadTarget + IntroWindup, IntroLeadTarget, introTimer);
		const float pathAngle = head + (IntroTarget[index] - IntroLeadTarget);

		placement.position = PathPos(center, pathAngle);

		if (pathAngle <= 0.f)
		{
			const float ct = (std::cos(pathAngle) + 1.f) * 0.5f;
			placement.scale = Lerp(ScaleBack, ScaleFront, ct);
			placement.alpha = 0.10f + 0.90f * std::pow(ct, 1.6f);
			placement.depth = std::cos(pathAngle);
		}
		else
		{
			const float lift = SmoothStep(pathAngle / LiftSpan);
			placement.scale = Lerp(RowScale, ScaleFront, lift);
			placement.alpha = Lerp(RowAlpha, 1.f, lift);
			placement.depth = -1.f;   // still behind the title
		}

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

		if (IsReady())
		{
			DrawArrow(target, -1);
			DrawArrow(target, 1);
		}
	}

	sf::Vector2f CarouselMenu::FrontSlotPosition() const
	{
		return { center.x, center.y + SideBaseY + DepthDropY };
	}

	sf::FloatRect CarouselMenu::FrontItemBounds() const
	{
		if (items.empty())
		{
			return {};
		}

		const std::size_t front = FrontItem();
		const Placement placement = PlacementOf(front);

		sf::Text label = items[front].text;
		label.setPosition(placement.position);
		label.setScale({ placement.scale, placement.scale });
		return label.getGlobalBounds();
	}

	sf::FloatRect CarouselMenu::ArrowBounds(int side) const
	{
		const sf::Vector2f centreOfArrow{
			FrontSlotPosition().x + static_cast<float>(side) * ArrowOffsetX,
			FrontSlotPosition().y };

		return {
			{ centreOfArrow.x - ArrowHalfSize.x - ArrowHitPadding,
			  centreOfArrow.y - ArrowHalfSize.y - ArrowHitPadding },
			{ 2.f * (ArrowHalfSize.x + ArrowHitPadding),
			  2.f * (ArrowHalfSize.y + ArrowHitPadding) } };
	}

	void CarouselMenu::DrawArrow(sf::RenderTarget& target, int side) const
	{
		const sf::Vector2f c{
			FrontSlotPosition().x + static_cast<float>(side) * ArrowOffsetX,
			FrontSlotPosition().y };
		const float dir = static_cast<float>(side);   // -1 points left, +1 points right

		sf::ConvexShape arrow(3);
		arrow.setPoint(0, { c.x - dir * ArrowHalfSize.x, c.y });
		arrow.setPoint(1, { c.x + dir * ArrowHalfSize.x, c.y - ArrowHalfSize.y });
		arrow.setPoint(2, { c.x + dir * ArrowHalfSize.x, c.y + ArrowHalfSize.y });

		const bool hot = hoveredArrow == side;
		arrow.setFillColor(sf::Color(255, 255, 255, hot ? 230 : 120));
		target.draw(arrow);
	}

	void CarouselMenu::PointerMoved(sf::Vector2f point)
	{
		hoveredArrow = 0;
		if (!IsReady())
		{
			return;
		}

		if (ArrowBounds(-1).contains(point))
		{
			hoveredArrow = -1;
		}
		else if (ArrowBounds(1).contains(point))
		{
			hoveredArrow = 1;
		}
	}

	CarouselMenu::PointerHit CarouselMenu::PointerPressed(sf::Vector2f point)
	{
		if (!IsReady())
		{
			return PointerHit::None;
		}

		if (ArrowBounds(-1).contains(point))
		{
			RotateLeft();
			return PointerHit::Rotated;
		}
		if (ArrowBounds(1).contains(point))
		{
			RotateRight();
			return PointerHit::Rotated;
		}
		if (FrontItemBounds().contains(point))
		{
			Activate();
			return PointerHit::Activated;
		}

		return PointerHit::None;
	}
}
