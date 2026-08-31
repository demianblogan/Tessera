#include "GraphicsCategoryPanel.h"

#include <algorithm>
#include <cstdint>
#include <string>

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Mouse.hpp>

#include "../core/Context.h"
#include "../display/DisplayManager.h"
#include "../input/MenuInput.h"
#include "../localization/LocalizationManager.h"
#include "../localization/TextKeys.h"
#include "../resources/Assets.h"
#include "../settings/SettingsManager.h"
#include "../ui/TextLayout.h"

namespace
{
	constexpr sf::FloatRect PanelBounds{ { 600.f, 200.f }, { 1260.f, 730.f } };
	constexpr unsigned int PanelSourceBorder = 28u;
	constexpr sf::Vector2f PanelTargetBorder{ 44.f, 44.f };

	constexpr unsigned int TitleSize = 52;
	constexpr float TitleY = PanelBounds.position.y + 56.f;

	constexpr float RowsTop = PanelBounds.position.y + 130.f;
	constexpr float RowMargin = 80.f;
	constexpr float RowHeight = 74.f;
	constexpr float RowGap = 6.f;

	constexpr float FadeSpeed = 9.f;
	constexpr float PreviewOpacity = 0.55f;

	[[nodiscard]] sf::String FormatResolution(sf::Vector2u size)
	{
		return sf::String(std::to_string(size.x) + "  x  " + std::to_string(size.y));
	}
}

GraphicsCategoryPanel::GraphicsCategoryPanel(Context& context, sf::Color accent)
	: context(context)
	, accent(accent)
	, frame(context.textures.Get(Assets::TextureID::UiFrame), PanelBounds, PanelSourceBorder, PanelTargetBorder)
	, titleText(context.fonts.Get(Assets::FontID::Menu), context.localization.GetText(TextKey::Options::Graphics), TitleSize)
	, resolutions(context.display.AvailableResolutions())
{
	UI::TextLayout::CentreOrigin(titleText);
	titleText.setPosition({ PanelBounds.position.x + PanelBounds.size.x * 0.5f, TitleY });

	working = context.settings.GetSettings();
	BuildRows();
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

	// Closest by pixel count.
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

	rows.clear();

	std::vector<sf::String> resolutionOptions;
	resolutionOptions.reserve(resolutions.size());
	for (const sf::Vector2u size : resolutions)
	{
		resolutionOptions.push_back(FormatResolution(size));
	}

	auto resolutionRowPtr = std::make_unique<UI::CarouselRow>(font, text.GetText(TextKey::Options::Resolution),
		std::move(resolutionOptions), ResolutionIndexFor(working.display.resolution),
		[this](std::size_t index)
		{
			if (index < resolutions.size())
			{
				working.display.resolution = resolutions[index];
			}
		});
	resolutionRowPtr->SetEnabled(working.display.windowMode != Display::WindowMode::Borderless);
	rows.push_back(std::move(resolutionRowPtr));
	resolutionRow = 0;

	std::vector<sf::String> modeOptions{
		text.GetText(TextKey::Options::ModeFullscreen),
		text.GetText(TextKey::Options::ModeBorderless),
		text.GetText(TextKey::Options::ModeWindow) };

	rows.push_back(std::make_unique<UI::CarouselRow>(font, text.GetText(TextKey::Options::WindowMode),
		std::move(modeOptions), static_cast<std::size_t>(working.display.windowMode),
		[this](std::size_t index)
		{
			working.display.windowMode = static_cast<Display::WindowMode>(index);
			rows[resolutionRow]->SetEnabled(working.display.windowMode != Display::WindowMode::Borderless);
		}));

	const sf::String on = text.GetText(TextKey::Options::On);
	const sf::String off = text.GetText(TextKey::Options::Off);

	rows.push_back(std::make_unique<UI::ToggleRow>(font, text.GetText(TextKey::Options::Vsync), on, off,
		working.verticalSyncEnabled, [this](bool value) { working.verticalSyncEnabled = value; }));
	rows.push_back(std::make_unique<UI::ToggleRow>(font, text.GetText(TextKey::Options::ShowFps), on, off,
		working.showFps, [this](bool value) { working.showFps = value; }));
	rows.push_back(std::make_unique<UI::ToggleRow>(font, text.GetText(TextKey::Options::CrtFilter), on, off,
		working.crtFilterEnabled, [this](bool value) { working.crtFilterEnabled = value; }));

	for (std::size_t i = 0; i < rows.size(); ++i)
	{
		rows[i]->SetLayout(
			{ PanelBounds.position.x + RowMargin, RowsTop + static_cast<float>(i) * (RowHeight + RowGap) },
			PanelBounds.size.x - 2.f * RowMargin, RowHeight);
	}

	selected = 0;
}

void GraphicsCategoryPanel::Open()
{
	working = context.settings.GetSettings();
	BuildRows();
	active = true;
}

void GraphicsCategoryPanel::MoveSelection(int direction)
{
	if (rows.empty())
	{
		return;
	}

	const int count = static_cast<int>(rows.size());
	int index = static_cast<int>(selected);
	for (int step = 0; step < count; ++step)
	{
		index = (index + direction + count) % count;
		if (rows[static_cast<std::size_t>(index)]->IsEnabled())
		{
			break;
		}
	}
	selected = static_cast<std::size_t>(index);
}

void GraphicsCategoryPanel::SetVisibility(Visibility visibility, float previewFade)
{
	switch (visibility)
	{
	case Visibility::Open:    targetAlpha = 1.f; break;
	case Visibility::Preview: targetAlpha = PreviewOpacity * std::clamp(previewFade, 0.f, 1.f); break;
	case Visibility::Hidden:  targetAlpha = 0.f; break;
	}

	active = visibility == Visibility::Open;
}

void GraphicsCategoryPanel::Update(float deltaTime)
{
	alpha += (targetAlpha - alpha) * std::min(1.f, deltaTime * FadeSpeed);

	for (std::size_t i = 0; i < rows.size(); ++i)
	{
		rows[i]->SetSelected(active && i == selected);
		rows[i]->Update(deltaTime);
	}
}

void GraphicsCategoryPanel::Render(sf::RenderTarget& target)
{
	if (alpha <= 0.01f)
	{
		return;
	}

	const auto a = static_cast<std::uint8_t>(std::clamp(alpha, 0.f, 1.f) * 255.f);

	frame.SetColor(sf::Color(255, 255, 255, a));
	frame.Draw(target);

	titleText.setFillColor(sf::Color(accent.r, accent.g, accent.b, a));
	target.draw(titleText);

	for (const std::unique_ptr<UI::OptionRow>& row : rows)
	{
		row->Render(target, alpha);
	}
}

bool GraphicsCategoryPanel::HandleEvent(const sf::Event& event)
{
	if (!active)
	{
		return false;
	}

	switch (MenuInput::Resolve(event, context.gamepad))
	{
	case MenuInput::Action::Up:      MoveSelection(-1); return true;
	case MenuInput::Action::Down:    MoveSelection(1);  return true;
	case MenuInput::Action::Left:    if (selected < rows.size()) { rows[selected]->Adjust(-1); } return true;
	case MenuInput::Action::Right:   if (selected < rows.size()) { rows[selected]->Adjust(1); }  return true;
	case MenuInput::Action::Confirm: if (selected < rows.size()) { rows[selected]->Activate(); } return true;
	case MenuInput::Action::Back:    return false;   // let OptionsScreen close the category (Apply flow comes next)
	default:                         break;
	}

	if (const auto* moved = event.getIf<sf::Event::MouseMoved>())
	{
		const sf::Vector2f point = context.window.mapPixelToCoords(moved->position);
		for (std::size_t i = 0; i < rows.size(); ++i)
		{
			if (rows[i]->IsEnabled() && rows[i]->Bounds().contains(point))
			{
				selected = i;
			}
			rows[i]->HandlePointer(point, false);
		}
		return true;
	}

	if (const auto* clicked = event.getIf<sf::Event::MouseButtonPressed>())
	{
		if (clicked->button == sf::Mouse::Button::Left)
		{
			const sf::Vector2f point = context.window.mapPixelToCoords(clicked->position);
			for (std::size_t i = 0; i < rows.size(); ++i)
			{
				if (rows[i]->HandlePointer(point, true))
				{
					selected = i;
				}
			}
		}
		return true;
	}

	return false;
}
