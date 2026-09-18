#include "GraphicsCategoryPanel.h"

#include <algorithm>
#include <cstdint>
#include <string>

#include <SFML/Graphics/RenderTarget.hpp>

#include "../../core/Context.h"
#include "../../display/DisplayManager.h"
#include "../../localization/LocalizationManager.h"
#include "../../localization/TextKeys.h"
#include "../../resources/Assets.h"
#include "../../settings/SettingsManager.h"

namespace
{
	constexpr sf::FloatRect PanelBounds{ { 600.f, 222.f }, { 1260.f, 706.f } };
	constexpr float RowsTop = PanelBounds.position.y + 56.f;
	constexpr float RowMargin = 84.f;
	constexpr float RowHeight = 90.f;
	constexpr float RowGap = 10.f;

	constexpr unsigned int BorderlessNoteSize = 24u;
	const sf::Color BorderlessNoteColor{ 150, 160, 175 };
	constexpr float BorderlessNoteOffsetX = 26.f;
	constexpr float BorderlessNoteHeightFraction = 0.66f;

	[[nodiscard]] sf::String FormatResolution(sf::Vector2u size)
	{
		return sf::String(std::to_string(size.x) + "  x  " + std::to_string(size.y));
	}
}

GraphicsCategoryPanel::GraphicsCategoryPanel(Context& context, sf::Color accent)
	: SettingsCategoryPanel(context, accent, PanelBounds, context.textures.Get(Assets::TextureID::UiFrameBlue))
	, resolutions(context.display.GetAvailableResolutions())
	, borderlessNote(context.fonts.Get(Assets::FontID::Main),
		context.localization.GetText(TextKey::Options::BorderlessNote), BorderlessNoteSize)
{
	borderlessNote.setFillColor(BorderlessNoteColor);
	BuildRows();
}

GameSettings GraphicsCategoryPanel::GetDefaultSettings() const
{
	GameSettings defaults;
	defaults.display.resolution = context.display.GetDesktopResolution();
	return defaults;
}

bool GraphicsCategoryPanel::IsSettingsEqual(const GameSettings& current, const GameSettings& saved) const
{
	return current.display == saved.display
		&& current.isVerticalSyncEnabled == saved.isVerticalSyncEnabled
		&& current.needToShowFPS == saved.needToShowFPS
		&& current.isCRTFilterEnabled == saved.isCRTFilterEnabled;
}

std::size_t GraphicsCategoryPanel::GetResolutionIndex(sf::Vector2u resolution) const
{
	for (std::size_t i = 0; i < resolutions.size(); ++i)
	{
		if (resolutions[i] == resolution)
		{
			return i;
		}
	}

	std::size_t best = 0;
	std::uint64_t bestDelta = ~0ull;
	const auto pixels = [](sf::Vector2u size) { return static_cast<std::uint64_t>(size.x) * size.y; };
	for (std::size_t i = 0; i < resolutions.size(); ++i)
	{
		const std::uint64_t currentPixels = pixels(resolutions[i]);
		const std::uint64_t candidatePixels = pixels(resolution);
		const std::uint64_t delta = currentPixels > candidatePixels
			? currentPixels - candidatePixels : candidatePixels - currentPixels;
		if (delta < bestDelta)
		{
			bestDelta = delta;
			best = i;
		}
	}
	return best;
}

void GraphicsCategoryPanel::BuildRows()
{
	const LocalizationManager& text = context.localization;
	const sf::Font& font = context.fonts.Get(Assets::FontID::Main);
	const sf::Texture& arrow = context.textures.Get(Assets::TextureID::CarouselArrow);
	const sf::Texture& checkbox = context.textures.Get(Assets::TextureID::Checkbox);

	rows.clear();

	std::vector<sf::String> resolutionOptions;
	resolutionOptions.reserve(resolutions.size());
	for (const sf::Vector2u size : resolutions)
	{
		resolutionOptions.push_back(FormatResolution(size));
	}

	auto resolutionRow = std::make_unique<UI::CarouselRow>(font, text.GetText(TextKey::Options::Resolution),
		std::move(resolutionOptions), GetResolutionIndex(working.display.resolution), arrow,
		[this](std::size_t index)
		{
			if (index < resolutions.size())
			{
				working.display.resolution = resolutions[index];
			}
		});
	resolutionRowPtr = resolutionRow.get();
	resolutionRowPtr->SetEnabled(working.display.windowMode != Display::WindowMode::Borderless);
	rows.push_back(std::move(resolutionRow));

	std::vector<sf::String> modeOptions{
		text.GetText(TextKey::Options::ModeFullscreen),
		text.GetText(TextKey::Options::ModeBorderless),
		text.GetText(TextKey::Options::ModeWindow) };

	auto windowModeRow = std::make_unique<UI::CarouselRow>(font, text.GetText(TextKey::Options::WindowMode),
		std::move(modeOptions), static_cast<std::size_t>(working.display.windowMode), arrow,
		[this](std::size_t index)
		{
			working.display.windowMode = static_cast<Display::WindowMode>(index);
			resolutionRowPtr->SetEnabled(working.display.windowMode != Display::WindowMode::Borderless);
		});
	windowModeRowPtr = windowModeRow.get();
	rows.push_back(std::move(windowModeRow));

	auto vsyncRow = std::make_unique<UI::ToggleRow>(font, text.GetText(TextKey::Options::Vsync), checkbox,
		working.isVerticalSyncEnabled, [this](bool isEnabled) { working.isVerticalSyncEnabled = isEnabled; });
	vsyncRowPtr = vsyncRow.get();
	rows.push_back(std::move(vsyncRow));

	auto showFPSRow = std::make_unique<UI::ToggleRow>(font, text.GetText(TextKey::Options::ShowFps), checkbox,
		working.needToShowFPS, [this](bool isEnabled) { working.needToShowFPS = isEnabled; });
	showFPSRowPtr = showFPSRow.get();
	rows.push_back(std::move(showFPSRow));

	auto crtRow = std::make_unique<UI::ToggleRow>(font, text.GetText(TextKey::Options::CrtFilter), checkbox,
		working.isCRTFilterEnabled, [this](bool isEnabled) { working.isCRTFilterEnabled = isEnabled; });
	crtRowPtr = crtRow.get();
	rows.push_back(std::move(crtRow));

	LayOutRows(RowsTop, RowMargin, RowHeight, RowGap);
	selectedRow = 0;
}

void GraphicsCategoryPanel::SyncRows()
{
	resolutionRowPtr->SetCurrent(GetResolutionIndex(working.display.resolution));
	resolutionRowPtr->SetEnabled(working.display.windowMode != Display::WindowMode::Borderless);
	windowModeRowPtr->SetCurrent(static_cast<std::size_t>(working.display.windowMode));
	vsyncRowPtr->SetOn(working.isVerticalSyncEnabled);
	showFPSRowPtr->SetOn(working.needToShowFPS);
	crtRowPtr->SetOn(working.isCRTFilterEnabled);
}

void GraphicsCategoryPanel::ApplyWorking()
{
	GameSettings& saved = context.settings.GetSettings();
	const bool isDisplayChanged = !(saved.display == working.display);

	saved.display = working.display;
	saved.isVerticalSyncEnabled = working.isVerticalSyncEnabled;
	saved.needToShowFPS = working.needToShowFPS;
	saved.isCRTFilterEnabled = working.isCRTFilterEnabled;

	context.settings.Apply(context);
	context.settings.Save();

	if (isDisplayChanged)
	{
		context.display.RequestApply(saved.display);
	}
}

void GraphicsCategoryPanel::ResetWorking()
{
	const GameSettings defaults = GetDefaultSettings();
	working.display = defaults.display;
	working.isVerticalSyncEnabled = defaults.isVerticalSyncEnabled;
	working.needToShowFPS = defaults.needToShowFPS;
	working.isCRTFilterEnabled = defaults.isCRTFilterEnabled;
	SyncRows();
}

void GraphicsCategoryPanel::AdjustRow(std::size_t index, int direction)
{
	if (index < rows.size()) { AdjustRowByType(*rows[index], direction); }
}

void GraphicsCategoryPanel::ActivateRow(std::size_t index)
{
	if (index < rows.size()) { ActivateRowByType(*rows[index]); }
}

void GraphicsCategoryPanel::RowClicked(std::size_t index, int direction)
{
	if (index < rows.size()) { RowClickedByType(*rows[index], direction); }
}

void GraphicsCategoryPanel::RefreshText()
{
	SettingsCategoryPanel::RefreshText();
	borderlessNote.setString(context.localization.GetText(TextKey::Options::BorderlessNote));
}

void GraphicsCategoryPanel::RenderExtra(sf::RenderTarget& target, float alpha)
{
	if (working.display.windowMode != Display::WindowMode::Borderless || resolutionRowPtr == nullptr)
	{
		return;
	}

	const sf::FloatRect bounds = resolutionRowPtr->GetBounds();
	borderlessNote.setPosition({ bounds.position.x + BorderlessNoteOffsetX,
		bounds.position.y + bounds.size.y * BorderlessNoteHeightFraction });
	sf::Color color = borderlessNote.getFillColor();
	color.a = static_cast<std::uint8_t>(std::clamp(alpha, 0.f, 1.f) * 255.f);
	borderlessNote.setFillColor(color);
	target.draw(borderlessNote);
}
