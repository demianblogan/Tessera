#pragma once

#include <vector>

#include "../core/Context.h"
#include "../core/State.h"
#include "../ui/Button.h"
#include "../ui/Label.h"
#include "../ui/Layout.h"
#include "../ui/MenuList.h"
#include "../ui/Spacer.h"
#include "../rendering/NeonGlow.h"

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

	Context& context;

	NeonGlow neonGlow;

	UI::Layout rootLayout;
	UI::Layout* menuLayout = nullptr;
	UI::MenuList menuList;
	std::vector<MenuAction> actions;

	sf::Sprite backgroundSprite;
	sf::Sprite titleBackgroundSprite;

	void CreateMenuButton(const sf::String& text, MenuAction action);
	void PerformAction(MenuAction action);
	void UpdateLayout();

public:
	explicit MainMenuState(Context& context);

	[[nodiscard]] StateId GetId() const override { return StateId::MainMenu; }

	void HandleEvent(const sf::Event& event) override;
	void Update(float deltaTime) override;
	void Render(sf::RenderTarget& target) override;
};
