#pragma once

#include <vector>

#include "../core/Context.h"
#include "../core/State.h"
#include "../ui/Button.h"
#include "../ui/Layout.h"
#include "../ui/Label.h"
#include "../ui/Spacer.h"

class GameOverState final : public State
{
private:
	enum class MenuAction
	{
		SaveRecord,
		RestartGame,
		MainMenu
	};

	struct MenuButton
	{
		UI::Button* button = nullptr;
		MenuAction action;
	};

	static constexpr std::size_t MAX_NAME_LENGTH = 20;

	Context& context;

	UI::Layout rootLayout;
	UI::Layout* menuLayout = nullptr;
	UI::Button* saveButton = nullptr;

	std::vector<MenuButton> buttons;

	UI::Label* scoreLabel = nullptr;
	UI::Label* highScoreLabel = nullptr;
	UI::Label* playerNameLabel = nullptr;

	int selectedIndex = 0;
	int finalScore = 0;
	bool isHighScore = false;
	sf::String playerName;

	void CreateMenuButton(const sf::String& text, MenuAction action);

	void SelectPreviousMenuItem();
	void SelectNextMenuItem();

	void UpdateSelection();
	void UpdateLayout();
	void ActivateSelectedButton();

	void UpdateSaveButtonState();

	[[nodiscard]] sf::String TrimPlayerName(const sf::String& string) const;
	[[nodiscard]] bool IsPlayerNameValid() const;

public:
	GameOverState(Context& context, int finalScore);

	[[nodiscard]] StateId GetId() const override { return StateId::GameOver; }

	void ProcessEvents(sf::RenderWindow& window) override;
	void Update(float deltaTime) override;
	void Render(sf::RenderTarget& target) override;
};