#pragma once

#include <cstddef>
#include <vector>

#include <SFML/System/String.hpp>

#include "../core/Context.h"
#include "../core/State.h"
#include "../ui/Button.h"
#include "../ui/Label.h"
#include "../ui/Layout.h"
#include "../ui/MenuList.h"
#include "../ui/Spacer.h"
#include "../rendering/NeonGlow.h"

class GameOverState final : public State
{
private:
	enum class MenuAction
	{
		SaveRecord,
		RestartGame,
		MainMenu
	};

	static constexpr std::size_t MaxNameLength = 20;

	Context& context;

	NeonGlow neonGlow;

	UI::Layout rootLayout;
	UI::Layout* menuLayout = nullptr;
	UI::MenuList menuList;
	std::vector<MenuAction> actions;

	UI::Button* saveButton = nullptr;
	UI::Label* playerNameLabel = nullptr;

	int finalScore = 0;
	bool isHighScore = false;
	sf::String playerName;

	void CreateMenuButton(const sf::String& text, MenuAction action);
	void PerformAction(MenuAction action);
	void HandleTextInput(char32_t character);
	void RefreshSaveButton();
	void UpdateLayout();

	[[nodiscard]] sf::String TrimPlayerName(const sf::String& string) const;
	[[nodiscard]] bool IsPlayerNameValid() const;

public:
	GameOverState(Context& context, int finalScore);

	void HandleEvent(const sf::Event& event) override;
	void Update(float deltaTime) override;
	void Render(sf::RenderTarget& target) override;
};
