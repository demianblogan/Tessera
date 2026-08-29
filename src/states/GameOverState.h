#pragma once

#include <cstddef>

#include <SFML/System/String.hpp>

#include "MenuScreenState.h"

namespace UI
{
	class Button;
	class Label;
}

class GameOverState final : public MenuScreenState
{
public:
	GameOverState(Context& context, int finalScore);

	void Render(sf::RenderTarget& target) override;

private:
	static constexpr std::size_t MaxNameLength = 20;

	UI::Button* saveButton = nullptr;
	UI::Label* playerNameLabel = nullptr;

	int finalScore = 0;
	bool isHighScore = false;
	sf::String playerName;

	void SaveRecordAndLeave();
	void HandleTextInput(char32_t character);
	void RefreshSaveButton();

	[[nodiscard]] sf::String TrimPlayerName(const sf::String& string) const;
	[[nodiscard]] bool IsPlayerNameValid() const;

	void OnBack() override;
	[[nodiscard]] bool HandleExtraEvent(const sf::Event& event) override;
};
