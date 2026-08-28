#pragma once

#include <memory>
#include <vector>

#include "../core/Context.h"
#include "../core/State.h"
#include "../ui/Button.h"
#include "../ui/Layout.h"
#include "../ui/Label.h"
#include "../ui/Spacer.h"

class MainMenuState final : public State
{
private:
	enum class MenuAction
	{
		StartGame,
		Options,
		Statistics,
		Exit
	};

	struct MenuButton
	{
		UI::Button* button = nullptr;
		MenuAction action;
	};

	Context& context;
	UI::Layout rootLayout;
	UI::Layout* menuLayout = nullptr;
	std::vector<MenuButton> buttons;
	int selectedIndex = 0;

	sf::Sprite backgroundSprite;
	sf::Sprite titleBackgroundSprite;

	void CreateMenuButton(const sf::String& text, MenuAction action);

	void SelectPreviousMenuItem();
	void SelectNextMenuItem();

	void UpdateSelection();
	void ActivateSelectedButton();
	void UpdateLayout();

public:
	explicit MainMenuState(Context& context);

	[[nodiscard]] StateId GetId() const override { return StateId::MainMenu; }

	void ProcessEvents(sf::RenderWindow& window) override;
	void Update(float deltaTime) override;

	void Render(sf::RenderTarget& target) override;
};