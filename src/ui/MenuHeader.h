#pragma once

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/System/String.hpp>
#include <SFML/System/Vector2.hpp>

namespace sf
{
	class Font;
	class RenderTarget;
}

namespace UI
{
	// The window header a menu sub-screen sits under. It is the same object the
	// activated menu entry appears to become: RiseFrom() starts it at that
	// entry's place and size and floats it up to the header slot, growing;
	// SinkTo() drops it back toward a menu entry and fades it out.
	class MenuHeader
	{
	public:
		explicit MenuHeader(const sf::Font& font);

		void RiseFrom(sf::Vector2f fromCentre, float fromHeight, const sf::String& label, sf::Color colour);
		void SinkTo(sf::Vector2f toCentre, float toHeight);

		void Update(float deltaTime);
		void Render(sf::RenderTarget& target) const;

		[[nodiscard]] bool IsIdle() const;      // hidden, nothing animating
		[[nodiscard]] bool IsSettled() const;   // fully in the header slot

	private:
		enum class Mode { Hidden, Rising, Shown, Sinking };

		[[nodiscard]] float NaturalHeight() const;

		mutable sf::Text text;
		sf::Color colour{ sf::Color::White };

		Mode mode = Mode::Hidden;
		float timer = 0.f;

		sf::Vector2f fromPosition;
		sf::Vector2f toPosition;
		float fromScale = 1.f;
		float toScale = 1.f;
	};
}
