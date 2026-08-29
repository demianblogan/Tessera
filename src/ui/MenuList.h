#pragma once

#include <cstddef>
#include <functional>
#include <vector>

#include <SFML/System/Vector2.hpp>

namespace UI
{
	class Button;

	// Keyboard-driven selection over a set of Buttons that are owned elsewhere
	// (usually by a Layout). Previous/Next wrap around and skip disabled
	// buttons. Two callbacks let the host react: one when the selection
	// actually moves, one when the selected button is activated.
	//
	// SelectAt() exists for future mouse support; nothing calls it yet.
	class MenuList
	{
	public:
		void AddButton(Button& button);

		void SelectPrevious();
		void SelectNext();
		void Select(std::size_t index, bool notify = true);

		// Mouse: hover to select, click to select-and-activate. `point` is in
		// the same space the buttons were arranged in.
		void SelectAt(sf::Vector2f point);
		void PointerPressed(sf::Vector2f point);

		void Activate();

		[[nodiscard]] std::size_t GetSelectedIndex() const noexcept;
		[[nodiscard]] std::size_t GetButtonCount() const noexcept;

		// Fired once when the selection moves to a different button.
		std::function<void()> onSelectionChanged;

		// Fired by Activate() with the selected index, when that button is enabled.
		std::function<void(std::size_t index)> onActivate;

	private:
		void RefreshSelectionVisuals() const;

		std::vector<Button*> buttons;
		std::size_t selectedIndex = 0;
	};
}
