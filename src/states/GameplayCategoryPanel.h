#pragma once

#include <cstddef>

#include <SFML/Graphics/Color.hpp>

#include "SettingsCategoryPanel.h"

struct Context;

// Gameplay settings: three on/off toggles -- gamepad vibration, gamepad
// lightbar, and screen shake. The two gamepad toggles apply at once; screen
// shake is picked up when the next game starts.
class GameplayCategoryPanel final : public SettingsCategoryPanel
{
public:
	GameplayCategoryPanel(Context& context, sf::Color accent);

protected:
	void BuildRows() override;
	[[nodiscard]] bool SettingsEqual(const GameSettings& a, const GameSettings& b) const override;
	[[nodiscard]] GameSettings DefaultSettings() const override;
	void ApplyWorking() override;
	void ResetWorking() override;

	void AdjustRow(std::size_t index, int direction) override;
	void ActivateRow(std::size_t index) override;
	void RowClicked(std::size_t index) override;

private:
	void SyncRows();

	UI::ToggleRow* vibrationRowPtr = nullptr;
	UI::ToggleRow* lightbarRowPtr = nullptr;
	UI::ToggleRow* shakeRowPtr = nullptr;
};
