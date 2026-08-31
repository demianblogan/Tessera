#pragma once

#include <optional>

#include <SFML/Graphics/Text.hpp>
#include <SFML/System/String.hpp>

#include "MenuLabel.h"
#include "../input/MenuInput.h"
#include "../rendering/NeonGlow.h"

namespace sf
{
	class Font;
	class RenderTarget;
}

namespace UI
{
	// A modal yes / no dialog: dims the screen and shows a centred box with a
	// message and two buttons. Drive it with Navigate(); poll TakeResult() for
	// the answer (nullopt while still open).
	class ConfirmDialog
	{
	public:
		ConfirmDialog(const sf::Font& messageFont, const sf::Font& buttonFont,
			sf::Shader& dilateShader, sf::Shader& blurShader);

		void Show(const sf::String& message, const sf::String& yesLabel, const sf::String& noLabel);

		[[nodiscard]] bool IsOpen() const { return open; }
		[[nodiscard]] std::optional<bool> TakeResult();

		void Navigate(MenuInput::Action action);

		void Update(float deltaTime);
		void Render(sf::RenderTarget& target) const;

	private:
		void Resolve(bool answer);

		mutable NeonGlow glow;

		sf::Text messageText;
		MenuLabel yesButton;
		MenuLabel noButton;

		bool open = false;
		bool yesSelected = false;
		std::optional<bool> result;
		float appear = 0.f;   // 0..1 pop-in
	};
}
