#include "GameplayCategoryPanel.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

#include <SFML/Graphics/RenderTarget.hpp>

#include "../../core/Context.h"
#include "../../localization/LocalizationManager.h"
#include "../../localization/TextKeys.h"
#include "../../resources/Assets.h"
#include "../../settings/GameSettings.h"
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

	constexpr unsigned int RestartNoteSize = 30u;
	const sf::Color WarningYellow{ 255, 245, 140 };
	constexpr float RestartNoteTopOffset = 34.f;
}

GameplayCategoryPanel::GameplayCategoryPanel(Context& context, sf::Color accent)
	: SettingsCategoryPanel(context, accent, PanelBounds, context.textures.Get(Assets::TextureID::UiFrameCyan))
	, restartNote(context.fonts.Get(Assets::FontID::Main),
		context.localization.GetText(TextKey::Options::RestartToApply), RestartNoteSize)
{
	restartNote.setFillColor(WarningYellow);   // a warning, not a label

	const sf::FloatRect noteBounds = restartNote.getLocalBounds();
	restartNote.setOrigin({ noteBounds.position.x + noteBounds.size.x * 0.5f, noteBounds.position.y });
	restartNote.setPosition({ PanelBounds.position.x + PanelBounds.size.x * 0.5f, PanelBounds.position.y + RestartNoteTopOffset });

	BuildRows();
}

bool GameplayCategoryPanel::IsSettingsEqual(const GameSettings& current, const GameSettings& saved) const
{
	return current.isGamepadVibrationEnabled == saved.isGamepadVibrationEnabled
		&& current.isGamepadLightbarEnabled == saved.isGamepadLightbarEnabled
		&& current.isScreenShakeEnabled == saved.isScreenShakeEnabled
		&& current.isGhostPieceEnabled == saved.isGhostPieceEnabled
		&& current.isHoldTetrominoEnabled == saved.isHoldTetrominoEnabled
		&& current.nextQueueLength == saved.nextQueueLength
		&& current.isSevenBagEnabled == saved.isSevenBagEnabled;
}

GameSettings GameplayCategoryPanel::GetDefaultSettings() const
{
	return GameSettings{};
}

std::size_t GameplayCategoryPanel::GetNextLengthIndex(unsigned int length) const
{
	return static_cast<std::size_t>(std::clamp(length, MinNextQueueLength, MaxNextQueueLength) - MinNextQueueLength);
}

void GameplayCategoryPanel::BuildRows()
{
	const LocalizationManager& text = context.localization;
	const sf::Font& font = context.fonts.Get(Assets::FontID::Main);
	const sf::Texture& checkbox = context.textures.Get(Assets::TextureID::Checkbox);
	const sf::Texture& arrow = context.textures.Get(Assets::TextureID::CarouselArrow);

	rows.clear();

	auto vibrationRow = std::make_unique<UI::ToggleRow>(font, text.GetText(TextKey::Options::GameplayVibration),
		checkbox, working.isGamepadVibrationEnabled, [this](bool isOn) { working.isGamepadVibrationEnabled = isOn; });
	vibrationRowPtr = vibrationRow.get();
	rows.push_back(std::move(vibrationRow));

	auto lightbarRow = std::make_unique<UI::ToggleRow>(font, text.GetText(TextKey::Options::GameplayLightbar),
		checkbox, working.isGamepadLightbarEnabled, [this](bool isOn) { working.isGamepadLightbarEnabled = isOn; });
	lightbarRowPtr = lightbarRow.get();
	rows.push_back(std::move(lightbarRow));

	auto shakeRow = std::make_unique<UI::ToggleRow>(font, text.GetText(TextKey::Options::GameplayShake),
		checkbox, working.isScreenShakeEnabled, [this](bool isOn) { working.isScreenShakeEnabled = isOn; });
	shakeRowPtr = shakeRow.get();
	rows.push_back(std::move(shakeRow));

	auto ghostRow = std::make_unique<UI::ToggleRow>(font, text.GetText(TextKey::Options::GameplayGhost),
		checkbox, working.isGhostPieceEnabled, [this](bool isOn) { working.isGhostPieceEnabled = isOn; });
	ghostRowPtr = ghostRow.get();
	rows.push_back(std::move(ghostRow));

	auto holdRow = std::make_unique<UI::ToggleRow>(font, text.GetText(TextKey::Options::GameplayHold),
		checkbox, working.isHoldTetrominoEnabled, [this](bool isOn) { working.isHoldTetrominoEnabled = isOn; });
	holdRowPtr = holdRow.get();
	rows.push_back(std::move(holdRow));

	std::vector<sf::String> lengthOptions;
	for (unsigned int length = MinNextQueueLength; length <= MaxNextQueueLength; ++length)
	{
		lengthOptions.push_back(sf::String(std::to_string(length)));
	}

	auto nextLengthRow = std::make_unique<UI::CarouselRow>(font, text.GetText(TextKey::Options::GameplayNextLength),
		std::move(lengthOptions), GetNextLengthIndex(working.nextQueueLength), arrow,
		[this](std::size_t index)
		{
			working.nextQueueLength = MinNextQueueLength + static_cast<unsigned int>(index);
		});
	nextLengthRowPtr = nextLengthRow.get();
	rows.push_back(std::move(nextLengthRow));

	std::vector<sf::String> randomiserOptions{
		text.GetText(TextKey::Options::RandomiserSevenBag),
		text.GetText(TextKey::Options::RandomiserRandom) };

	auto randomiserRow = std::make_unique<UI::CarouselRow>(font, text.GetText(TextKey::Options::GameplayRandomiser),
		std::move(randomiserOptions), working.isSevenBagEnabled ? 0 : 1, arrow,
		[this](std::size_t index) { working.isSevenBagEnabled = (index == 0); });
	randomiserRowPtr = randomiserRow.get();
	rows.push_back(std::move(randomiserRow));

	LayOutRows(RowsTop, RowMargin, RowHeight, RowGap);
	selectedRow = 0;
}

void GameplayCategoryPanel::SyncRows()
{
	vibrationRowPtr->SetOn(working.isGamepadVibrationEnabled);
	lightbarRowPtr->SetOn(working.isGamepadLightbarEnabled);
	shakeRowPtr->SetOn(working.isScreenShakeEnabled);
	ghostRowPtr->SetOn(working.isGhostPieceEnabled);
	holdRowPtr->SetOn(working.isHoldTetrominoEnabled);
	nextLengthRowPtr->SetCurrent(GetNextLengthIndex(working.nextQueueLength));
	randomiserRowPtr->SetCurrent(working.isSevenBagEnabled ? 0 : 1);
}

void GameplayCategoryPanel::ApplyWorking()
{
	GameSettings& saved = context.settings.GetSettings();
	saved.isGamepadVibrationEnabled = working.isGamepadVibrationEnabled;
	saved.isGamepadLightbarEnabled = working.isGamepadLightbarEnabled;
	saved.isScreenShakeEnabled = working.isScreenShakeEnabled;
	saved.isGhostPieceEnabled = working.isGhostPieceEnabled;
	saved.isHoldTetrominoEnabled = working.isHoldTetrominoEnabled;
	saved.nextQueueLength = working.nextQueueLength;
	saved.isSevenBagEnabled = working.isSevenBagEnabled;

	context.settings.Apply(context);   // the gamepad toggles take effect immediately
	context.settings.Save();
}

void GameplayCategoryPanel::ResetWorking()
{
	const GameSettings defaults;
	working.isGamepadVibrationEnabled = defaults.isGamepadVibrationEnabled;
	working.isGamepadLightbarEnabled = defaults.isGamepadLightbarEnabled;
	working.isScreenShakeEnabled = defaults.isScreenShakeEnabled;
	working.isGhostPieceEnabled = defaults.isGhostPieceEnabled;
	working.isHoldTetrominoEnabled = defaults.isHoldTetrominoEnabled;
	working.nextQueueLength = defaults.nextQueueLength;
	working.isSevenBagEnabled = defaults.isSevenBagEnabled;
	SyncRows();
}

void GameplayCategoryPanel::AdjustRow(std::size_t index, int direction)
{
	if (index < rows.size()) { AdjustRowByType(*rows[index], direction); }
}

void GameplayCategoryPanel::ActivateRow(std::size_t index)
{
	if (index < rows.size()) { ActivateRowByType(*rows[index]); }
}

void GameplayCategoryPanel::RowClicked(std::size_t index, int direction)
{
	if (index < rows.size()) { RowClickedByType(*rows[index], direction); }
}

void GameplayCategoryPanel::RefreshText()
{
	SettingsCategoryPanel::RefreshText();
	restartNote.setString(context.localization.GetText(TextKey::Options::RestartToApply));
}

void GameplayCategoryPanel::RenderExtra(sf::RenderTarget& target, float alpha)
{
	sf::Color color = restartNote.getFillColor();
	color.a = static_cast<std::uint8_t>(std::clamp(alpha, 0.f, 1.f) * 255.f);
	restartNote.setFillColor(color);
	target.draw(restartNote);
}
