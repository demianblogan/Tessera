#include "AudioCategoryPanel.h"

#include "../core/Context.h"
#include "../localization/LocalizationManager.h"
#include "../localization/TextKeys.h"
#include "../resources/Assets.h"
#include "../settings/SettingsManager.h"

namespace
{
	constexpr sf::FloatRect PanelBounds{ { 700.f, 300.f }, { 1060.f, 452.f } };
	constexpr float RowsTop = PanelBounds.position.y + 64.f;
	constexpr float RowMargin = 82.f;
	constexpr float RowHeight = 96.f;
	constexpr float RowGap = 18.f;

	constexpr int VolumeSteps = 10;   // 0..100% in 10% steps
}

AudioCategoryPanel::AudioCategoryPanel(Context& context, sf::Color accent)
	: SettingsCategoryPanel(context, accent, PanelBounds)
{
	BuildRows();
}

bool AudioCategoryPanel::SettingsEqual(const GameSettings& a, const GameSettings& b) const
{
	return a.soundVolume == b.soundVolume && a.musicVolume == b.musicVolume;
}

GameSettings AudioCategoryPanel::DefaultSettings() const
{
	return GameSettings{};
}

void AudioCategoryPanel::BuildRows()
{
	const LocalizationManager& text = context.localization;
	const sf::Font& font = context.fonts.Get(Assets::FontID::Main);
	const sf::Texture& arrow = context.textures.Get(Assets::TextureID::CarouselArrow);

	rows.clear();

	auto soundRow = std::make_unique<UI::SliderRow>(font, text.GetText(TextKey::Options::Sound), arrow,
		VolumeSteps, static_cast<int>(working.soundVolume),
		[this](int value) { working.soundVolume = static_cast<unsigned int>(value); });
	soundRowPtr = soundRow.get();
	rows.push_back(std::move(soundRow));

	auto musicRow = std::make_unique<UI::SliderRow>(font, text.GetText(TextKey::Options::Music), arrow,
		VolumeSteps, static_cast<int>(working.musicVolume),
		[this](int value) { working.musicVolume = static_cast<unsigned int>(value); });
	musicRowPtr = musicRow.get();
	rows.push_back(std::move(musicRow));

	LayOutRows(RowsTop, RowMargin, RowHeight, RowGap);
	selectedRow = 0;
}

void AudioCategoryPanel::SyncRows()
{
	soundRowPtr->SetCurrent(static_cast<int>(working.soundVolume));
	musicRowPtr->SetCurrent(static_cast<int>(working.musicVolume));
}

void AudioCategoryPanel::ApplyWorking()
{
	GameSettings& saved = context.settings.GetSettings();
	saved.soundVolume = working.soundVolume;
	saved.musicVolume = working.musicVolume;

	context.settings.Apply(context);
	context.settings.Save();
}

void AudioCategoryPanel::ResetWorking()
{
	const GameSettings defaults;
	working.soundVolume = defaults.soundVolume;
	working.musicVolume = defaults.musicVolume;
	SyncRows();
}
