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

	constexpr float IntroDuration = 0.55f;
	constexpr float RotateDuration = 0.26f;
	constexpr float EntryOffsetX = 1150.f; // how far right the ring starts before flying in

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
		const float a = static_cast<float>(index) * QuarterTurn - angle;
		const float depth = std::cos(a);
		const float t = (depth + 1.f) * 0.5f;   // 0 at the back, 1 at the front

		Placement placement;
		placement.depth = depth;
		placement.position = {
			center.x + std::sin(a) * RadiusX,
			center.y + SideBaseY + depth * DepthDropY };
		placement.scale = Lerp(ScaleBack, ScaleFront, t);
		placement.alpha = 0.10f + 0.90f * std::pow(t, 1.6f);

		if (introTimer < 1.f)
		{
			const float e = EaseOutCubic(introTimer);
			placement.position.x += (1.f - e) * EntryOffsetX;
			placement.alpha *= e;
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

			// Everything passes behind the title during the fly-in.
			const bool isFront = introTimer >= 1.f && placement.depth >= 0.f;
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
