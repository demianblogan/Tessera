#include "MenuFlash.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

#include <SFML/Graphics/BlendMode.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

namespace
{
	constexpr sf::Vector2f VirtualSize{ 1920.f, 1080.f };
	constexpr sf::Color Tint{ 130, 185, 255 };   // cool blue-white
	constexpr float MaxAlpha = 0.12f;
	constexpr float PulseAdd = 0.85f;
	constexpr float Decay = 6.5f;                 // per second, exponential
}

namespace UI
{
	void MenuFlash::Pulse()
	{
		intensity = std::min(1.f, intensity + PulseAdd);
	}

	void MenuFlash::Update(float deltaTime)
	{
		intensity *= std::exp(-Decay * deltaTime);
		if (intensity < 0.003f)
		{
			intensity = 0.f;
		}
	}

	void MenuFlash::Render(sf::RenderTarget& target) const
	{
		if (intensity <= 0.f)
		{
			return;
		}

		sf::RectangleShape quad(VirtualSize);
		quad.setFillColor(sf::Color(Tint.r, Tint.g, Tint.b,
			static_cast<std::uint8_t>(std::clamp(intensity * MaxAlpha, 0.f, 1.f) * 255.f)));

		sf::RenderStates additive;
		additive.blendMode = sf::BlendAdd;
		target.draw(quad, additive);
	}
}
