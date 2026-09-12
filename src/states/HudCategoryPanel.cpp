#include "HudCategoryPanel.h"

#include <array>
#include <string_view>

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

	constexpr sf::FloatRect PanelBounds{ { 680.f, 200.f }, { 1120.f, 720.f } };
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

	constexpr std::array<Toggle, HudCategoryPanel::ToggleCount> Toggles = { {
		{ TextKey::Options::HudHold,           &GameSettings::hudHold },
		{ TextKey::Options::HudNext,           &GameSettings::hudNext },
		{ TextKey::Options::HudScore,          &GameSettings::hudScore },
		{ TextKey::Options::HudLines,          &GameSettings::hudLines },
		{ TextKey::Options::HudLevel,          &GameSettings::hudLevel },
		{ TextKey::Options::HudTime,           &GameSettings::hudTime },
		{ TextKey::Options::HudControlsLegend, &GameSettings::hudControlsLegend },
	} };
}

HudCategoryPanel::HudCategoryPanel(Context& context, sf::Color accent)
	: SettingsCategoryPanel(context, accent, PanelBounds, context.textures.Get(Assets::TextureID::UiFrameWhiteRed))
{
	BuildRows();
}

bool HudCategoryPanel::SettingsEqual(const GameSettings& a, const GameSettings& b) const
{
	for (const Toggle& toggle : Toggles)
	{
		if (a.*toggle.field != b.*toggle.field)
		{
			return false;
		}
	}
	return true;
}

GameSettings HudCategoryPanel::DefaultSettings() const
{
	return GameSettings{};
}

void HudCategoryPanel::BuildRows()
{
	const LocalizationManager& text = context.localization;
	const sf::Font& font = context.fonts.Get(Assets::FontID::Main);
	const sf::Texture& checkbox = context.textures.Get(Assets::TextureID::Checkbox);

	rows.clear();

	for (std::size_t i = 0; i < Toggles.size(); ++i)
	{
		bool GameSettings::* field = Toggles[i].field;
		auto row = std::make_unique<UI::ToggleRow>(font, text.GetText(Toggles[i].key),
			checkbox, working.*field, [this, field](bool on) { working.*field = on; });
		toggleRows[i] = row.get();
		rows.push_back(std::move(row));
	}

	LayOutRows(RowsTop, RowMargin, RowHeight, RowGap);
	selectedRow = 0;
}

void HudCategoryPanel::SyncRows()
{
	for (std::size_t i = 0; i < Toggles.size(); ++i)
	{
		toggleRows[i]->SetOn(working.*Toggles[i].field);
	}
}

void HudCategoryPanel::ApplyWorking()
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

void HudCategoryPanel::ResetWorking()
{
	const GameSettings defaults;
	for (const Toggle& toggle : Toggles)
	{
		working.*toggle.field = defaults.*toggle.field;
	}
	SyncRows();
}

void HudCategoryPanel::AdjustRow(std::size_t index, int direction)
{
	if (index < rows.size())
	{
		rows[index]->Adjust(direction);
		Sfx::Toggle(context.audioPlayer);
	}
}

void HudCategoryPanel::ActivateRow(std::size_t index)
{
	if (index < rows.size())
	{
		rows[index]->Activate();
		Sfx::Toggle(context.audioPlayer);
	}
}

void HudCategoryPanel::RowClicked(std::size_t /*index*/)
{
	Sfx::Toggle(context.audioPlayer);
}
