#include "SettingsRowList.h"

#include <SFML/Graphics/Color.hpp>

#include "../ui/Button.h"
#include "../ui/Label.h"
#include "../ui/Slider.h"

void SettingsRowList::AddButton(UI::Button& button)
{
	rows.push_back({ .button = &button });
	RefreshHighlight();
}

void SettingsRowList::AddSlider(UI::Slider& slider, UI::Label& nameLabel, UI::Label& valueLabel)
{
	rows.push_back({ .slider = &slider, .nameLabel = &nameLabel, .valueLabel = &valueLabel });
	RefreshHighlight();
}

void SettingsRowList::Select(std::size_t index)
{
	if (index >= rows.size() || index == selectedIndex)
	{
		return;
	}

	selectedIndex = index;
	RefreshHighlight();

	if (onSelectionChanged)
	{
		onSelectionChanged();
	}
}

void SettingsRowList::RefreshHighlight() const
{
	for (std::size_t index = 0; index < rows.size(); index++)
	{
		const bool selected = index == selectedIndex;
		const Row& row = rows[index];

		if (row.button != nullptr)
		{
			row.button->SetSelected(selected);
		}
		else
		{
			const sf::Color colour = selected ? sf::Color::Yellow : sf::Color::White;
			row.nameLabel->SetFillColor(colour);
			row.valueLabel->SetFillColor(colour);
		}
	}
}

void SettingsRowList::SelectPrevious()
{
	if (!rows.empty())
	{
		Select(selectedIndex == 0 ? rows.size() - 1 : selectedIndex - 1);
	}
}

void SettingsRowList::SelectNext()
{
	if (!rows.empty())
	{
		Select((selectedIndex + 1) % rows.size());
	}
}

void SettingsRowList::SelectAt(sf::Vector2f point)
{
	for (std::size_t index = 0; index < rows.size(); index++)
	{
		const Row& row = rows[index];
		const bool hit = row.button != nullptr ? row.button->Contains(point) : row.slider->Contains(point);

		if (hit)
		{
			Select(index);
			return;
		}
	}
}

bool SettingsRowList::PressAt(sf::Vector2f point)
{
	for (std::size_t index = 0; index < rows.size(); index++)
	{
		const Row& row = rows[index];

		if (row.button != nullptr && row.button->Contains(point))
		{
			Select(index);
			if (onButtonActivated)
			{
				onButtonActivated(*row.button);
			}
			return true;
		}

		if (row.slider != nullptr && row.slider->Contains(point))
		{
			Select(index);
			row.slider->SetValueFromPointer(point);
			if (onSliderAdjusted)
			{
				onSliderAdjusted(*row.slider);
			}
			return true;
		}
	}

	return false;
}

void SettingsRowList::AdjustCurrent(int direction)
{
	if (selectedIndex >= rows.size() || direction == 0)
	{
		return;
	}

	UI::Slider* slider = rows[selectedIndex].slider;

	if (slider == nullptr)
	{
		return;
	}

	if (direction > 0)
	{
		slider->Increase();
	}
	else
	{
		slider->Decrease();
	}

	if (onSliderAdjusted)
	{
		onSliderAdjusted(*slider);
	}
}

void SettingsRowList::ActivateCurrent()
{
	if (selectedIndex >= rows.size())
	{
		return;
	}

	UI::Button* button = rows[selectedIndex].button;

	if (button != nullptr && onButtonActivated)
	{
		onButtonActivated(*button);
	}
}

UI::Slider* SettingsRowList::CurrentSlider() const
{
	return selectedIndex < rows.size() ? rows[selectedIndex].slider : nullptr;
}

void SettingsRowList::DragCurrentTo(sf::Vector2f point)
{
	if (UI::Slider* slider = CurrentSlider())
	{
		slider->SetValueFromPointer(point);

		if (onSliderAdjusted)
		{
			onSliderAdjusted(*slider);
		}
	}
}
