#include "GameplayCategoryPanel.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

#include <SFML/Graphics/RenderTarget.hpp>

#include "../core/Context.h"
#include "../localization/LocalizationManager.h"
#include "../localization/TextKeys.h"
#include "../resources/Assets.h"
#include "../settings/GameSettings.h"
#include "../settings/SettingsManager.h"

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
}

GameplayCategoryPanel::GameplayCategoryPanel(Context& context, sf::Color accent)
	: SettingsCategoryPanel(context, accent, PanelBounds, context.textures.Get(Assets::TextureID::UiFrameCyan))
	, restartNote(context.fonts.Get(Assets::FontID::Main),
		context.localization.GetText(TextKey::Options::RestartToApply), 30)
{
	restartNote.setFillColor(sf::Color(255, 245, 140));   // light yellow -- a warning, not a label

	const sf::FloatRect noteBounds = restartNote.getLocalBounds();
	restartNote.setOrigin({ noteBounds.position.x + noteBounds.size.x * 0.5f, noteBounds.position.y });
	restartNote.setPosition({ PanelBounds.position.x + PanelBounds.size.x * 0.5f, PanelBounds.position.y + 34.f });

	BuildRows();
}

bool GameplayCategoryPanel::SettingsEqual(const GameSettings& a, const GameSettings& b) const
{
	return a.gamepadVibrationEnabled == b.gamepadVibrationEnabled
		&& a.gamepadLightbarEnabled == b.gamepadLightbarEnabled
		&& a.screenShakeEnabled == b.screenShakeEnabled
		&& a.ghostPieceEnabled == b.ghostPieceEnabled
		&& a.holdEnabled == b.holdEnabled
		&& a.nextQueueLength == b.nextQueueLength
		&& a.sevenBagEnabled == b.sevenBagEnabled;
}

GameSettings GameplayCategoryPanel::DefaultSettings() const
{
	return GameSettings{};
}

std::size_t GameplayCategoryPanel::NextLengthIndexFor(unsigned int length) const
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

	auto ghostRow = std::make_unique<UI::ToggleRow>(font, text.GetText(TextKey::Options::GameplayGhost),
		checkbox, working.ghostPieceEnabled, [this](bool on) { working.ghostPieceEnabled = on; });
	ghostRowPtr = ghostRow.get();
	rows.push_back(std::move(ghostRow));

	auto holdRow = std::make_unique<UI::ToggleRow>(font, text.GetText(TextKey::Options::GameplayHold),
		checkbox, working.holdEnabled, [this](bool on) { working.holdEnabled = on; });
	holdRowPtr = holdRow.get();
	rows.push_back(std::move(holdRow));

	std::vector<sf::String> lengthOptions;
	for (unsigned int length = MinNextQueueLength; length <= MaxNextQueueLength; ++length)
	{
		lengthOptions.push_back(sf::String(std::to_string(length)));
	}

	auto nextLengthRow = std::make_unique<UI::CarouselRow>(font, text.GetText(TextKey::Options::GameplayNextLength),
		std::move(lengthOptions), NextLengthIndexFor(working.nextQueueLength), arrow,
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
		std::move(randomiserOptions), working.sevenBagEnabled ? 0 : 1, arrow,
		[this](std::size_t index) { working.sevenBagEnabled = (index == 0); });
	randomiserRowPtr = randomiserRow.get();
	rows.push_back(std::move(randomiserRow));

	LayOutRows(RowsTop, RowMargin, RowHeight, RowGap);
	selectedRow = 0;
}

void GameplayCategoryPanel::SyncRows()
{
	vibrationRowPtr->SetOn(working.gamepadVibrationEnabled);
	lightbarRowPtr->SetOn(working.gamepadLightbarEnabled);
	shakeRowPtr->SetOn(working.screenShakeEnabled);
	ghostRowPtr->SetOn(working.ghostPieceEnabled);
	holdRowPtr->SetOn(working.holdEnabled);
	nextLengthRowPtr->SetCurrent(NextLengthIndexFor(working.nextQueueLength));
	randomiserRowPtr->SetCurrent(working.sevenBagEnabled ? 0 : 1);
}

void GameplayCategoryPanel::ApplyWorking()
{
	GameSettings& saved = context.settings.GetSettings();
	saved.gamepadVibrationEnabled = working.gamepadVibrationEnabled;
	saved.gamepadLightbarEnabled = working.gamepadLightbarEnabled;
	saved.screenShakeEnabled = working.screenShakeEnabled;
	saved.ghostPieceEnabled = working.ghostPieceEnabled;
	saved.holdEnabled = working.holdEnabled;
	saved.nextQueueLength = working.nextQueueLength;
	saved.sevenBagEnabled = working.sevenBagEnabled;

	context.settings.Apply(context);   // the gamepad toggles take effect immediately
	context.settings.Save();
}

void GameplayCategoryPanel::ResetWorking()
{
	const GameSettings defaults;
	working.gamepadVibrationEnabled = defaults.gamepadVibrationEnabled;
	working.gamepadLightbarEnabled = defaults.gamepadLightbarEnabled;
	working.screenShakeEnabled = defaults.screenShakeEnabled;
	working.ghostPieceEnabled = defaults.ghostPieceEnabled;
	working.holdEnabled = defaults.holdEnabled;
	working.nextQueueLength = defaults.nextQueueLength;
	working.sevenBagEnabled = defaults.sevenBagEnabled;
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
	sf::Color c = restartNote.getFillColor();
	c.a = static_cast<std::uint8_t>(std::clamp(alpha, 0.f, 1.f) * 255.f);
	restartNote.setFillColor(c);
	target.draw(restartNote);
}
