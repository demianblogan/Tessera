#pragma once

#include <optional>

#include <SFML/Graphics/Text.hpp>
#include <SFML/System/String.hpp>

#include "../input/MenuInput.h"

namespace sf
{
	class Font;
	class RenderTarget;
	class Texture;
}

namespace UI
{
	// A modal yes / no dialog: dims the screen and shows a centred box with a
	// message and two buttons -- a tick and a cross, each on a checkbox sprite.
	// Drive it with Navigate(); poll TakeResult() for the answer (nullopt while
	// still open).
	class ConfirmDialog
	{
	public:
		ConfirmDialog(const sf::Font& messageFont, const sf::Texture& checkboxTexture);

		void Show(const sf::String& message);

		[[nodiscard]] bool IsOpen() const { return open; }
		[[nodiscard]] std::optional<bool> TakeResult();

		void Navigate(MenuInput::Action action);

		void Update(float deltaTime);
		void Render(sf::RenderTarget& target) const;

	private:
		void Resolve(bool answer);
		void DrawButton(sf::RenderTarget& target, bool yesSide, bool chosen) const;

		const sf::Texture& checkboxTexture;

		sf::Text messageText;

		bool open = false;
		bool yesSelected = false;
		std::optional<bool> result;
		float appear = 0.f;
	};
}
