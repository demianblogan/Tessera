#pragma once

#include <optional>

#include <SFML/Graphics/Text.hpp>
#include <SFML/System/String.hpp>

#include "../input/MenuInput.h"

namespace sf
{
	class Font;
	class RenderTarget;
}

namespace UI
{
	// A modal yes / no dialog: dims the screen and shows a centred box with a
	// message and two framed text buttons. Drive it with Navigate(); poll
	// TakeResult() for the answer (nullopt while still open).
	class ConfirmDialog
	{
	public:
		explicit ConfirmDialog(const sf::Font& font);

		void Show(const sf::String& message, const sf::String& yesLabel, const sf::String& noLabel);

		[[nodiscard]] bool IsOpen() const { return open; }
		[[nodiscard]] std::optional<bool> TakeResult();

		void Navigate(MenuInput::Action action);

		void Update(float deltaTime);
		void Render(sf::RenderTarget& target) const;

	private:
		void Resolve(bool answer);
		void DrawButton(sf::RenderTarget& target, sf::Text& text, sf::Vector2f centre, bool chosen) const;

		mutable sf::Text messageText;
		mutable sf::Text yesText;
		mutable sf::Text noText;

		bool open = false;
		bool yesSelected = false;
		std::optional<bool> result;
		float appear = 0.f;
	};
}
