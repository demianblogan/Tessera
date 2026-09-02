#include "GameplayCategoryPanel.h"

#include "../audio/AudioPlayer.h"
#include "../core/Context.h"
#include "../localization/LocalizationManager.h"
#include "../localization/TextKeys.h"
#include "../resources/Assets.h"
#include "../settings/SettingsManager.h"
#include "OptionsSfx.h"

namespace
{
	namespace Sfx = OptionsSfx;

	constexpr sf::FloatRect PanelBounds{ { 680.f, 286.f }, { 1120.f, 508.f } };
	constexpr float RowsTop = PanelBounds.position.y + 60.f;
	constexpr float RowMargin = 88.f;
	constexpr float RowHeight = 92.f;
	constexpr float RowGap = 14.f;
}

GameplayCategoryPanel::GameplayCategoryPanel(Context& context, sf::Color accent)
	: SettingsCategoryPanel(context, accent, PanelBounds)
{
	BuildRows();
}

bool GameplayCategoryPanel::SettingsEqual(const GameSettings& a, const GameSettings& b) const
{
	return a.gamepadVibrationEnabled == b.gamepadVibrationEnabled
		&& a.gamepadLightbarEnabled == b.gamepadLightbarEnabled
		&& a.screenShakeEnabled == b.screenShakeEnabled;
}

GameSettings GameplayCategoryPanel::DefaultSettings() const
{
	return GameSettings{};
}

void GameplayCategoryPanel::BuildRows()
{
	const LocalizationManager& text = context.localization;
	const sf::Font& font = context.fonts.Get(Assets::FontID::Main);
	const sf::Texture& checkbox = context.textures.Get(Assets::TextureID::Checkbox);

	rows.clear();

	auto vibrationRow = std::make_unique<UI::ToggleRow>(font, text.GetText(TextKey::Options::GameplayVibration),
		checkbox, working.gamepadVibrationEnabled, [this](bool on) { working.gamepadVibrationEnabled = on; });
	vibrationRowPtr = vibrationRow.get();
	rows.push_back(std::move(vibrationRow));

	auto lightbarRow = std::make_unique<UI::ToggleRow>(font, text.GetText(TextKey::Options::GameplayLightbar),
		checkbox, working.gamepadLightbarEnabled, [this](bool on) { working.gamepadLightbarEnabled = on; });
	lightbarRowPtr = lightbarRow.get();
	rows.push_back(std::move(lightbarRow));

	auto shakeRow = std::make_unique<UI::ToggleRow>(font, text.GetText(TextKey::Options::GameplayShake),
		checkbox, working.screenShakeEnabled, [this](bool on) { working.screenShakeEnabled = on; });
	shakeRowPtr = shakeRow.get();
	rows.push_back(std::move(shakeRow));

	LayOutRows(RowsTop, RowMargin, RowHeight, RowGap);
	selectedRow = 0;
}

void GameplayCategoryPanel::SyncRows()
{
	vibrationRowPtr->SetOn(working.gamepadVibrationEnabled);
	lightbarRowPtr->SetOn(working.gamepadLightbarEnabled);
	shakeRowPtr->SetOn(working.screenShakeEnabled);
}

void GameplayCategoryPanel::ApplyWorking()
{
	GameSettings& saved = context.settings.GetSettings();
	saved.gamepadVibrationEnabled = working.gamepadVibrationEnabled;
	saved.gamepadLightbarEnabled = working.gamepadLightbarEnabled;
	saved.screenShakeEnabled = working.screenShakeEnabled;

	context.settings.Apply(context);   // the gamepad toggles take effect immediately
	context.settings.Save();
}

void GameplayCategoryPanel::ResetWorking()
{
	const GameSettings defaults;
	working.gamepadVibrationEnabled = defaults.gamepadVibrationEnabled;
	working.gamepadLightbarEnabled = defaults.gamepadLightbarEnabled;
	working.screenShakeEnabled = defaults.screenShakeEnabled;
	SyncRows();
}

void GameplayCategoryPanel::AdjustRow(std::size_t index, int direction)
{
	if (index < rows.size())
	{
		rows[index]->Adjust(direction);
		Sfx::Toggle(context.audioPlayer);
	}
}

void GameplayCategoryPanel::ActivateRow(std::size_t index)
{
	if (index < rows.size())
	{
		rows[index]->Activate();
		Sfx::Toggle(context.audioPlayer);
	}
}

void GameplayCategoryPanel::RowClicked(std::size_t /*index*/)
{
	Sfx::Toggle(context.audioPlayer);
}
