#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <optional>

#include <SFML/Graphics/Color.hpp>

#include "../ui/MenuButtonColumn.h"
#include "MenuScreen.h"
#include "OptionsCategoryPanel.h"

namespace sf
{
	class Event;
	class RenderTarget;
}

// The Options sub-screen: a left-aligned column of category buttons that flies
// up from the bottom on entry, with a right-hand panel that previews the
// selected category and, once opened, shows its content while the column
// shrinks and dims around the open entry. The "OPTIONS" header above it is the
// shell's, morphed from the menu entry.
//
// Graphics and Audio open settings panels in place. Controls is its own page:
// hovering it shows a dimmed flyout of Keyboard / Gamepad / Back to its right,
// and activating it slides the category column off to the left while that
// three-button column slides into its place. Gameplay and Language are disabled.
class OptionsScreen final : public MenuScreen
{
public:
	OptionsScreen(MenuShell& shell, sf::Color accent);

	void HandleEvent(const sf::Event& event) override;
	void Update(float deltaTime) override;
	void Render(sf::RenderTarget& target) override;

	void PlayIntro() override;
	void StartExit() override;
	[[nodiscard]] bool ExitFinished() const override;

	[[nodiscard]] std::optional<sf::Color> LightbarColour() const override { return accent; }

private:
	static constexpr std::size_t RowCount = 6;

	// Which column has the focus, and the horizontal slide between them.
	enum class Page { Categories, ToControls, Controls, ToCategories };

	void Leave();
	void OpenCategory(std::size_t index);
	void CloseCategory();
	void OpenControls();
	void CloseControls();
	void OpenControlsItem(std::size_t item);
	void CloseControlsItem();

	void ApplyColumnShifts();
	[[nodiscard]] bool CategoriesInteractive() const { return page == Page::Categories; }

	sf::Color accent;
	UI::MenuButtonColumn column;
	UI::MenuButtonColumn controlsColumn;

	std::array<std::unique_ptr<OptionsCategoryPanel>, RowCount> panels{};
	std::array<std::unique_ptr<OptionsCategoryPanel>, 2> controlsPanels{};   // 0 = Keyboard, 1 = Gamepad
	std::optional<std::size_t> openIndex;
	std::optional<std::size_t> openControlsItem;
	std::size_t previewIndex = 0;
	float previewFade = 0.f;

	Page page = Page::Categories;
	float pageT = 0.f;   // 0..1 progress through a slide

	bool leaving = false;
};
