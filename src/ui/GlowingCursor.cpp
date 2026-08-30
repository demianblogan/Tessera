#include "GlowingCursor.h"

#include <cmath>
#include <cstdint>

#include <SFML/Graphics/BlendMode.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Texture.hpp>

namespace
{
	// Where the click point sits inside the 32x32 image -- top-left for a
	// classic pointer.
	constexpr sf::Vector2f Hotspot{ 0.f, 0.f };

	// Neon cyan, matching the board / menu glow.
	constexpr std::uint8_t GlowR = 120;
	constexpr std::uint8_t GlowG = 210;
	constexpr std::uint8_t GlowB = 255;

	constexpr float PulseSpeed = 5.5f;   // radians per second
	constexpr float MinGlow = 0.20f;
	constexpr float MaxGlow = 0.85f;
}

namespace UI
{
	GlowingCursor::GlowingCursor(const sf::Texture& texture)
		: sprite(texture)
	{
		sprite.setOrigin(Hotspot);
	}

	void GlowingCursor::Update(float deltaTime)
	{
		pulseTime += deltaTime;
	}

	void GlowingCursor::Render(sf::RenderTarget& target, sf::Vector2f position)
	{
		sprite.setPosition(position);

		sprite.setColor(sf::Color::White);
		target.draw(sprite);

		const float wave = 0.5f + 0.5f * std::sin(pulseTime * PulseSpeed);
		const float strength = MinGlow + (MaxGlow - MinGlow) * wave;

		sprite.setColor(sf::Color(GlowR, GlowG, GlowB, static_cast<std::uint8_t>(strength * 255.f)));
		target.draw(sprite, sf::RenderStates(sf::BlendAdd));
	}
}
