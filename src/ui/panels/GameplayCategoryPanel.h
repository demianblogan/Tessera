#pragma once

#include <cstddef>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Text.hpp>

#include "SettingsCategoryPanel.h"

namespace sf
{
	class RenderTarget;
}

struct Context;

// Gameplay settings: feedback (gamepad vibration, gamepad lightbar, screen
// shake) and rules (ghost piece, hold, next-queue length, randomiser). The
// feedback toggles apply at once; the rules rows are picked up by the next
// game that starts (GameplayState reads them once, at construction).
class GameplayCategoryPanel final : public SettingsCategoryPanel
{
public:
	GameplayCategoryPanel(Context& context, sf::Color accent);

protected:
	void BuildRows() override;
	[[nodiscard]] bool IsSettingsEqual(const GameSettings& current, const GameSettings& saved) const override;
	[[nodiscard]] GameSettings GetDefaultSettings() const override;
	void ApplyWorking() override;
	void ResetWorking() override;

	void AdjustRow(std::size_t index, int direction) override;
	void ActivateRow(std::size_t index) override;
	void RowClicked(std::size_t index, int direction) override;
	void RenderExtra(sf::RenderTarget& target, float alpha) override;
	void RefreshText() override;

private:
	void SyncRows();
	[[nodiscard]] std::size_t GetNextLengthIndex(unsigned int length) const;

	UI::ToggleRow* vibrationRowPtr = nullptr;
	UI::ToggleRow* lightbarRowPtr = nullptr;
	UI::ToggleRow* shakeRowPtr = nullptr;
	UI::ToggleRow* ghostRowPtr = nullptr;
	UI::ToggleRow* holdRowPtr = nullptr;
	UI::CarouselRow* nextLengthRowPtr = nullptr;
	UI::CarouselRow* randomiserRowPtr = nullptr;

	// "Takes effect next game" -- shown under the two rows that only affect a
	// GameplaySession that's already running (unlike everything else on this
	// panel, which applies the moment play resumes).
	sf::Text restartNote;
};
