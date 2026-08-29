#pragma once

#include <cstddef>
#include <functional>
#include <vector>

#include <SFML/System/Vector2.hpp>

namespace UI
{
	class Button;
	class Slider;
	class Label;
}

// The focusable rows on the Settings screen: toggle buttons and labelled
// sliders. Owns the current selection, paints the yellow highlight, and turns
// keyboard / gamepad / mouse input into callbacks.
//
// The plain menus use UI::MenuList instead; the two selection models are meant
// to be unified when the menu system is reworked (Phase 2).
class SettingsRowList
{
public:
	void AddButton(UI::Button& button);
	void AddSlider(UI::Slider& slider, UI::Label& nameLabel, UI::Label& valueLabel);

	void SelectPrevious();
	void SelectNext();

	// Hover: select the row under `point` if there is one.
	void SelectAt(sf::Vector2f point);
	// Click: select and act on the row under `point`. Returns whether a row was hit.
	bool PressAt(sf::Vector2f point);

	// Left / right on the selected row (only sliders react).
	void AdjustCurrent(int direction);
	// Confirm on the selected row (only buttons react).
	void ActivateCurrent();

	// The selected slider, or nullptr when a button is selected -- for click-drag.
	[[nodiscard]] UI::Slider* CurrentSlider() const;
	void DragCurrentTo(sf::Vector2f point);

	// Fired with the row that changed / was pressed.
	std::function<void(UI::Button&)> onButtonActivated;
	std::function<void(UI::Slider&)> onSliderAdjusted;
	// Fired once when the selection moves to a different row.
	std::function<void()> onSelectionChanged;

private:
	struct Row
	{
		UI::Button* button = nullptr;
		UI::Slider* slider = nullptr;
		UI::Label* nameLabel = nullptr;   // slider rows only
		UI::Label* valueLabel = nullptr;  // slider rows only
	};

	void Select(std::size_t index);
	void RefreshHighlight() const;

	std::vector<Row> rows;
	std::size_t selectedIndex = 0;
};
