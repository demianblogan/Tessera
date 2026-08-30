#pragma once

#include <cstddef>
#include <functional>
#include <vector>

#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/System/String.hpp>
#include <SFML/System/Vector2.hpp>

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

		void AddItem(const sf::String& text, std::function<void()> onActivate);
		void SetCenter(sf::Vector2f center);

		// Kick off the fly-in. Until it finishes IsReady() is false and the
		// rotate / activate calls do nothing.
		void Begin();
		void Skip();                       // jump straight to the settled ring
		[[nodiscard]] bool IsReady() const;

		void RotateLeft();
		void RotateRight();
		void Activate();

		// Mouse. The caller maps the pixel to view coordinates first.
		enum class PointerHit { None, RotatedLeft, RotatedRight, Activated };
		PointerHit PointerPressed(sf::Vector2f point);
		void PointerMoved(sf::Vector2f point);

		void Update(float deltaTime);
		void RenderBack(sf::RenderTarget& target) const;
		void RenderFront(sf::RenderTarget& target) const;

	private:
		struct Item
		{
			sf::Text text;
			std::function<void()> activate;
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
		float introTimer = 0.f;      // 0..1 across the fly-in

		int hoveredArrow = 0;        // -1 left, +1 right, 0 none

		// Seconds since each arrow (0 = left, 1 = right) was last pressed; large
		// means "not pressed", which reads as no feedback.
		float arrowPressTime[2] = { 1000.f, 1000.f };
	};
}
