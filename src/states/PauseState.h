#pragma once

#include <vector>

#include "../core/Context.h"
#include "../core/State.h"
#include "../ui/Button.h"
#include "../ui/Layout.h"
#include "../ui/Label.h"

class PauseState final : public State
{
private:
	enum class MenuAction
	{
		ResumeGame,
		RestartGame,
		MainMenu
	};

	struct MenuButton
	{
		UI::Button* button = nullptr;
		MenuAction action;
	};

private:
	Context& context;
	UI::Layout rootLayout;
	UI::Layout* menuLayout = nullptr;
	std::vector<MenuButton> buttons;
	int selectedIndex = 0;

private:
	void CreateMenuButton(const sf::String& text, MenuAction action);

	void SelectPreviousMenuItem();
	void SelectNextMenuItem();

	void UpdateSelection();
	void UpdateLayout();
	void ActivateSelectedButton();

public:
	explicit PauseState(Context& context);

	[[nodiscard]] StateId GetId() const override { return StateId::Pause; }

	void ProcessEvents(sf::RenderWindow& window) override;
	void Update(float deltaTime) override;
	void Render(sf::RenderTarget& target) override;

	[[nodiscard]] bool IsTransparent() const override;
};