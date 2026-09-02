#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <optional>
#include <string_view>
#include <utility>

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
// Gameplay, Graphics and Audio open settings panels in place. Controls and
// Language are each their own sub-page: hovering one shows a dimmed flyout of
// its list to the right, and activating it slides the category column off to
// the left while that list slides into its place.
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

	[[nodiscard]] std::optional<sf::Color> LightbarColour() const override;

private:
	static constexpr std::size_t RowCount = 6;

	// The category column, or one of the sub-lists, and the slide between them.
	enum class Page { Categories, ToSub, Sub, ToCategories };

	void Leave();
	void OpenCategory(std::size_t index);
	void CloseCategory();
	void OpenSub(std::size_t categoryRow);   // categoryRow == Controls or Language
	void CloseSub();
	void OpenControlsItem(std::size_t item);
	void CloseControlsItem();

	[[nodiscard]] UI::MenuButtonColumn& SubColumnFor(std::size_t categoryRow);
	[[nodiscard]] UI::MenuButtonColumn& ActiveSubColumn() { return SubColumnFor(subRow); }

	// The haptics.json key and on-screen fallback colour for whatever is focused.
	[[nodiscard]] std::pair<std::string_view, sf::Color> CurrentLightbar() const;

	void ApplyColumnShifts();

	sf::Color accent;
	UI::MenuButtonColumn column;
	UI::MenuButtonColumn controlsColumn;
	UI::MenuButtonColumn languageColumn;

	std::array<std::unique_ptr<OptionsCategoryPanel>, RowCount> panels{};
	std::array<std::unique_ptr<OptionsCategoryPanel>, 2> controlsPanels{};   // 0 = Keyboard, 1 = Gamepad
	std::optional<std::size_t> openIndex;
	std::optional<std::size_t> openControlsItem;
	std::size_t previewIndex = 0;
	float previewFade = 0.f;

	Page page = Page::Categories;
	std::size_t subRow = 0;   // which category row (Controls / Language) the sub-page belongs to
	float pageT = 0.f;        // 0..1 progress through a slide

	bool leaving = false;
};
