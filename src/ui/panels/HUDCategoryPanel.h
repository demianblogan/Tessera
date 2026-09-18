#pragma once

#include <array>
#include <cstddef>

#include <SFML/Graphics/Color.hpp>

#include "SettingsCategoryPanel.h"

struct Context;

// HUD settings: one on/off toggle per in-game panel (Hold, Next, Score, Lines,
// Level, Time) plus the controls legend, so the player can strip the screen
// down to just the well.
class HUDCategoryPanel final : public SettingsCategoryPanel
{
public:
	HUDCategoryPanel(Context& context, sf::Color accent);

	static constexpr std::size_t ToggleCount = 7;

protected:
	void BuildRows() override;
	[[nodiscard]] bool IsSettingsEqual(const GameSettings& current, const GameSettings& saved) const override;
	[[nodiscard]] GameSettings GetDefaultSettings() const override;
	void ApplyWorking() override;
	void ResetWorking() override;

	void AdjustRow(std::size_t index, int direction) override;
	void ActivateRow(std::size_t index) override;
	void RowClicked(std::size_t index, int direction) override;

private:
	void SyncRows();

	std::array<UI::ToggleRow*, ToggleCount> toggleRows{};
};
