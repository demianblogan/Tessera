#pragma once

#include <cstddef>
#include <vector>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/System/Vector2.hpp>

#include "SettingsCategoryPanel.h"

struct Context;

namespace sf
{
	class RenderTarget;
}

// Graphics settings: screen resolution and window mode (carousels), vertical
// sync / show FPS / CRT filter (toggles). Applying a display-mode change queues
// a window recreation.
class GraphicsCategoryPanel final : public SettingsCategoryPanel
{
public:
	GraphicsCategoryPanel(Context& context, sf::Color accent);

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

private:
	static constexpr std::size_t FirstToggleRow = 2;   // rows 0-1 carousels, 2-4 toggles

	void SyncRows();
	[[nodiscard]] std::size_t ResolutionIndexFor(sf::Vector2u resolution) const;

	std::vector<sf::Vector2u> resolutions;

	UI::CarouselRow* resolutionRowPtr = nullptr;
	UI::CarouselRow* windowModeRowPtr = nullptr;
	UI::ToggleRow* vsyncRowPtr = nullptr;
	UI::ToggleRow* showFpsRowPtr = nullptr;
	UI::ToggleRow* crtRowPtr = nullptr;

	sf::Text borderlessNote;
};
