#include "CarouselMenu.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <utility>

#include <SFML/Graphics/BlendMode.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Glyph.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/System/Angle.hpp>

namespace
{
	constexpr float Pi = 3.14159265f;
	constexpr float QuarterTurn = Pi * 0.5f;

	constexpr float RadiusX = 540.f;      // horizontal spread of the side entries
	constexpr float SideBaseY = 150.f;    // a side entry sits this far below the centre
	constexpr float DepthDropY = 145.f;   // front drops this much more, back rises this much (back tucks behind the title)

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

	// Click arrows hugging the front entry, pointing outward (the direction the
	// ring turns). The source sprite points up; it is rotated a quarter turn.
	constexpr float ArrowGap = 16.f;             // between the widest entry's edge and the arrow
	constexpr float ArrowHeightFraction = 0.85f; // arrow on-screen height vs the front entry's text height
	constexpr float ArrowHitPadding = 4.f;

	// Entry text styling: dark rim, vertical gradient fill, soft drop shadow.
	constexpr float EntryOutlineThickness = 3.f;
	constexpr sf::Color EntryOutlineColour{ 10, 10, 16 };
	constexpr sf::Vector2f EntryShadowOffset{ 5.f, 7.f };   // local units, before the entry scale
	constexpr float EntryShadowAlpha = 0.5f;
	constexpr sf::Color EntryFillTop{ 255, 255, 255 };
	constexpr sf::Color EntryFillBottom{ 150, 158, 176 };

	// Press feedback (no pressed sprite -- faked with a squash, an inward
	// nudge, a warm tint, and a quick orange ring).
	constexpr float ArrowPressDuration = 0.22f;
	constexpr float ArrowPressDip = 0.22f;       // scale reduction at the peak
	constexpr float ArrowPressShift = 8.f;       // px pushed inward at the peak
	constexpr sf::Color ArrowPressTint{ 255, 150, 60 };

	// The press ring is a soft, dense orange haze -- many overlapping additive
	// bands rather than one thin outline.
	constexpr sf::Color ArrowPulseColour{ 255, 138, 46 };
	constexpr float ArrowPulseRadiusStart = 10.f;
	constexpr float ArrowPulseRadiusEnd = 40.f;
	constexpr int ArrowPulseBands = 9;
	constexpr float ArrowPulseBandSpread = 22.f;  // total radial thickness of the haze
	constexpr float ArrowPulseBandWidth = 9.f;

	struct ArrowGeom
	{
		sf::Vector2f centre;
		float scale = 1.f;
		sf::Vector2f halfExtent;   // on-screen, after the quarter-turn rotation
	};

	[[nodiscard]] ArrowGeom ComputeArrow(sf::Vector2f frontSlot, float maxHalfWidth, float itemHeight,
		sf::Vector2u textureSize, int side) noexcept
	{
		// After a +/-90 turn the texture's width runs vertically on screen.
		const float screenHeight = std::max(12.f, itemHeight * ArrowHeightFraction);
		const float scale = screenHeight / std::max(1.f, static_cast<float>(textureSize.x));
		const sf::Vector2f halfExtent{
			static_cast<float>(textureSize.y) * scale * 0.5f,
			static_cast<float>(textureSize.x) * scale * 0.5f };
		const float x = frontSlot.x + static_cast<float>(side) * (maxHalfWidth + ArrowGap + halfExtent.x);
		return { { x, frontSlot.y }, scale, halfExtent };
	}

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

	// One glyph quad (two triangles) into `array`, in string-local space:
	// `penX` is the pen origin, `offset` a local nudge (for the shadow).
	void AppendGlyphQuad(sf::VertexArray& array, float penX, const sf::Glyph& glyph,
		sf::Color topColour, sf::Color bottomColour, sf::Vector2f offset = {})
	{
		const sf::FloatRect bounds = glyph.bounds;
		const sf::FloatRect tex(glyph.textureRect);

		const float x0 = penX + bounds.position.x + offset.x;
		const float x1 = x0 + bounds.size.x;
		const float y0 = bounds.position.y + offset.y;
		const float y1 = y0 + bounds.size.y;

		const float u0 = tex.position.x;
		const float u1 = tex.position.x + tex.size.x;
		const float v0 = tex.position.y;
		const float v1 = tex.position.y + tex.size.y;

		array.append({ { x0, y0 }, topColour, { u0, v0 } });
		array.append({ { x1, y0 }, topColour, { u1, v0 } });
		array.append({ { x1, y1 }, bottomColour, { u1, v1 } });
		array.append({ { x0, y0 }, topColour, { u0, v0 } });
		array.append({ { x1, y1 }, bottomColour, { u1, v1 } });
		array.append({ { x0, y1 }, bottomColour, { u0, v1 } });
	}
}

namespace UI
{
	CarouselMenu::CarouselMenu(const sf::Font& fontRef, unsigned int size, const sf::Texture& arrow)
		: font(fontRef)
		, characterSize(size)
		, arrowTexture(arrow)
	{
	}

	void CarouselMenu::AddItem(const sf::String& text, std::function<void()> onActivate)
	{
		sf::Text label(font, text, characterSize);
		const sf::FloatRect bounds = label.getLocalBounds();
		label.setOrigin({ bounds.position.x + bounds.size.x * 0.5f, bounds.position.y + bounds.size.y * 0.5f });

		maxItemHalfWidth = std::max(maxItemHalfWidth, bounds.size.x * 0.5f);
		maxItemHeight = std::max(maxItemHeight, bounds.size.y);

		Item item{ std::move(label), std::move(onActivate), {}, bounds.position.y + bounds.size.y * 0.5f };

		// Walk the pen so each glyph can be drawn as its own quad (gradient fill
		// + a real dark outline glyph); `sf::Text::findCharacterPos` is deprecated
		// in this SFML build.
		std::vector<std::pair<char32_t, float>> raw;
		float penX = 0.f;
		char32_t previous = 0;
		for (std::size_t i = 0; i < text.getSize(); ++i)
		{
			const char32_t codepoint = text[i];
			if (previous != 0)
			{
				penX += font.getKerning(previous, codepoint, characterSize);
			}
			if (codepoint != U' ')
			{
				raw.push_back({ codepoint, penX });
			}
			penX += font.getGlyph(codepoint, characterSize, false).advance;
			previous = codepoint;
		}

		const float halfWidth = penX * 0.5f;
		for (const auto& [codepoint, x] : raw)
		{
			item.glyphs.push_back({ codepoint, x - halfWidth });
		}

		items.push_back(std::move(item));
	}

	void CarouselMenu::SetCenter(sf::Vector2f newCenter)
	{
		center = newCenter;
	}

	void CarouselMenu::Begin()
	{
		started = true;
	}

	void CarouselMenu::Skip()
	{
		started = true;
		introTimer = 1.f;
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
		arrowPressTime[0] = 0.f;
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
		arrowPressTime[1] = 0.f;
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

		for (float& pressTime : arrowPressTime)
		{
			pressTime += deltaTime;
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
			DrawEntry(target, entry.index, entry.placement);
		}
	}

	void CarouselMenu::DrawEntry(sf::RenderTarget& target, std::size_t index, const Placement& placement) const
	{
		const Item& item = items[index];
		const float alphaFraction = std::clamp(placement.alpha, 0.f, 1.f);
		const auto alpha = static_cast<std::uint8_t>(alphaFraction * 255.f);
		if (alpha == 0u || item.glyphs.empty())
		{
			return;
		}

		sf::Transform transform;
		transform.translate(placement.position);
		transform.scale({ placement.scale, placement.scale });
		transform.translate({ 0.f, -item.inkCentreY });

		sf::Color shadowColour(0, 0, 0, static_cast<std::uint8_t>(EntryShadowAlpha * alphaFraction * 255.f));
		sf::Color outlineColour = EntryOutlineColour;   outlineColour.a = alpha;
		sf::Color fillTop = EntryFillTop;               fillTop.a = alpha;
		sf::Color fillBottom = EntryFillBottom;         fillBottom.a = alpha;

		sf::VertexArray shadow(sf::PrimitiveType::Triangles);
		sf::VertexArray outline(sf::PrimitiveType::Triangles);
		sf::VertexArray fill(sf::PrimitiveType::Triangles);

		for (const Item::Glyph& glyph : item.glyphs)
		{
			const sf::Glyph& body = font.getGlyph(glyph.codepoint, characterSize, false);
			const sf::Glyph& rim = font.getGlyph(glyph.codepoint, characterSize, false, EntryOutlineThickness);

			AppendGlyphQuad(shadow, glyph.penX, body, shadowColour, shadowColour, EntryShadowOffset);
			AppendGlyphQuad(outline, glyph.penX, rim, outlineColour, outlineColour);
			AppendGlyphQuad(fill, glyph.penX, body, fillTop, fillBottom);
		}

		sf::RenderStates states;
		states.transform = transform;
		states.texture = &font.getTexture(characterSize);

		target.draw(shadow, states);
		target.draw(outline, states);
		target.draw(fill, states);
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
		const ArrowGeom g = ComputeArrow(FrontSlotPosition(), maxItemHalfWidth, maxItemHeight,
			arrowTexture.getSize(), side);
		return {
			{ g.centre.x - g.halfExtent.x - ArrowHitPadding, g.centre.y - g.halfExtent.y - ArrowHitPadding },
			{ 2.f * (g.halfExtent.x + ArrowHitPadding), 2.f * (g.halfExtent.y + ArrowHitPadding) } };
	}

	void CarouselMenu::DrawArrow(sf::RenderTarget& target, int side) const
	{
		const ArrowGeom g = ComputeArrow(FrontSlotPosition(), maxItemHalfWidth, maxItemHeight,
			arrowTexture.getSize(), side);

		const std::size_t idx = side < 0 ? 0u : 1u;
		const float press = std::clamp(1.f - arrowPressTime[idx] / ArrowPressDuration, 0.f, 1.f);

		// A soft, dense orange haze ring, expanding and fading as it settles.
		if (press > 0.f)
		{
			const float spread = EaseOutCubic(1.f - press);
			const float radius = Lerp(ArrowPulseRadiusStart, ArrowPulseRadiusEnd, spread);
			const float coreAlpha = press * press;   // fades a touch faster than linear

			sf::RenderStates additive;
			additive.blendMode = sf::BlendAdd;

			for (int band = 0; band < ArrowPulseBands; ++band)
			{
				const float t = static_cast<float>(band) / static_cast<float>(ArrowPulseBands - 1);   // 0..1
				const float offset = (t - 0.5f) * ArrowPulseBandSpread;
				const float bandRadius = std::max(1.f, radius + offset);
				// Brightest in the middle of the band, faint at the edges.
				const float weight = 1.f - std::abs(t - 0.5f) * 2.f;

				sf::CircleShape ring(bandRadius);
				ring.setOrigin({ bandRadius, bandRadius });
				ring.setPosition(g.centre);
				ring.setFillColor(sf::Color::Transparent);
				ring.setOutlineThickness(ArrowPulseBandWidth);
				ring.setOutlineColor(sf::Color(ArrowPulseColour.r, ArrowPulseColour.g, ArrowPulseColour.b,
					static_cast<std::uint8_t>(coreAlpha * weight * weight * 90.f)));
				target.draw(ring, additive);
			}
		}

		// The arrow itself: squashed and nudged inward while pressed, tinted warm.
		const float scale = g.scale * (1.f - ArrowPressDip * press);
		const sf::Vector2f centre{ g.centre.x - static_cast<float>(side) * ArrowPressShift * press, g.centre.y };

		const std::uint8_t restAlpha = hoveredArrow == side ? 255u : 150u;
		const sf::Color colour{
			static_cast<std::uint8_t>(255 - (255 - ArrowPressTint.r) * press),
			static_cast<std::uint8_t>(255 - (255 - ArrowPressTint.g) * press),
			static_cast<std::uint8_t>(255 - (255 - ArrowPressTint.b) * press),
			static_cast<std::uint8_t>(restAlpha + (255 - restAlpha) * press) };

		sf::Sprite arrow(arrowTexture);
		arrow.setOrigin(sf::Vector2f(arrowTexture.getSize()) * 0.5f);
		arrow.setScale({ scale, scale });
		// Source points up; a quarter turn aims it away from the entry -- left
		// for a left click (ring turns left), right for a right click.
		arrow.setRotation(sf::degrees(static_cast<float>(side) * 90.f));
		arrow.setPosition(centre);
		arrow.setColor(colour);
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
			return PointerHit::RotatedLeft;
		}
		if (ArrowBounds(1).contains(point))
		{
			RotateRight();
			return PointerHit::RotatedRight;
		}
		if (FrontItemBounds().contains(point))
		{
			Activate();
			return PointerHit::Activated;
		}

		return PointerHit::None;
	}
}
