#pragma once

#include <cstddef>
#include <optional>
#include <string_view>
#include <utility>

#include <SFML/Graphics/Color.hpp>

#include "../ui/MenuButtonColumn.h"
#include "MenuScreen.h"

namespace sf
{
	class Event;
	class RenderTarget;
}

// The screen "Start Game" opens. A left column with two entries -- Campaign and
// Other Modes -- each of which is its own sub-list, exactly like Options ->
// Controls: hovering shows a dimmed flyout of the sub-list to the right,
// activating slides the main column off left while the sub-list slides into its
// place.
//
//   Campaign     -> Start New Campaign / Continue Campaign / Select Level / Back
//   Other Modes  -> Marathon / Sprint / Ultra / Zen / Player vs Player / Back
//
// Only Marathon is live in v1.3.0; everything else is a disabled stub for the
// version that builds it (see docs/CAMPAIGN_AND_MODES.md). The "GAME MODES"
// header above it is the shell's, morphed from the ring entry.
class ModeSelectScreen final : public MenuScreen
{
public:
	ModeSelectScreen(MenuShell& shell, sf::Color accent);

	void HandleEvent(const sf::Event& event) override;
	void Update(float deltaTime) override;
	void Render(sf::RenderTarget& target) override;

	void PlayIntro() override;
	void StartExit() override;
	[[nodiscard]] bool ExitFinished() const override;

	[[nodiscard]] std::optional<sf::Color> LightbarColour() const override;

private:
	// The main column, or one of the sub-lists, and the slide between them.
	enum class Page { Main, ToSub, Sub, ToMain };

	void Leave();
	void StartMarathon();
	void OpenSub(std::size_t mainRow);
	void CloseSub();

	[[nodiscard]] UI::MenuButtonColumn& SubColumnFor(std::size_t mainRow);
	[[nodiscard]] UI::MenuButtonColumn& ActiveSubColumn() { return SubColumnFor(subRow); }

	[[nodiscard]] std::pair<std::string_view, sf::Color> CurrentLightbar() const;

	void ApplyColumnShifts();

	sf::Color accent;
	UI::MenuButtonColumn column;
	UI::MenuButtonColumn campaignColumn;
	UI::MenuButtonColumn otherModesColumn;

	Page page = Page::Main;
	std::size_t subRow = 0;   // which main row (Campaign / Other Modes) the sub-page belongs to
	float pageT = 0.f;        // 0..1 progress through a slide

	bool leaving = false;
};
