#pragma once

#include <cstddef>
#include <vector>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/System/Vector2.hpp>

#include "../ui/MenuLabel.h"
#include "../ui/NineSliceFrame.h"
#include "OptionsCategoryPanel.h"

struct Context;

namespace sf
{
	class Event;
	class RenderTarget;
}

// Controls > Gamepad: a read-only reference table. Left column is the same list
// of gameplay actions as the Keyboard screen (plus Pause); the two right columns
// show the fixed Xbox and PlayStation buttons, drawn from the button-prompt
// atlases. Nothing is editable -- rows can be highlighted but not activated --
// so the only real control is a single Back button.
class GamepadCategoryPanel final : public OptionsCategoryPanel
{
public:
	GamepadCategoryPanel(Context& context, sf::Color accent);

	void Open() override;
	void Close() override;
	void SetVisibility(Visibility visibility, float previewFade) override;
	void Update(float deltaTime) override;
	void Render(sf::RenderTarget& target) override;
	bool HandleEvent(const sf::Event& event) override;

	[[nodiscard]] bool WantsToClose() const override { return closeRequested; }

private:
	enum class Focus { Rows, Back };

	struct Row
	{
		sf::Text label;
		sf::IntRect xbox;          // sprite rect in the Xbox atlas
		sf::IntRect playStation;   // sprite rect in the PlayStation atlas
		float highlight = 0.f;
	};

	void LayOut();
	void MoveSelection(int direction);

	Context& context;
	sf::Color accent;
	sf::FloatRect panelBounds;

	UI::NineSliceFrame frame;
	sf::Text xboxHeader;
	sf::Text playStationHeader;
	std::vector<Row> rows;

	UI::MenuLabel backButton;
	sf::Vector2f backCentre;
	sf::FloatRect backBox;

	Focus focus = Focus::Rows;
	std::size_t selectedRow = 0;

	float alpha = 0.f;
	float targetAlpha = 0.f;
	bool active = false;
	bool closeRequested = false;
};
