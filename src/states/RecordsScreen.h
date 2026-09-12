#pragma once

#include <optional>
#include <vector>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/System/Vector2.hpp>

#include "../rendering/NeonGlow.h"
#include "../ui/ConfirmDialog.h"
#include "../ui/MenuLabel.h"
#include "../ui/NineSliceFrame.h"
#include "MenuScreen.h"

namespace sf
{
	class Event;
	class RenderTarget;
}

// The Records sub-screen: a purple 9-slice framed leaderboard table (rank,
// name, score, lines, level in aligned columns) with two text buttons below
// the frame -- Reset (raises a confirm dialog that wipes the board) and Back.
// The "RECORDS" header is the shell's, morphed from the menu entry.
class RecordsScreen final : public MenuScreen
{
public:
	RecordsScreen(ScreenHost& host, sf::Color accent);

	void HandleEvent(const sf::Event& event) override;
	void Update(float deltaTime) override;
	void Render(sf::RenderTarget& target) override;

	void PlayIntro() override;
	void StartExit() override;
	[[nodiscard]] bool ExitFinished() const override;

	[[nodiscard]] std::optional<sf::Color> LightbarColour() const override { return accent; }

private:
	enum class Focus { Reset, Back };

	struct Cell
	{
		sf::Text text;
		sf::Color base;
	};

	void BuildHeader();
	void RefreshRows();
	void Activate();
	void Leave();
	void DoReset();

	[[nodiscard]] float PanelAlpha() const;
	void DrawButton(sf::RenderTarget& target, UI::MenuLabel& label, sf::Vector2f centre,
		sf::Color hue, bool selected, float alpha);

	sf::Color accent;
	UI::NineSliceFrame panel;

	std::vector<Cell> headerCells;
	sf::RectangleShape rule;
	std::vector<Cell> rowCells;

	UI::MenuLabel resetLabel;
	UI::MenuLabel backLabel;
	sf::Vector2f resetCentre;
	sf::Vector2f backCentre;
	NeonGlow buttonGlow;
	Focus focus = Focus::Back;

	UI::ConfirmDialog dialog;

	float introTime = 0.f;
	float exitTime = -1.f;      // >= 0 once leaving
	float pressTime = 1000.f;   // since a button was activated
	bool leaving = false;
};
