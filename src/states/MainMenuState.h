#pragma once

#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Text.hpp>

#include "MenuScreenState.h"

class MainMenuState final : public MenuScreenState
{
public:
	explicit MainMenuState(Context& context);

	void Render(sf::RenderTarget& target) override;

private:
	sf::Sprite backgroundSprite;
	sf::Sprite titleBackgroundSprite;
	sf::Text versionText;

	void OnBack() override;
};
