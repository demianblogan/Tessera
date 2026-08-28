#pragma once

#include <vector>

#include "../core/Context.h"
#include "../core/State.h"
#include "../ui/Button.h"
#include "../ui/Label.h"
#include "../ui/Layout.h"
#include "../ui/MenuList.h"
#include "../ui/Spacer.h"

class PauseState final : public State
{
private:
	enum class MenuAction
	{
		ResumeGame,
		RestartGame,
		MainMenu
	};

	Context& context;

	UI::Layout rootLayout;
	UI::Layout* menuLayout = nullptr;
	UI::MenuList menuList;
	std::vector<MenuAction> actions;

	void CreateMenuButton(const sf::String& text, MenuAction action);
	void PerformAction(MenuAction action);
	void UpdateLayout();

public:
	explicit PauseState(Context& context);

	[[nodiscard]] StateId GetId() const override { return StateId::Pause; }

	void HandleEvent(const sf::Event& event) override;
	void Update(float deltaTime) override;
	void Render(sf::RenderTarget& target) override;

	[[nodiscard]] bool IsTransparent() const override;
};
