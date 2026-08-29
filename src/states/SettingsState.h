#pragma once

#include <SFML/Graphics/Sprite.hpp>
#include <SFML/System/String.hpp>

#include "../core/Context.h"
#include "../core/State.h"
#include "../ui/Layout.h"
#include "SettingsRowList.h"

namespace UI
{
	class Button;
	class Label;
	class Slider;
}

class SettingsState final : public State
{
public:
	explicit SettingsState(Context& context);

	void HandleEvent(const sf::Event& event) override;
	void Update(float deltaTime) override;
	void Render(sf::RenderTarget& target) override;

private:
	// A slider and the two labels the screen keeps in sync with it.
	struct SliderSetting
	{
		UI::Label* nameLabel = nullptr;
		UI::Slider* slider = nullptr;
		UI::Label* valueLabel = nullptr;
	};

	Context& context;

	UI::Layout rootLayout;
	sf::Sprite backgroundSprite;

	SettingsRowList rows;

	UI::Button* verticalSyncButton = nullptr;
	UI::Button* blockStyleButton = nullptr;

	SliderSetting frameRateSetting;
	SliderSetting soundSetting;
	SliderSetting musicSetting;

	void CreateGraphicsSection(UI::Layout& parent);
	void CreateAudioSection(UI::Layout& parent);
	void CreateSliderRow(UI::Layout& parent, const sf::String& text, float minimum, float maximum, float value, float step, SliderSetting& setting);

	void OnButtonActivated(UI::Button& button);
	void ToggleVerticalSync();
	void ToggleBlockStyle();

	[[nodiscard]] sf::String FormatFrameRate(int value) const;
	void UpdateSliderLabels();

	void ApplyAndSaveSettings();

	void HandlePointer(sf::Vector2f point, bool clicked);
	void UpdateLayout();
};
