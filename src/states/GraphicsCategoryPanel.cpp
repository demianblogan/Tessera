#include "GraphicsCategoryPanel.h"

#include <algorithm>
#include <cstdint>
#include <string>

#include <SFML/Graphics/RenderTarget.hpp>

#include "../audio/AudioPlayer.h"
#include "../core/Context.h"
#include "../display/DisplayManager.h"
#include "../localization/LocalizationManager.h"
#include "../localization/TextKeys.h"
#include "../resources/Assets.h"
#include "../settings/SettingsManager.h"
#include "OptionsSfx.h"

namespace
{
	namespace Sfx = OptionsSfx;

	constexpr sf::FloatRect PanelBounds{ { 600.f, 222.f }, { 1260.f, 706.f } };
	constexpr float RowsTop = PanelBounds.position.y + 56.f;
	constexpr float RowMargin = 84.f;
	constexpr float RowHeight = 90.f;
	constexpr float RowGap = 10.f;

	[[nodiscard]] sf::String FormatResolution(sf::Vector2u size)
	{
		return sf::String(std::to_string(size.x) + "  x  " + std::to_string(size.y));
	}
}

GraphicsCategoryPanel::GraphicsCategoryPanel(Context& context, sf::Color accent)
	: SettingsCategoryPanel(context, accent, PanelBounds)
	, resolutions(context.display.AvailableResolutions())
	, borderlessNote(context.fonts.Get(Assets::FontID::Main),
		context.localization.GetText(TextKey::Options::BorderlessNote), 24)
{
	borderlessNote.setFillColor(sf::Color(150, 160, 175));
	BuildRows();
}

GameSettings GraphicsCategoryPanel::DefaultSettings() const
{
	GameSettings defaults;
	defaults.display.resolution = context.display.DesktopResolution();
	return defaults;
}

bool GraphicsCategoryPanel::SettingsEqual(const GameSettings& a, const GameSettings& b) const
{
	return a.display == b.display
		&& a.verticalSyncEnabled == b.verticalSyncEnabled
		&& a.showFps == b.showFps
		&& a.crtFilterEnabled == b.crtFilterEnabled;
}

std::size_t GraphicsCategoryPanel::ResolutionIndexFor(sf::Vector2u resolution) const
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
	const auto pixels = [](sf::Vector2u s) { return static_cast<std::uint64_t>(s.x) * s.y; };
	for (std::size_t i = 0; i < resolutions.size(); ++i)
	{
		const std::uint64_t a = pixels(resolutions[i]);
		const std::uint64_t b = pixels(resolution);
		const std::uint64_t delta = a > b ? a - b : b - a;
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
		std::move(resolutionOptions), ResolutionIndexFor(working.display.resolution), arrow,
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
		working.verticalSyncEnabled, [this](bool value) { working.verticalSyncEnabled = value; });
	vsyncRowPtr = vsyncRow.get();
	rows.push_back(std::move(vsyncRow));

	auto showFpsRow = std::make_unique<UI::ToggleRow>(font, text.GetText(TextKey::Options::ShowFps), checkbox,
		working.showFps, [this](bool value) { working.showFps = value; });
	showFpsRowPtr = showFpsRow.get();
	rows.push_back(std::move(showFpsRow));

	auto crtRow = std::make_unique<UI::ToggleRow>(font, text.GetText(TextKey::Options::CrtFilter), checkbox,
		working.crtFilterEnabled, [this](bool value) { working.crtFilterEnabled = value; });
	crtRowPtr = crtRow.get();
	rows.push_back(std::move(crtRow));

	LayOutRows(RowsTop, RowMargin, RowHeight, RowGap);
	selectedRow = 0;
}

void GraphicsCategoryPanel::SyncRows()
{
	resolutionRowPtr->SetCurrent(ResolutionIndexFor(working.display.resolution));
	resolutionRowPtr->SetEnabled(working.display.windowMode != Display::WindowMode::Borderless);
	windowModeRowPtr->SetCurrent(static_cast<std::size_t>(working.display.windowMode));
	vsyncRowPtr->SetOn(working.verticalSyncEnabled);
	showFpsRowPtr->SetOn(working.showFps);
	crtRowPtr->SetOn(working.crtFilterEnabled);
}

void GraphicsCategoryPanel::ApplyWorking()
{
	GameSettings& saved = context.settings.GetSettings();
	const bool displayChanged = !(saved.display == working.display);

	saved.display = working.display;
	saved.verticalSyncEnabled = working.verticalSyncEnabled;
	saved.showFps = working.showFps;
	saved.crtFilterEnabled = working.crtFilterEnabled;

	context.settings.Apply(context);
	context.settings.Save();

	if (displayChanged)
	{
		context.display.RequestApply(saved.display);
	}
}

void GraphicsCategoryPanel::ResetWorking()
{
	const GameSettings defaults = DefaultSettings();
	working.display = defaults.display;
	working.verticalSyncEnabled = defaults.verticalSyncEnabled;
	working.showFps = defaults.showFps;
	working.crtFilterEnabled = defaults.crtFilterEnabled;
	SyncRows();
}

void GraphicsCategoryPanel::AdjustRow(std::size_t index, int direction)
{
	if (index >= rows.size())
	{
		return;
	}

	if (index >= FirstToggleRow)
	{
		rows[index]->Adjust(direction);
		Sfx::Toggle(context.audioPlayer);
	}
	else
	{
		auto* carousel = static_cast<UI::CarouselRow*>(rows[index].get());
		const std::size_t before = carousel->Current();
		carousel->Adjust(direction);
		if (carousel->Current() != before)
		{
			Sfx::Step(context.audioPlayer, direction);
		}
	}
}

void GraphicsCategoryPanel::ActivateRow(std::size_t index)
{
	if (index >= rows.size())
	{
		return;
	}
	rows[index]->Activate();
	if (index >= FirstToggleRow)
	{
		Sfx::Toggle(context.audioPlayer);
	}
}

void GraphicsCategoryPanel::RowClicked(std::size_t index)
{
	if (index >= FirstToggleRow) { Sfx::Toggle(context.audioPlayer); }
	else { Sfx::Step(context.audioPlayer, 1); }
}

void GraphicsCategoryPanel::RenderExtra(sf::RenderTarget& target, float alpha)
{
	if (working.display.windowMode != Display::WindowMode::Borderless || resolutionRowPtr == nullptr)
	{
		return;
	}

	const sf::FloatRect bounds = resolutionRowPtr->Bounds();
	borderlessNote.setPosition({ bounds.position.x + 26.f, bounds.position.y + bounds.size.y * 0.66f });
	sf::Color c = borderlessNote.getFillColor();
	c.a = static_cast<std::uint8_t>(std::clamp(alpha, 0.f, 1.f) * 255.f);
	borderlessNote.setFillColor(c);
	target.draw(borderlessNote);
}
