#pragma once

#include <cstddef>
#include <functional>
#include <optional>
#include <vector>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/System/String.hpp>
#include <SFML/System/Vector2.hpp>

#include "PixelDust.h"

class NeonGlow;

namespace sf
{
	class Font;
	class RenderTarget;
	class Texture;
}

namespace UI
{
	// A ring of menu entries seen at a slight 3D angle: the front entry sits
	// large and bright below the title, the two sides are smaller and dimmer,
	// and the back entry is tucked up behind the title, almost invisible.
	// Left / right navigation rotates the ring so a neighbour swings to the
	// front. On entry the whole ring flies in from the right.
	//
	// Draw order is split: RenderBack() before the title, RenderFront() after,
	// so the back entry really does pass behind it.
	class CarouselMenu
	{
	public:
		CarouselMenu(const sf::Font& font, unsigned int characterSize, const sf::Texture& arrowTexture);

		// A disabled entry (enabled == false) is greyed, has no glow, and cannot
		// be activated -- but the ring still rotates through it. `colour`
		// overrides the default per-slot tetromino hue.
		void AddItem(const sf::String& text, std::function<void()> onActivate, bool enabled = true,
			std::optional<sf::Color> colour = std::nullopt);
		void SetCenter(sf::Vector2f center);

		// Put `index` at the front with no rotation animation. For rebuilding the
		// ring already focused on a particular entry (e.g. returning from that
		// entry's sub-screen). Call after the items are added.
		void SetFrontImmediate(std::size_t index);

		// The index of the entry currently at the front.
		[[nodiscard]] std::size_t CurrentFrontIndex() const;

		// Called with the entry index as that entry swishes in past the screen
		// edge during the fly-in.
		void SetSwooshCallback(std::function<void(std::size_t)> callback);

		// Kick off the fly-in. Until it finishes IsReady() is false and the
		// rotate / activate calls do nothing.
		void Begin();
		void Skip();                       // jump straight to the settled ring
		[[nodiscard]] bool IsReady() const;

		// The front entry's hue (grey while the ring is empty).
		[[nodiscard]] sf::Color FrontColour() const;

		void RotateLeft();
		void RotateRight();
		void Activate();

		// A quick scale-punch and flash on the front entry, to register a press
		// before the transition proper begins.
		void PulseActivate();

		// Play the ring's exit: the front entry and its arrows stop drawing (the
		// shell's header takes the entry's place) and every other entry bursts
		// into pixels.
		void StartExit();

		// The front entry's on-screen centre and ink height, for handing off to
		// the header at the start of a transition.
		[[nodiscard]] sf::Vector2f FrontEntryCentre() const;
		[[nodiscard]] float FrontEntryHeight() const;

		// Mouse. The caller maps the pixel to view coordinates first.
		enum class PointerHit { None, RotatedLeft, RotatedRight, Activated };
		PointerHit PointerPressed(sf::Vector2f point);
		void PointerMoved(sf::Vector2f point);

		void Update(float deltaTime);
		void RenderBack(sf::RenderTarget& target) const;
		void RenderFront(sf::RenderTarget& target, NeonGlow* glow = nullptr) const;

	private:
		struct Item
		{
			struct Glyph
			{
				char32_t codepoint = 0;
				float penX = 0.f;   // pen origin, relative to the string centre
			};

			sf::Text text;                 // kept for hit-testing / metrics
			std::function<void()> activate;
			std::vector<Glyph> glyphs;
			float inkCentreY = 0.f;        // vertical ink centre of the string
			sf::Color colour;              // this entry's hue (grey if disabled)
			bool enabled = true;
		};

		struct Placement
		{
			sf::Vector2f position;
			float scale = 1.f;
			float alpha = 1.f;
			float depth = 0.f;   // +1 front, -1 back
		};

		[[nodiscard]] Placement PlacementOf(std::size_t index) const;
		void Render(sf::RenderTarget& target, bool frontHalf) const;
		void DrawEntry(sf::RenderTarget& target, std::size_t index, const Placement& placement) const;
		void DrawFrontGlow(sf::RenderTarget& target, NeonGlow& glow) const;
		[[nodiscard]] float ArrivalFlash(std::size_t index) const;
		[[nodiscard]] float BreathScale() const;   // front-entry idle pulse, else 1
		[[nodiscard]] float SlotStep() const;      // angle between adjacent entries
		[[nodiscard]] float IntroPathAngle(std::size_t index) const;
		[[nodiscard]] std::size_t FrontItem() const;

		[[nodiscard]] sf::Vector2f FrontSlotPosition() const;
		[[nodiscard]] sf::FloatRect FrontItemBounds() const;
		[[nodiscard]] sf::FloatRect ArrowBounds(int side) const;   // side: -1 left, +1 right
		void DrawArrow(sf::RenderTarget& target, int side) const;

		const sf::Font& font;
		unsigned int characterSize;
		const sf::Texture& arrowTexture;

		std::vector<Item> items;
		sf::Vector2f center;
		float maxItemHalfWidth = 0.f;   // half the widest entry, for anchoring the arrows
		float maxItemHeight = 0.f;      // tallest entry, for sizing the arrows (stable per switch)

		int frontIndex = 0;          // which item is at the front (may be < 0 or >= size)
		float angle = 0.f;           // current ring rotation, radians
		float rotateFrom = 0.f;
		float rotateTo = 0.f;
		float rotateTimer = 1.f;     // >= 1 means settled

		bool started = false;
		bool exiting = false;        // playing the transition-out
		float introTimer = 0.f;      // 0..1 across the fly-in

		float activatePulseTime = 1000.f;   // seconds since PulseActivate(); large = no pulse
		PixelDust dust;              // the disintegrating entries during the exit

		float arrivalFlashTime = 1000.f;   // seconds since an entry last locked to the front
		float breathTime = 0.f;            // drives the front entry's idle breath

		std::function<void(std::size_t)> onSwoosh;
		std::vector<char> swooshFired;     // one flag per entry, for the fly-in swoosh

		int hoveredArrow = 0;        // -1 left, +1 right, 0 none

		// Seconds since each arrow (0 = left, 1 = right) was last pressed; large
		// means "not pressed", which reads as no feedback.
		float arrowPressTime[2] = { 1000.f, 1000.f };
	};
}
