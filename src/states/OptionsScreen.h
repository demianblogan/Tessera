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
// Phase 1 of the content: Graphics and Audio open placeholder panels; Gameplay,
// Controls and Language are disabled.
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

	void Leave();
	void OpenCategory(std::size_t index);
	void CloseCategory();

	sf::Color accent;
	UI::MenuButtonColumn column;

	std::array<std::unique_ptr<OptionsCategoryPanel>, RowCount> panels{};
	std::optional<std::size_t> openIndex;
	std::size_t previewIndex = 0;
	float previewFade = 0.f;

	bool leaving = false;
};
