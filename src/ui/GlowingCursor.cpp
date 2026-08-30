#include "GlowingCursor.h"

#include <cmath>
#include <cstdint>

#include <SFML/Graphics/BlendMode.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Texture.hpp>

namespace
{
	// On-screen size of the cursor, in the 1920x1080 virtual space. Fixed
	// regardless of the source image's pixel dimensions.
	constexpr float DisplaySize = 72.f;

	// Where the click point sits inside the image, as a 0..1 fraction --
	// top-left for a classic pointer.
	constexpr sf::Vector2f HotspotFraction{ 0.f, 0.f };

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
		const sf::Vector2f textureSize(texture.getSize());
		if (textureSize.x > 0.f && textureSize.y > 0.f)
		{
			sprite.setOrigin({ textureSize.x * HotspotFraction.x, textureSize.y * HotspotFraction.y });
			sprite.setScale({ DisplaySize / textureSize.x, DisplaySize / textureSize.y });
		}
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
