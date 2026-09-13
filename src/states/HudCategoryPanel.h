#pragma once

#include <array>
#include <cstddef>

#include <SFML/Graphics/Color.hpp>

#include "SettingsCategoryPanel.h"

struct Context;

// HUD settings: one on/off toggle per in-game panel (Hold, Next, Score, Lines,
// Level, Time) plus the controls legend, so the player can strip the screen
// down to just the well.
class HudCategoryPanel final : public SettingsCategoryPanel
{
public:
	HudCategoryPanel(Context& context, sf::Color accent);

	static constexpr std::size_t ToggleCount = 7;

protected:
	void BuildRows() override;
	[[nodiscard]] bool SettingsEqual(const GameSettings& a, const GameSettings& b) const override;
	[[nodiscard]] GameSettings DefaultSettings() const override;
	void ApplyWorking() override;
	void ResetWorking() override;

	void AdjustRow(std::size_t index, int direction) override;
	void ActivateRow(std::size_t index) override;
	void RowClicked(std::size_t index, int direction) override;

private:
	void SyncRows();

	std::array<UI::ToggleRow*, ToggleCount> toggleRows{};
};
