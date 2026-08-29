#include "MenuList.h"

#include "Button.h"

namespace UI
{
	void MenuList::AddButton(Button& button)
	{
		buttons.push_back(&button);
		RefreshSelectionVisuals();
	}

	void MenuList::Select(std::size_t index, bool notify)
	{
		if (index >= buttons.size() || !buttons[index]->IsEnabled())
		{
			return;
		}

		const bool selectionMoved = index != selectedIndex;
		selectedIndex = index;
		RefreshSelectionVisuals();

		if (selectionMoved && notify && onSelectionChanged)
		{
			onSelectionChanged();
		}
	}

	void MenuList::SelectPrevious()
	{
		if (buttons.empty())
		{
			return;
		}

		std::size_t index = selectedIndex;
		do
		{
			index = index == 0 ? buttons.size() - 1 : index - 1;
		}
		while (!buttons[index]->IsEnabled() && index != selectedIndex);

		Select(index);
	}

	void MenuList::SelectNext()
	{
		if (buttons.empty())
		{
			return;
		}

		std::size_t index = selectedIndex;
		do
		{
			index = (index + 1) % buttons.size();
		}
		while (!buttons[index]->IsEnabled() && index != selectedIndex);

		Select(index);
	}

	void MenuList::SelectAt(sf::Vector2f point)
	{
		for (std::size_t index = 0; index < buttons.size(); index++)
		{
			if (buttons[index]->IsEnabled() && buttons[index]->Contains(point))
			{
				Select(index);
				return;
			}
		}
	}

	void MenuList::PointerPressed(sf::Vector2f point)
	{
		for (std::size_t index = 0; index < buttons.size(); index++)
		{
			if (buttons[index]->IsEnabled() && buttons[index]->Contains(point))
			{
				Select(index);
				Activate();
				return;
			}
		}
	}

	void MenuList::Activate()
	{
		if (selectedIndex < buttons.size() && buttons[selectedIndex]->IsEnabled() && onActivate)
		{
			onActivate(selectedIndex);
		}
	}

	std::size_t MenuList::GetSelectedIndex() const noexcept
	{
		return selectedIndex;
	}

	std::size_t MenuList::GetButtonCount() const noexcept
	{
		return buttons.size();
	}

	void MenuList::RefreshSelectionVisuals() const
	{
		for (std::size_t index = 0; index < buttons.size(); index++)
		{
			buttons[index]->SetSelected(index == selectedIndex);
		}
	}
}
