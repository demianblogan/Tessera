#include "MenuHeader.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

namespace
{
	constexpr unsigned int HeaderTextSize = 110;
	constexpr sf::Vector2f HeaderCentre{ 960.f, 120.f };

	constexpr float RiseDuration = 0.28f;
	constexpr float SinkDuration = 0.24f;

	[[nodiscard]] float EaseOutCubic(float t) noexcept
	{
		const float inv = 1.f - std::clamp(t, 0.f, 1.f);
		return 1.f - inv * inv * inv;
	}

	[[nodiscard]] float Lerp(float a, float b, float t) noexcept
	{
		return a + (b - a) * t;
	}

	[[nodiscard]] sf::Vector2f Lerp(sf::Vector2f a, sf::Vector2f b, float t) noexcept
	{
		return { Lerp(a.x, b.x, t), Lerp(a.y, b.y, t) };
	}
}

namespace UI
{
	MenuHeader::MenuHeader(const sf::Font& font)
		: text(font, "", HeaderTextSize)
	{
	}

	float MenuHeader::NaturalHeight() const
	{
		const float height = text.getLocalBounds().size.y;
		return height > 1.f ? height : 1.f;
	}

	void MenuHeader::RiseFrom(sf::Vector2f fromCentre, float fromHeight, const sf::String& label, sf::Color newColour)
	{
		colour = newColour;
		text.setString(label);

		fromPosition = fromCentre;
		toPosition = HeaderCentre;
		fromScale = std::max(fromHeight, 1.f) / NaturalHeight();
		toScale = 1.f;

		mode = Mode::Rising;
		timer = 0.f;
	}

	void MenuHeader::SinkTo(sf::Vector2f toCentre, float toHeight)
	{
		fromPosition = HeaderCentre;
		toPosition = toCentre;
		fromScale = 1.f;
		toScale = std::max(toHeight, 1.f) / NaturalHeight();

		mode = Mode::Sinking;
		timer = 0.f;
	}

	void MenuHeader::Update(float deltaTime)
	{
		if (mode == Mode::Rising)
		{
			timer = std::min(1.f, timer + deltaTime / RiseDuration);
			if (timer >= 1.f)
			{
				mode = Mode::Shown;
			}
		}
		else if (mode == Mode::Sinking)
		{
			timer = std::min(1.f, timer + deltaTime / SinkDuration);
			if (timer >= 1.f)
			{
				mode = Mode::Hidden;
			}
		}
	}

	void MenuHeader::Render(sf::RenderTarget& target) const
	{
		if (mode == Mode::Hidden)
		{
			return;
		}

		float scale = toScale;
		sf::Vector2f position = toPosition;
		float alpha = 1.f;

		if (mode == Mode::Rising)
		{
			const float e = EaseOutCubic(timer);
			position = Lerp(fromPosition, toPosition, e);
			scale = Lerp(fromScale, toScale, e);
			alpha = std::min(1.f, timer * 1.6f);
		}
		else if (mode == Mode::Sinking)
		{
			const float e = EaseOutCubic(timer);
			position = Lerp(fromPosition, toPosition, e);
			scale = Lerp(fromScale, toScale, e);
			alpha = 1.f - timer;
		}

		const sf::FloatRect bounds = text.getLocalBounds();
		text.setOrigin(
			{
				bounds.position.x + bounds.size.x * 0.5f,
				bounds.position.y + bounds.size.y * 0.5f
			});
		text.setPosition(position);
		text.setScale({ scale, scale });
		text.setFillColor(sf::Color(colour.r, colour.g, colour.b,
			static_cast<std::uint8_t>(std::clamp(alpha, 0.f, 1.f) * 255.f)));

		target.draw(text);
	}

	bool MenuHeader::IsIdle() const
	{
		return mode == Mode::Hidden;
	}

	bool MenuHeader::IsSettled() const
	{
		return mode == Mode::Shown;
	}
}
