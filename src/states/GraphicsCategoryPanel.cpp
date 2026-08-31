#include "GraphicsCategoryPanel.h"

#include <algorithm>
#include <cstdint>
#include <optional>
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
#include "../ui/ColourUtils.h"
#include "../ui/TextLayout.h"

namespace
{
	constexpr sf::FloatRect PanelBounds{ { 600.f, 190.f }, { 1260.f, 760.f } };
	constexpr unsigned int PanelSourceBorder = 28u;
	constexpr sf::Vector2f PanelTargetBorder{ 44.f, 44.f };

	constexpr unsigned int TitleSize = 52;
	constexpr unsigned int ButtonSize = 40;

	constexpr float TitleY = PanelBounds.position.y + 56.f;
	constexpr float RowsTop = PanelBounds.position.y + 128.f;
	constexpr float RowMargin = 80.f;
	constexpr float RowHeight = 74.f;
	constexpr float RowGap = 6.f;
	constexpr float ButtonRowY = PanelBounds.position.y + PanelBounds.size.y - 78.f;
	constexpr float ButtonGap = 90.f;

	constexpr float FadeSpeed = 9.f;
	constexpr float PreviewOpacity = 0.55f;

	const sf::Color DisabledButton{ 120, 124, 132 };
	const sf::Color IdleButton{ 188, 194, 204 };

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
	, buttons{ {
		{ context.fonts.Get(Assets::FontID::Menu), ButtonSize },
		{ context.fonts.Get(Assets::FontID::Menu), ButtonSize },
		{ context.fonts.Get(Assets::FontID::Menu), ButtonSize } } }
	, buttonGlow(context.shaders.Get(Assets::ShaderID::NeonDilate), context.shaders.Get(Assets::ShaderID::NeonBlur))
	, dialog(context.fonts.Get(Assets::FontID::Main), context.fonts.Get(Assets::FontID::Menu),
		context.shaders.Get(Assets::ShaderID::NeonDilate), context.shaders.Get(Assets::ShaderID::NeonBlur))
	, resolutions(context.display.AvailableResolutions())
{
	UI::TextLayout::CentreOrigin(titleText);
	titleText.setPosition({ PanelBounds.position.x + PanelBounds.size.x * 0.5f, TitleY });

	const LocalizationManager& text = context.localization;
	buttons[ButtonId::Apply].SetText(text.GetText(TextKey::Options::Apply));
	buttons[ButtonId::Reset].SetText(text.GetText(TextKey::Options::Reset));
	buttons[ButtonId::Back].SetText(text.GetText(TextKey::Options::BackButton));

	sf::Vector2f maxGlowBox{ 0.f, 0.f };
	for (UI::MenuLabel& button : buttons)
	{
		maxGlowBox.x = std::max(maxGlowBox.x, button.GlowBox().x);
		maxGlowBox.y = std::max(maxGlowBox.y, button.GlowBox().y);
	}
	for (UI::MenuLabel& button : buttons)
	{
		button.SetGlowBoxSize(maxGlowBox);
	}

	working = applied = context.settings.GetSettings();
	BuildRows();
	LayOutButtons();
}

GameSettings GraphicsCategoryPanel::Defaults() const
{
	GameSettings defaults;
	defaults.display.resolution = context.display.DesktopResolution();
	return defaults;
}

bool GraphicsCategoryPanel::GraphicsMatch(const GameSettings& a, const GameSettings& b) const
{
	return a.display == b.display
		&& a.verticalSyncEnabled == b.verticalSyncEnabled
		&& a.showFps == b.showFps
		&& a.crtFilterEnabled == b.crtFilterEnabled;
}

bool GraphicsCategoryPanel::IsDirty() const { return !GraphicsMatch(working, applied); }
bool GraphicsCategoryPanel::IsAtDefaults() const { return GraphicsMatch(working, Defaults()); }

bool GraphicsCategoryPanel::ButtonEnabled(std::size_t index) const
{
	switch (index)
	{
	case ButtonId::Apply: return IsDirty();
	case ButtonId::Reset: return !IsAtDefaults();
	default:              return true;   // Back
	}
}

std::size_t GraphicsCategoryPanel::FirstEnabledButton() const
{
	for (std::size_t i = 0; i < ButtonId::ButtonCount; ++i)
	{
		if (ButtonEnabled(i))
		{
			return i;
		}
	}
	return ButtonId::Back;
}

std::size_t GraphicsCategoryPanel::LastEnabledRow() const
{
	for (std::size_t i = rows.size(); i-- > 0;)
	{
		if (rows[i]->IsEnabled())
		{
			return i;
		}
	}
	return 0;
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

	rows.clear();

	std::vector<sf::String> resolutionOptions;
	resolutionOptions.reserve(resolutions.size());
	for (const sf::Vector2u size : resolutions)
	{
		resolutionOptions.push_back(FormatResolution(size));
	}

	auto resolutionRow = std::make_unique<UI::CarouselRow>(font, text.GetText(TextKey::Options::Resolution),
		std::move(resolutionOptions), ResolutionIndexFor(working.display.resolution),
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
		std::move(modeOptions), static_cast<std::size_t>(working.display.windowMode),
		[this](std::size_t index)
		{
			working.display.windowMode = static_cast<Display::WindowMode>(index);
			resolutionRowPtr->SetEnabled(working.display.windowMode != Display::WindowMode::Borderless);
		});
	windowModeRowPtr = windowModeRow.get();
	rows.push_back(std::move(windowModeRow));

	const sf::String on = text.GetText(TextKey::Options::On);
	const sf::String off = text.GetText(TextKey::Options::Off);

	auto vsyncRow = std::make_unique<UI::ToggleRow>(font, text.GetText(TextKey::Options::Vsync), on, off,
		working.verticalSyncEnabled, [this](bool value) { working.verticalSyncEnabled = value; });
	vsyncRowPtr = vsyncRow.get();
	rows.push_back(std::move(vsyncRow));

	auto showFpsRow = std::make_unique<UI::ToggleRow>(font, text.GetText(TextKey::Options::ShowFps), on, off,
		working.showFps, [this](bool value) { working.showFps = value; });
	showFpsRowPtr = showFpsRow.get();
	rows.push_back(std::move(showFpsRow));

	auto crtRow = std::make_unique<UI::ToggleRow>(font, text.GetText(TextKey::Options::CrtFilter), on, off,
		working.crtFilterEnabled, [this](bool value) { working.crtFilterEnabled = value; });
	crtRowPtr = crtRow.get();
	rows.push_back(std::move(crtRow));

	for (std::size_t i = 0; i < rows.size(); ++i)
	{
		rows[i]->SetLayout(
			{ PanelBounds.position.x + RowMargin, RowsTop + static_cast<float>(i) * (RowHeight + RowGap) },
			PanelBounds.size.x - 2.f * RowMargin, RowHeight);
	}

	selectedRow = 0;
}

void GraphicsCategoryPanel::LayOutButtons()
{
	float totalWidth = 0.f;
	for (const UI::MenuLabel& button : buttons)
	{
		totalWidth += button.InkSize().x;
	}
	totalWidth += ButtonGap * (ButtonId::ButtonCount - 1);

	float x = PanelBounds.position.x + PanelBounds.size.x * 0.5f - totalWidth * 0.5f;
	for (std::size_t i = 0; i < ButtonId::ButtonCount; ++i)
	{
		const float half = buttons[i].InkSize().x * 0.5f;
		buttonPositions[i] = { x + half, ButtonRowY };
		x += buttons[i].InkSize().x + ButtonGap;
	}
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

void GraphicsCategoryPanel::Open()
{
	working = applied = context.settings.GetSettings();
	BuildRows();
	LayOutButtons();
	focus = Focus::Rows;
	selectedRow = 0;
	for (std::size_t i = 0; i < rows.size(); ++i)
	{
		if (rows[i]->IsEnabled()) { selectedRow = i; break; }
	}
	selectedButton = ButtonId::Back;
	closeRequested = false;
	active = true;
}

void GraphicsCategoryPanel::Close()
{
	active = false;
	closeRequested = false;
}

void GraphicsCategoryPanel::MoveVertical(int direction)
{
	if (focus == Focus::Buttons)
	{
		if (direction < 0)
		{
			focus = Focus::Rows;
			selectedRow = LastEnabledRow();
		}
		return;
	}

	int index = static_cast<int>(selectedRow);
	while (true)
	{
		index += direction;
		if (index >= static_cast<int>(rows.size()))
		{
			focus = Focus::Buttons;
			selectedButton = FirstEnabledButton();
			return;
		}
		if (index < 0)
		{
			return;
		}
		if (rows[static_cast<std::size_t>(index)]->IsEnabled())
		{
			selectedRow = static_cast<std::size_t>(index);
			return;
		}
	}
}

void GraphicsCategoryPanel::MoveHorizontal(int direction)
{
	if (focus == Focus::Rows)
	{
		if (selectedRow < rows.size())
		{
			rows[selectedRow]->Adjust(direction);
		}
		return;
	}

	int index = static_cast<int>(selectedButton);
	while (true)
	{
		index += direction;
		if (index < 0 || index >= static_cast<int>(ButtonId::ButtonCount))
		{
			return;
		}
		if (ButtonEnabled(static_cast<std::size_t>(index)))
		{
			selectedButton = static_cast<std::size_t>(index);
			return;
		}
	}
}

void GraphicsCategoryPanel::ConfirmFocused()
{
	if (focus == Focus::Rows)
	{
		if (selectedRow < rows.size())
		{
			rows[selectedRow]->Activate();
		}
		return;
	}

	switch (selectedButton)
	{
	case ButtonId::Apply: if (ButtonEnabled(ButtonId::Apply)) { DoApply(); } break;
	case ButtonId::Reset: if (ButtonEnabled(ButtonId::Reset)) { DoReset(); } break;
	default:              BackPressed(); break;
	}
}

void GraphicsCategoryPanel::BackPressed()
{
	if (IsDirty())
	{
		const LocalizationManager& text = context.localization;
		dialog.Show(text.GetText(TextKey::Options::Unsaved),
			text.GetText(TextKey::Common::Yes), text.GetText(TextKey::Common::No));
	}
	else
	{
		closeRequested = true;
	}
}

void GraphicsCategoryPanel::DoApply()
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

	applied = saved;

	if (focus == Focus::Buttons && !ButtonEnabled(selectedButton))
	{
		selectedButton = ButtonId::Back;
	}
}

void GraphicsCategoryPanel::DoReset()
{
	const GameSettings defaults = Defaults();
	working.display = defaults.display;
	working.verticalSyncEnabled = defaults.verticalSyncEnabled;
	working.showFps = defaults.showFps;
	working.crtFilterEnabled = defaults.crtFilterEnabled;

	SyncRows();

	if (focus == Focus::Buttons && !ButtonEnabled(selectedButton))
	{
		selectedButton = FirstEnabledButton();
	}
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

bool GraphicsCategoryPanel::WantsToStayOpen() const
{
	return dialog.IsOpen();
}

void GraphicsCategoryPanel::Update(float deltaTime)
{
	alpha += (targetAlpha - alpha) * std::min(1.f, deltaTime * FadeSpeed);

	dialog.Update(deltaTime);
	if (const std::optional<bool> answer = dialog.TakeResult())
	{
		if (*answer)
		{
			DoApply();
		}
		closeRequested = true;
	}

	buttonGlow.Update(deltaTime);
	for (std::size_t i = 0; i < ButtonId::ButtonCount; ++i)
	{
		const bool focused = active && focus == Focus::Buttons && selectedButton == i && !dialog.IsOpen();
		buttons[i].SetWaveEnabled(focused);
		buttons[i].Update(deltaTime);
	}

	for (std::size_t i = 0; i < rows.size(); ++i)
	{
		rows[i]->SetSelected(active && focus == Focus::Rows && i == selectedRow && !dialog.IsOpen());
		rows[i]->Update(deltaTime);
	}
}

void GraphicsCategoryPanel::Render(sf::RenderTarget& target)
{
	if (alpha > 0.01f)
	{
		const auto a = static_cast<std::uint8_t>(std::clamp(alpha, 0.f, 1.f) * 255.f);

		frame.SetColor(sf::Color(255, 255, 255, a));
		frame.Draw(target);

		titleText.setFillColor(sf::Color(accent.r, accent.g, accent.b, a));
		target.draw(titleText);

		for (const std::unique_ptr<UI::OptionRow>& row : rows)
		{
			row->Render(target, alpha);
		}

		for (std::size_t i = 0; i < ButtonId::ButtonCount; ++i)
		{
			const bool enabled = ButtonEnabled(i);
			const bool focused = active && focus == Focus::Buttons && selectedButton == i && !dialog.IsOpen();

			const sf::Color colour = !enabled ? DisabledButton : (focused ? sf::Color::White : IdleButton);
			const float scale = focused ? 1.06f : 1.f;

			if (focused)
			{
				buttons[i].DrawGlow(target, buttonGlow, buttonPositions[i], scale,
					UI::ScaleRgb(sf::Color::White, 0.5f * alpha));
			}
			buttons[i].Draw(target, buttonPositions[i], scale, colour, alpha);
		}
	}

	dialog.Render(target);
}

bool GraphicsCategoryPanel::HandleEvent(const sf::Event& event)
{
	if (!active)
	{
		return false;
	}

	const MenuInput::Action action = MenuInput::Resolve(event, context.gamepad);

	if (dialog.IsOpen())
	{
		dialog.Navigate(action);
		return true;
	}

	switch (action)
	{
	case MenuInput::Action::Up:      MoveVertical(-1);   return true;
	case MenuInput::Action::Down:    MoveVertical(1);    return true;
	case MenuInput::Action::Left:    MoveHorizontal(-1); return true;
	case MenuInput::Action::Right:   MoveHorizontal(1);  return true;
	case MenuInput::Action::Confirm: ConfirmFocused();   return true;
	case MenuInput::Action::Back:    BackPressed();      return true;
	default:                         break;
	}

	if (const auto* moved = event.getIf<sf::Event::MouseMoved>())
	{
		const sf::Vector2f point = context.window.mapPixelToCoords(moved->position);
		for (std::size_t i = 0; i < rows.size(); ++i)
		{
			if (rows[i]->IsEnabled() && rows[i]->Bounds().contains(point))
			{
				focus = Focus::Rows;
				selectedRow = i;
			}
			rows[i]->HandlePointer(point, false);
		}
		for (std::size_t i = 0; i < ButtonId::ButtonCount; ++i)
		{
			if (ButtonEnabled(i) && buttons[i].Bounds(buttonPositions[i], 1.f).contains(point))
			{
				focus = Focus::Buttons;
				selectedButton = i;
			}
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
					focus = Focus::Rows;
					selectedRow = i;
				}
			}
			for (std::size_t i = 0; i < ButtonId::ButtonCount; ++i)
			{
				if (ButtonEnabled(i) && buttons[i].Bounds(buttonPositions[i], 1.f).contains(point))
				{
					focus = Focus::Buttons;
					selectedButton = i;
					ConfirmFocused();
				}
			}
		}
		return true;
	}

	return false;
}
