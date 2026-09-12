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
	[[nodiscard]] bool SettingsEqual(const GameSettings& a, const GameSettings& b) const override;
	[[nodiscard]] GameSettings DefaultSettings() const override;
	void ApplyWorking() override;
	void ResetWorking() override;

	void AdjustRow(std::size_t index, int direction) override;
	void ActivateRow(std::size_t index) override;
	void RowClicked(std::size_t index) override;
	void RenderExtra(sf::RenderTarget& target, float alpha) override;
	void RefreshText() override;

private:
	// Rows 0-4 are toggles (Feedback, then Rules); rows 5-6 are carousels.
	static constexpr std::size_t FirstCarouselRow = 5;

	void SyncRows();
	[[nodiscard]] std::size_t NextLengthIndexFor(unsigned int length) const;

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
