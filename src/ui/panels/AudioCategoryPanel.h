#pragma once

#include <cstddef>

#include <SFML/Graphics/Color.hpp>

#include "SettingsCategoryPanel.h"

struct Context;

// Audio settings: two volume sliders (sound and music, 0..100% in 10% steps).
class AudioCategoryPanel final : public SettingsCategoryPanel
{
public:
	AudioCategoryPanel(Context& context, sf::Color accent);

protected:
	void BuildRows() override;
	[[nodiscard]] bool IsSettingsEqual(const GameSettings& current, const GameSettings& saved) const override;
	[[nodiscard]] GameSettings GetDefaultSettings() const override;
	void ApplyWorking() override;
	void ResetWorking() override;

private:
	void SyncRows();

	UI::SliderRow* soundRowPtr = nullptr;
	UI::SliderRow* musicRowPtr = nullptr;
};
