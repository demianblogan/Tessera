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

#include "ColourUtils.h"
#include "Easing.h"
#include "GlyphQuad.h"
#include "TetrominoPalette.h"
#include "../rendering/NeonGlow.h"

namespace
{
	constexpr float Pi = 3.14159265f;
	constexpr float TwoPi = 2.f * Pi;
	constexpr float QuarterTurn = Pi * 0.5f;

	constexpr float RadiusX = 540.f;      // horizontal spread of the side entries
	constexpr float SideBaseY = 150.f;    // a side entry sits this far below the centre
	constexpr float DepthDropY = 145.f;   // front drops this much more, back rises this much (back tucks behind the title)

	constexpr float ScaleBack = 0.42f;
	constexpr float ScaleFront = 1.12f;

	constexpr float RotateDuration = 0.26f;

	// Fly-in: the entries feed in as one straight row from off-screen right,
	// travelling behind the title, and each one merges onto the ring exactly
	// like a car joining a roundabout -- same speed, same spacing, following
	// the one ahead. The whole thing is parameterised by an "unrolled angle":
	// a <= 0 is a point on the ring, a > 0 is the straight tangent continuing
	// off to the right (lifting up to title level as it goes).
	constexpr float IntroDuration = 1.6f;
	constexpr float IntroWindup = 8.6f;       // radians the lead entry travels in from
	constexpr float SwooshTriggerAngle = 1.7f;   // fires as an entry crosses the right screen edge
	constexpr float RowScale = 0.8f;
	constexpr float RowAlpha = 0.75f;
	constexpr float LiftSpan = QuarterTurn;   // over how much of the tangent the row rises to title level

	// Click arrows hugging the front entry, pointing outward (the direction the
	// ring turns). The source sprite points up; it is rotated a quarter turn.
	constexpr float ArrowGap = 16.f;             // between the widest entry's edge and the arrow
	constexpr float ArrowHeightFraction = 0.85f; // arrow on-screen height vs the front entry's text height
	constexpr float ArrowHitPadding = 4.f;

	// Entry text styling: dark rim, vertical gradient fill in the entry's own
	// hue, soft drop shadow.
	constexpr float EntryOutlineThickness = 4.f;
	constexpr float EntryLetterSpacing = 0.09f;     // extra tracking, as a fraction of the char size
	constexpr sf::Vector2f EntryShadowOffset{ 5.f, 7.f };   // local units, before the entry scale
	constexpr float EntryShadowAlpha = 0.5f;
	constexpr float EntryGradientTopMix = 0.42f;    // toward white at the top edge
	constexpr float EntryGradientBottom = 0.5f;     // darken at the bottom edge
	constexpr float EntryOutlineDarken = 0.18f;

	// Depth cues for the side / back entries: they desaturate and go soft
	// (drawn as a smear of offset copies) the further round the ring they are.
	constexpr float EntryMaxDesaturate = 0.62f;
	constexpr float EntryMaxBlur = 9.f;    // local units at the very back
	constexpr float EntryBlurFalloff = 1.9f;   // >1 keeps the sides fairly sharp, blurs the back
	constexpr int EntryBlurTaps = 5;

	constexpr float EntryGlowIntensity = 0.55f;
	constexpr float EntryArrivalFlashDuration = 0.24f;

	// Idle "breath" of the front entry.
	constexpr float EntryBreathAmplitude = 0.018f;
	constexpr float EntryBreathSpeed = 2.1f;

	using UI::Darken;
	using UI::Desaturate;
	using UI::MixToWhite;
	using UI::ScaleRgb;

	// Activation feedback on the front entry: a quick scale-punch and flash.
	constexpr float ActivatePulseDuration = 0.18f;
	constexpr float ActivatePulseScale = 0.15f;   // extra scale at the peak of the punch
	constexpr float ActivatePulseFlash = 0.6f;    // how far toward white at the peak

	// Disintegration: particles emitted per non-front entry when the ring exits.
	constexpr int ExitDustPerEntry = 46;

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

	using UI::Easing::EaseOutCubic;
	using UI::Easing::Lerp;
	using UI::Easing::SmoothStep;

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
	CarouselMenu::CarouselMenu(const sf::Font& fontRef, unsigned int size, const sf::Texture& arrow)
		: font(fontRef)
		, characterSize(size)
		, arrowTexture(arrow)
	{
	}

	void CarouselMenu::AddItem(const sf::String& text, std::function<void()> onActivate, bool enabled,
		std::optional<sf::Color> colour)
	{
		sf::Text label(font, text, characterSize);
		const sf::FloatRect bounds = label.getLocalBounds();
		label.setOrigin({ bounds.position.x + bounds.size.x * 0.5f, bounds.position.y + bounds.size.y * 0.5f });

		maxItemHeight = std::max(maxItemHeight, bounds.size.y);

		const sf::Color entryColour = !enabled
			? UI::DisabledEntryColour
			: colour.value_or(UI::TetrominoColours[items.size() % UI::TetrominoColours.size()]);

		Item item{ std::move(label), std::move(onActivate), {}, 0.f, entryColour, enabled };

		// Walk the pen so each glyph can be drawn as its own quad (gradient fill
		// + a real dark outline glyph); `sf::Text::findCharacterPos` is deprecated
		// in this SFML build.
		const float tracking = EntryLetterSpacing * static_cast<float>(characterSize);

		std::vector<std::pair<char32_t, float>> raw;
		float penX = 0.f;
		char32_t previous = 0;
		float inkTop = 0.f;
		float inkBottom = 0.f;
		for (std::size_t i = 0; i < text.getSize(); ++i)
		{
			const char32_t codepoint = text[i];
			if (previous != 0)
			{
				penX += font.getKerning(previous, codepoint, characterSize) + tracking;
			}
			if (codepoint != U' ')
			{
				raw.push_back({ codepoint, penX });

				// Ink extent in baseline-relative coordinates, matching the quads.
				const sf::FloatRect gb = font.getGlyph(codepoint, characterSize, false).bounds;
				inkTop = std::min(inkTop, gb.position.y);
				inkBottom = std::max(inkBottom, gb.position.y + gb.size.y);
			}
			penX += font.getGlyph(codepoint, characterSize, false).advance;
			previous = codepoint;
		}

		item.inkCentreY = (inkTop + inkBottom) * 0.5f;

		const float halfWidth = penX * 0.5f;
		maxItemHalfWidth = std::max(maxItemHalfWidth, halfWidth);
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

	void CarouselMenu::SetFrontImmediate(std::size_t index)
	{
		if (items.empty())
		{
			return;
		}

		frontIndex = static_cast<int>(index % items.size());
		angle = static_cast<float>(frontIndex) * SlotStep();
		rotateFrom = angle;
		rotateTo = angle;
		rotateTimer = 1.f;
	}

	std::size_t CarouselMenu::CurrentFrontIndex() const
	{
		return FrontItem();
	}

	void CarouselMenu::SetSwooshCallback(std::function<void(std::size_t)> callback)
	{
		onSwoosh = std::move(callback);
	}

	void CarouselMenu::Begin()
	{
		started = true;
		swooshFired.assign(items.size(), 0);
	}

	void CarouselMenu::Skip()
	{
		started = true;
		introTimer = 1.f;
		swooshFired.assign(items.size(), 1);   // no swoosh when the intro is skipped
	}

	float CarouselMenu::IntroPathAngle(std::size_t index) const
	{
		const float step = SlotStep();
		const float count = static_cast<float>(items.size());
		const float introLead = -(count - 1.f) * step;
		const float introTargetI = index == 0
			? 0.f
			: -(count - static_cast<float>(index)) * step;

		const float head = Lerp(introLead + IntroWindup, introLead, introTimer);
		return head + (introTargetI - introLead);
	}

	bool CarouselMenu::IsReady() const
	{
		return started && introTimer >= 1.f;
	}

	std::size_t CarouselMenu::FrontItem() const
	{
		return items.empty() ? 0 : PositiveMod(frontIndex, items.size());
	}

	float CarouselMenu::SlotStep() const
	{
		return items.empty() ? QuarterTurn : TwoPi / static_cast<float>(items.size());
	}

	sf::Color CarouselMenu::FrontColour() const
	{
		return items.empty() ? UI::DisabledEntryColour : items[FrontItem()].colour;
	}

	void CarouselMenu::RotateLeft()
	{
		if (!IsReady())
		{
			return;
		}

		--frontIndex;
		rotateFrom = angle;
		rotateTo = static_cast<float>(frontIndex) * SlotStep();
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
		rotateTo = static_cast<float>(frontIndex) * SlotStep();
		rotateTimer = 0.f;
		arrowPressTime[1] = 0.f;
	}

	void CarouselMenu::Activate()
	{
		if (!IsReady() || items.empty())
		{
			return;
		}

		const Item& front = items[FrontItem()];
		if (front.enabled && front.activate)
		{
			front.activate();
		}
	}

	void CarouselMenu::PulseActivate()
	{
		activatePulseTime = 0.f;
	}

	void CarouselMenu::StartExit()
	{
		if (exiting)
		{
			return;
		}
		exiting = true;

		// Every entry except the front one (which the header takes over) bursts
		// into pixels from where it currently sits.
		for (std::size_t i = 0; i < items.size(); ++i)
		{
			if (i == FrontItem())
			{
				continue;
			}

			const Placement placement = PlacementOf(i);
			const sf::Vector2f size = items[i].text.getLocalBounds().size * placement.scale;
			dust.Emit(placement.position, size, items[i].colour, ExitDustPerEntry);
		}
	}

	sf::Vector2f CarouselMenu::FrontEntryCentre() const
	{
		return items.empty() ? center : PlacementOf(FrontItem()).position;
	}

	float CarouselMenu::FrontEntryHeight() const
	{
		if (items.empty())
		{
			return 0.f;
		}

		const std::size_t front = FrontItem();
		return items[front].text.getLocalBounds().size.y * PlacementOf(front).scale;
	}

	void CarouselMenu::Update(float deltaTime)
	{
		if (!started)
		{
			return;
		}

		const bool wasIntro = introTimer < 1.f;
		if (introTimer < 1.f)
		{
			introTimer = std::min(1.f, introTimer + deltaTime / IntroDuration);
		}

		// Fire the fly-in swoosh as each entry crosses the right screen edge.
		if (wasIntro && onSwoosh)
		{
			for (std::size_t i = 0; i < items.size() && i < swooshFired.size(); ++i)
			{
				if (!swooshFired[i] && IntroPathAngle(i) <= SwooshTriggerAngle)
				{
					swooshFired[i] = 1;
					onSwoosh(i);
				}
			}
		}

		if (rotateTimer < 1.f)
		{
			rotateTimer = std::min(1.f, rotateTimer + deltaTime / RotateDuration);
			angle = Lerp(rotateFrom, rotateTo, EaseOutCubic(rotateTimer));
			if (rotateTimer >= 1.f)
			{
				arrivalFlashTime = 0.f;   // an entry just locked to the front
			}
		}

		if (wasIntro && introTimer >= 1.f)
		{
			arrivalFlashTime = 0.f;
		}

		arrivalFlashTime += deltaTime;
		breathTime += deltaTime;
		activatePulseTime += deltaTime;
		dust.Update(deltaTime);

		for (float& pressTime : arrowPressTime)
		{
			pressTime += deltaTime;
		}
	}

	CarouselMenu::Placement CarouselMenu::PlacementOf(std::size_t index) const
	{
		// Resting place on the ring.
		const float a = static_cast<float>(index) * SlotStep() - angle;
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

		if (introTimer >= 1.f || items.size() < 2)
		{
			return placement;
		}

		// Fly-in. Every entry rides the same path at the same rate; the lead
		// entry's unrolled angle sweeps linearly from off-screen right down to
		// its target, and each other entry sits a fixed offset behind it.
		const float pathAngle = IntroPathAngle(index);

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
		// During the exit no entry is drawn: the front one is now the shell's
		// header, the rest are pixels (see RenderBack).
		if (exiting)
		{
			return;
		}

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

		// 0 at the back of the ring, 1 at the front.
		const float depthT = std::clamp((placement.depth + 1.f) * 0.5f, 0.f, 1.f);

		// A quick punch-and-flash on the front entry when it is activated.
		float pulse = 0.f;
		if (index == FrontItem() && activatePulseTime < ActivatePulseDuration)
		{
			pulse = std::sin((1.f - activatePulseTime / ActivatePulseDuration) * Pi);
		}

		// The front entry breathes very slightly.
		float scale = placement.scale;
		if (index == FrontItem())
		{
			scale *= BreathScale() * (1.f + ActivatePulseScale * pulse);
		}

		sf::Transform transform;
		transform.translate(placement.position);
		transform.scale({ scale, scale });
		transform.translate({ 0.f, -item.inkCentreY });

		sf::Color base = Desaturate(item.colour, (1.f - depthT) * EntryMaxDesaturate);
		const float whiten = std::max(ArrivalFlash(index), ActivatePulseFlash * pulse);
		if (whiten > 0.f)
		{
			base = MixToWhite(base, whiten);
		}

		// Side / back entries are drawn as a smear of offset copies (a cheap
		// depth-of-field blur); the alpha is split across the taps.
		const float blur = std::pow(1.f - depthT, EntryBlurFalloff) * EntryMaxBlur;
		const int taps = blur > 0.6f ? EntryBlurTaps : 0;
		const float tapFraction = taps == 0 ? 1.f : 1.f / (static_cast<float>(taps) * 0.55f + 1.f);
		const auto tapAlpha = static_cast<std::uint8_t>(alphaFraction * tapFraction * 255.f);

		const sf::Color shadowColour(0, 0, 0, static_cast<std::uint8_t>(EntryShadowAlpha * alphaFraction * 255.f));
		sf::Color outlineColour = Darken(base, EntryOutlineDarken);   outlineColour.a = tapAlpha;
		sf::Color fillTop = MixToWhite(base, EntryGradientTopMix);     fillTop.a = tapAlpha;
		sf::Color fillBottom = Darken(base, EntryGradientBottom);      fillBottom.a = tapAlpha;

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

		constexpr float Tau = 6.2831853f;
		for (int k = 0; k <= taps; ++k)
		{
			sf::Vector2f offset;
			if (k > 0)
			{
				const float a = static_cast<float>(k - 1) / static_cast<float>(taps) * Tau;
				offset = { std::cos(a) * blur, std::sin(a) * blur };
			}

			sf::RenderStates tapStates = states;
			tapStates.transform.translate(offset);
			target.draw(outline, tapStates);
			target.draw(fill, tapStates);
		}
	}

	float CarouselMenu::BreathScale() const
	{
		if (!IsReady())
		{
			return 1.f;
		}
		return 1.f + EntryBreathAmplitude * std::sin(breathTime * EntryBreathSpeed);
	}

	float CarouselMenu::ArrivalFlash(std::size_t index) const
	{
		if (index != FrontItem() || !items[index].enabled)
		{
			return 0.f;
		}
		return std::clamp(1.f - arrivalFlashTime / EntryArrivalFlashDuration, 0.f, 1.f);
	}

	void CarouselMenu::RenderBack(sf::RenderTarget& target) const
	{
		if (exiting)
		{
			dust.Render(target);
			return;
		}

		Render(target, false);
	}

	void CarouselMenu::DrawFrontGlow(sf::RenderTarget& target, NeonGlow& glow) const
	{
		if (items.empty() || !items[FrontItem()].enabled)
		{
			return;
		}

		const std::size_t index = FrontItem();
		const Placement placement = PlacementOf(index);
		if (placement.depth < 0.15f)
		{
			return;
		}

		const Item& item = items[index];
		const sf::Vector2f box{ maxItemHalfWidth * 2.6f + 60.f, maxItemHeight * 2.f };
		const sf::FloatRect area{
			{ placement.position.x - box.x * 0.5f, placement.position.y - box.y * 0.5f }, box };

		const float scale = placement.scale * BreathScale();

		sf::Transform transform;
		transform.translate(placement.position);
		transform.scale({ scale, scale });
		transform.translate({ 0.f, -item.inkCentreY });

		float strength = EntryGlowIntensity * std::clamp(placement.depth, 0.f, 1.f);
		strength = std::max(strength, 0.7f * ArrivalFlash(index));
		const sf::Color tint = ScaleRgb(item.colour, strength);

		glow.Draw(target, area,
			[this, &item, &transform](sf::RenderTarget& buffer, const sf::RenderStates& states)
			{
				sf::VertexArray white(sf::PrimitiveType::Triangles);
				for (const Item::Glyph& glyph : item.glyphs)
				{
					AppendGlyphQuad(white, glyph.penX,
						font.getGlyph(glyph.codepoint, characterSize, false),
						sf::Color::White, sf::Color::White);
				}

				sf::RenderStates s = states;
				s.transform *= transform;
				s.texture = &font.getTexture(characterSize);
				buffer.draw(white, s);
			},
			tint, false);
	}

	void CarouselMenu::RenderFront(sf::RenderTarget& target, NeonGlow* glow) const
	{
		if (glow != nullptr && IsReady() && !exiting)
		{
			DrawFrontGlow(target, *glow);
		}

		Render(target, true);

		if (IsReady() && !exiting)
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
		const ArrowGeom g = ComputeArrow(FrontSlotPosition(), maxItemHalfWidth * ScaleFront, maxItemHeight,
			arrowTexture.getSize(), side);
		return {
			{ g.centre.x - g.halfExtent.x - ArrowHitPadding, g.centre.y - g.halfExtent.y - ArrowHitPadding },
			{ 2.f * (g.halfExtent.x + ArrowHitPadding), 2.f * (g.halfExtent.y + ArrowHitPadding) } };
	}

	void CarouselMenu::DrawArrow(sf::RenderTarget& target, int side) const
	{
		const ArrowGeom g = ComputeArrow(FrontSlotPosition(), maxItemHalfWidth * ScaleFront, maxItemHeight,
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
		if (!items.empty() && items[FrontItem()].enabled && FrontItemBounds().contains(point))
		{
			Activate();
			return PointerHit::Activated;
		}

		return PointerHit::None;
	}
}
