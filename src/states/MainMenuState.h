#pragma once

#include <SFML/Graphics/Sprite.hpp>

#include "MenuScreenState.h"

class MainMenuState final : public MenuScreenState
{
public:
	explicit MainMenuState(Context& context);

	void Render(sf::RenderTarget& target) override;

private:
	sf::Sprite backgroundSprite;
	sf::Sprite titleBackgroundSprite;

	void OnBack() override;
};
