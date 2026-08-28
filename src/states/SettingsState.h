#pragma once

#include <vector>

#include "../core/Context.h"
#include "../core/State.h"
#include "../settings/GameSettings.h"
#include "../ui/Button.h"
#include "../ui/Label.h"
#include "../ui/Layout.h"
#include "../ui/Slider.h"
#include "../ui/Spacer.h"

class SettingsState final : public State
{
private:
	struct SliderSetting
	{
		UI::Label* nameLabel = nullptr;
		UI::Slider* slider = nullptr;
		UI::Label* valueLabel = nullptr;
	};

	enum class SelectableType
	{
		Button,
		Slider
	};

	struct SelectableElement
	{
		SelectableType type;
		UI::Button* button = nullptr;
		UI::Slider* slider = nullptr;
	};

private:
	Context& context;

	UI::Layout rootLayout;
	std::vector<SelectableElement>	selectableElements;
	UI::Button* verticalSyncButton = nullptr;
	UI::Button* blockStyleButton = nullptr;

	sf::Sprite backgroundSprite;

	SliderSetting frameRateSetting;
	SliderSetting soundSetting;
	SliderSetting musicSetting;

	int selectedIndex = 0;

	void CreateGraphicsSection(UI::Layout& parent);
	void CreateAudioSection(UI::Layout& parent);
	void CreateSliderRow(UI::Layout& parent, const sf::String& text, float minimum, float maximum, float value, float step, SliderSetting& setting);

	void UpdateSelection();
	void UpdateSliderLabels();

	void SelectPrevious();
	void SelectNext();

	void IncreaseCurrentSlider();
	void DecreaseCurrentSlider();
	void ActivateCurrentElement();

	void ApplyAndSaveSettings();

	void UpdateLayout();

public:
	explicit SettingsState(Context& context);

	[[nodiscard]] StateId GetId() const override { return StateId::Settings; }

	void HandleEvent(const sf::Event& event) override;
	void Update(float deltaTime) override;
	void Render(sf::RenderTarget& target) override;
};