#pragma once

#include <SFML/Graphics/Color.hpp>
#include <SFML/System/String.hpp>
#include <SFML/System/Vector2.hpp>

#include "MenuLabel.h"
#include "../rendering/NeonGlow.h"

namespace sf
{
	class Font;
	class RenderTarget;
	class Shader;
}

namespace UI
{
	// The window header a menu sub-screen sits under. It is the same object the
	// activated menu entry appears to become: RiseFrom() starts it at that
	// entry's place and size and floats it up to the header slot, growing;
	// SinkTo() drops it back toward a menu entry and fades it out. It carries the
	// carousel entry's full look through a shared MenuLabel plus a neon bloom.
	class MenuHeader
	{
	public:
		MenuHeader(const sf::Font& font, sf::Shader& dilateShader, sf::Shader& blurShader);

		void RiseFrom(sf::Vector2f fromCentre, float fromHeight, const sf::String& label, sf::Color colour);
		void SinkTo(sf::Vector2f toCentre, float toHeight);

		void Update(float deltaTime);
		void Render(sf::RenderTarget& target) const;

		[[nodiscard]] bool IsIdle() const;      // hidden, nothing animating
		[[nodiscard]] bool IsSettled() const;   // fully in the header slot

	private:
		enum class Mode { Hidden, Rising, Shown, Sinking };

		struct Pose { sf::Vector2f position; float scale = 1.f; float alpha = 1.f; };
		[[nodiscard]] Pose CurrentPose() const;

		MenuLabel label;
		mutable NeonGlow glow;

		sf::Color colour{ sf::Color::White };

		Mode mode = Mode::Hidden;
		float timer = 0.f;
		float animTime = 0.f;   // drives the glow breath

		sf::Vector2f fromPosition;
		sf::Vector2f toPosition;
		float fromScale = 1.f;
		float toScale = 1.f;
	};
}
