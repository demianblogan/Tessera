#include "HUDCategoryPanel.h"

#include <array>
#include <string_view>

#include "../../core/Context.h"
#include "../../localization/LocalizationManager.h"
#include "../../localization/TextKeys.h"
#include "../../resources/Assets.h"
#include "../../settings/SettingsManager.h"

namespace
{
	// Top sits 16px lower than the header's own maths would suggest: a language
	// whose "OPTIONS" translation has a tall diacritic (Russian's "Й") pushes
	// the header's visual baseline down a little, and this is the panel
	// closest to it.
	constexpr sf::FloatRect PanelBounds{ { 680.f, 216.f }, { 1120.f, 720.f } };
	constexpr float RowsTop = PanelBounds.position.y + 88.f;
	constexpr float RowMargin = 88.f;
	constexpr float RowHeight = 58.f;
	constexpr float RowGap = 10.f;

	// Each toggle: its label key and the GameSettings flag it drives, in the
	// order they appear in the panel.
	struct Toggle
	{
		std::string_view key;
		bool GameSettings::* field;
	};

	constexpr std::array<Toggle, HUDCategoryPanel::ToggleCount> Toggles = { {
		{ TextKey::Options::HUDHold,           &GameSettings::isHoldTetrominoPanelVisible },
		{ TextKey::Options::HUDNext,           &GameSettings::isNextTetrominoPanelVisible },
		{ TextKey::Options::HUDScore,          &GameSettings::isScorePanelVisible },
		{ TextKey::Options::HUDLines,          &GameSettings::isLinesPanelVisible },
		{ TextKey::Options::HUDLevel,          &GameSettings::isLevelPanelVisible },
		{ TextKey::Options::HUDTime,           &GameSettings::isTimePanelVisible },
		{ TextKey::Options::HUDControlsLegend, &GameSettings::isControlsLegendPanelVisible },
	} };
}

HUDCategoryPanel::HUDCategoryPanel(Context& context, sf::Color accent)
	: SettingsCategoryPanel(context, accent, PanelBounds, context.textures.Get(Assets::TextureID::UiFrameWhiteRed))
{
	BuildRows();
}

bool HUDCategoryPanel::IsSettingsEqual(const GameSettings& current, const GameSettings& saved) const
{
	for (const Toggle& toggle : Toggles)
	{
		if (current.*toggle.field != saved.*toggle.field)
		{
			return false;
		}
	}
	return true;
}

GameSettings HUDCategoryPanel::GetDefaultSettings() const
{
	return GameSettings{};
}

void HUDCategoryPanel::BuildRows()
{
	const LocalizationManager& text = context.localization;
	const sf::Font& font = context.fonts.Get(Assets::FontID::Main);
	const sf::Texture& checkbox = context.textures.Get(Assets::TextureID::Checkbox);

	rows.clear();

	for (std::size_t i = 0; i < Toggles.size(); ++i)
	{
		bool GameSettings::* field = Toggles[i].field;
		auto row = std::make_unique<UI::ToggleRow>(font, text.GetText(Toggles[i].key),
			checkbox, working.*field, [this, field](bool isOn) { working.*field = isOn; });
		toggleRows[i] = row.get();
		rows.push_back(std::move(row));
	}

	LayOutRows(RowsTop, RowMargin, RowHeight, RowGap);
	selectedRow = 0;
}

void HUDCategoryPanel::SyncRows()
{
	for (std::size_t i = 0; i < Toggles.size(); ++i)
	{
		toggleRows[i]->SetOn(working.*Toggles[i].field);
	}
}

void HUDCategoryPanel::ApplyWorking()
{
	GameSettings& saved = context.settings.GetSettings();
	for (const Toggle& toggle : Toggles)
	{
		saved.*toggle.field = working.*toggle.field;
	}

	// GameplayState re-reads these in OnResume(), so a change made from the
	// pause screen shows the moment play resumes.
	context.settings.Save();
}

void HUDCategoryPanel::ResetWorking()
{
	const GameSettings defaults;
	for (const Toggle& toggle : Toggles)
	{
		working.*toggle.field = defaults.*toggle.field;
	}
	SyncRows();
}

void HUDCategoryPanel::AdjustRow(std::size_t index, int direction)
{
	if (index < rows.size()) { AdjustRowByType(*rows[index], direction); }
}

void HUDCategoryPanel::ActivateRow(std::size_t index)
{
	if (index < rows.size()) { ActivateRowByType(*rows[index]); }
}

void HUDCategoryPanel::RowClicked(std::size_t index, int direction)
{
	if (index < rows.size()) { RowClickedByType(*rows[index], direction); }
}
