#include "SettingsState.h"

#include <string>

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Mouse.hpp>

#include "../audio/AudioPlayer.h"
#include "../core/StateMachine.h"
#include "../input/MenuInput.h"
#include "../ui/Slider.h"
#include "../resources/Assets.h"
#include "../settings/SettingsManager.h"
#include "MainMenuState.h"

namespace
{
	constexpr float TopSpacing = 70.f;
	constexpr float TitleSpacing = 80.f;
	constexpr float SectionGap = 120.f;
	constexpr float RowGap = 10.f;
	constexpr float RowHeight = 60.f;
	constexpr float FooterSpacing = 200.f;
	constexpr float FooterGap = 10.f;
	constexpr float SliderWidth = 350.f;
	constexpr float SliderTrackHeight = 12.f;
	constexpr float SliderHandleWidth = 20.f;
	constexpr float SliderHandleHeight = 40.f;
	constexpr float ToggleButtonWidth = 700.f;
	constexpr float ToggleButtonHeight = 60.f;
	constexpr float BlockStyleButtonWidth = 260.f;
	constexpr unsigned int TitleSize = 120;
	constexpr unsigned int SectionTitleSize = 60;
	constexpr unsigned int RowTextSize = 40;
	constexpr unsigned int FooterTextSize = 50;
	constexpr float SectionWidth = 900.f;

	constexpr float FrameRateSliderStep = 10.f;
	constexpr float VolumeSliderStep = 1.f;

	std::string FormatFrameRate(int value)
	{
		return value <= 0 ? std::string("Unlimited") : std::to_string(value);
	}
}

SettingsState::SettingsState(Context& context)
	: State(context.stateMachine)
	, context(context)
	, rootLayout(UI::Layout::Orientation::Vertical)
	, backgroundSprite(context.textures.Get(Assets::TextureID::MenuBackground))
{
	backgroundSprite.setColor(sf::Color(255, 255, 255, 180));

	rootLayout.SetHorizontalAlignment(UI::Layout::Alignment::Center);
	rootLayout.SetVerticalAlignment(UI::Layout::Alignment::Start);
	rootLayout.SetGap(0.f);

	// =====================================================
	// Top spacer
	// =====================================================

	rootLayout.Add(std::make_unique<UI::Spacer>(sf::Vector2f{ 0.f, TopSpacing }));

	// =====================================================
	// Title
	// =====================================================
	{
		auto title = std::make_unique<UI::Label>(context.fonts.Get(Assets::FontID::Main), "Settings", TitleSize);
		title->SetFillColor(sf::Color::White);
		rootLayout.Add(std::move(title));
	}

	// =====================================================
	// Spacer between title and content
	// =====================================================

	rootLayout.Add(std::make_unique<UI::Spacer>(sf::Vector2f{ 0.f, TitleSpacing }));

	// =====================================================
	// Content layout
	// =====================================================
	{
		auto content = std::make_unique<UI::Layout>(UI::Layout::Orientation::Vertical);

		content->SetGap(SectionGap);
		content->SetHorizontalAlignment(UI::Layout::Alignment::Start);

		CreateGraphicsSection(*content);
		CreateAudioSection(*content);

		content->SetPadding({ 200.f, 0.f, 0.f, 0.f });

		rootLayout.Add(std::move(content));
	}

	// =====================================================
	// Spacer before footer
	// =====================================================

	rootLayout.Add(std::make_unique<UI::Spacer>(sf::Vector2f{ 0.f, FooterSpacing }));

	// =====================================================
	// Footer
	// =====================================================
	{
		auto footer = std::make_unique<UI::Layout>(UI::Layout::Orientation::Vertical);

		footer->SetGap(FooterGap);
		footer->SetHorizontalAlignment(UI::Layout::Alignment::Center);

		auto label = std::make_unique<UI::Label>(
			context.fonts.Get(Assets::FontID::Main),
			"ESC - RETURN TO MAIN MENU",
			FooterTextSize
		);

		label->SetFillColor(sf::Color(180, 180, 180));
		footer->Add(std::move(label));

		rootLayout.Add(std::move(footer));
	}

	UpdateSliderLabels();
	UpdateSelection();
	UpdateLayout();
}

void SettingsState::CreateGraphicsSection(UI::Layout& parent)
{
	const auto& font = context.fonts.Get(Assets::FontID::Main);

	auto section = std::make_unique<UI::Layout>(UI::Layout::Orientation::Vertical);
	section->SetWidthPixels(900.f);
	section->SetGap(RowGap);

	// =====================================================
	// Section title
	// =====================================================

	section->SetHorizontalAlignment(UI::Layout::Alignment::Start);

	{
		auto label = std::make_unique<UI::Label>(font, "--- Graphics", SectionTitleSize);
		label->SetFillColor(sf::Color::White);
		section->Add(std::move(label));
	}

	// =====================================================
	// Vertical sync
	// =====================================================

	{
		const bool enabled = context.settings.GetSettings().verticalSyncEnabled;

		auto button = std::make_unique<UI::Button>(sf::Vector2f{ ToggleButtonWidth,	ToggleButtonHeight });
		button->SetTextAlignment(UI::Button::TextAlignment::Left);
		button->SetLabel(std::make_unique<UI::Label>(
			font,
			enabled ? "Vertical synchronization: ON" : "Vertical synchronization: OFF",
			RowTextSize)
		);

		button->SetNormalStyle({ .backgroundColor = sf::Color::Transparent,	.textColor = sf::Color::White });
		button->SetSelectedStyle({ .backgroundColor = sf::Color::Transparent, .textColor = sf::Color::Yellow });

		verticalSyncButton = button.get();

		selectableElements.push_back({ .type = SelectableType::Button, .button = verticalSyncButton });

		section->Add(std::move(button));
	}

	// =====================================================
	// FPS limit
	// =====================================================

	CreateSliderRow(
		*section,
		"FPS limit:",
		0.f,
		240.f,
		static_cast<float>(context.settings.GetSettings().frameRateLimit),
		FrameRateSliderStep,
		frameRateSetting
	);

	// =====================================================
	// Block style
	// =====================================================

	{
		const bool withOutline = context.settings.GetSettings().blockRenderStyle == BlockRenderStyle::WithOutline;

		auto button = std::make_unique<UI::Button>(sf::Vector2f{ ToggleButtonWidth,	ToggleButtonHeight });
		button->SetTextAlignment(UI::Button::TextAlignment::Left);
		button->SetLabel(std::make_unique<UI::Label>(
			font,
			withOutline ? "Block style: With outline" : "Block style: Without outline",
			RowTextSize)
		);

		button->SetNormalStyle(
			{
				.backgroundColor = sf::Color::Transparent,
				.textColor = sf::Color::White
			}
		);

		button->SetSelectedStyle(
			{
				.backgroundColor = sf::Color::Transparent,
				.textColor = sf::Color::Yellow
			}
		);

		blockStyleButton = button.get();

		selectableElements.push_back({ .type = SelectableType::Button, .button = blockStyleButton });

		section->Add(std::move(button));
	}

	parent.Add(std::move(section));
}

void SettingsState::CreateAudioSection(UI::Layout& parent)
{
	const auto& font = context.fonts.Get(Assets::FontID::Main);

	auto section = std::make_unique<UI::Layout>(UI::Layout::Orientation::Vertical);
	section->SetWidthPixels(900.f);
	section->SetGap(RowGap);

	// =====================================================
	// Title
	// =====================================================

	section->SetHorizontalAlignment(UI::Layout::Alignment::Center);

	{
		auto label = std::make_unique<UI::Label>(font, "--- Audio", SectionTitleSize);
		label->SetFillColor(sf::Color::White);
		section->Add(std::move(label));
	}

	section->SetHorizontalAlignment(UI::Layout::Alignment::Start);

	// =====================================================
	// Sliders
	// =====================================================

	CreateSliderRow(
		*section,
		"Sounds:",
		0.f,
		static_cast<float>(MaxVolumeStep),
		static_cast<float>(context.settings.GetSettings().soundVolume),
		VolumeSliderStep,
		soundSetting
	);

	CreateSliderRow(
		*section,
		"Music:",
		0.f,
		static_cast<float>(MaxVolumeStep),
		static_cast<float>(context.settings.GetSettings().musicVolume),
		VolumeSliderStep,
		musicSetting
	);

	parent.Add(std::move(section));
}

// =========================================================
// Slider row
// =========================================================

void SettingsState::CreateSliderRow(
	UI::Layout& parent,
	const sf::String& text,
	float minimum,
	float maximum,
	float value,
	float step,
	SliderSetting& setting)
{
	const auto& font = context.fonts.Get(Assets::FontID::Main);

	auto row = std::make_unique<UI::Layout>(UI::Layout::Orientation::Horizontal);
	row->SetGap(RowGap);
	row->SetVerticalAlignment(UI::Layout::Alignment::Center);
	row->SetHeightPixels(RowHeight);

	// =====================================================
	// Name
	// =====================================================

	auto name = std::make_unique<UI::Label>(font, text, RowTextSize);
	name->SetFillColor(sf::Color::White);
	setting.nameLabel = name.get();
	row->Add(std::move(name));

	// =====================================================
	// Min
	// =====================================================
	{
		auto label = std::make_unique<UI::Label>(font, "0", RowTextSize);
		label->SetFillColor(sf::Color::White);
		row->Add(std::move(label));
	}

	// =====================================================
	// Slider
	// =====================================================

	auto slider =
		std::make_unique<UI::Slider>(
			sf::Vector2f
			{
				SliderWidth,
				SliderTrackHeight
			},
			sf::Vector2f
			{
				SliderHandleWidth,
				SliderHandleHeight
			},
			minimum,
			maximum,
			value,
			step,
			UI::Slider::RectangleVisual
			{
				.fillColor = sf::Color(50, 50, 50)
			},
			UI::Slider::RectangleVisual
			{
				.fillColor = sf::Color(100, 220, 255)
			},
			UI::Slider::RectangleVisual
			{
				.fillColor = sf::Color::White
			}
		);

	setting.slider = slider.get();

	selectableElements.push_back({ .type = SelectableType::Slider, .slider = setting.slider });

	row->Add(std::move(slider));

	// =====================================================
	// Value
	// =====================================================

	auto valueLabel = std::make_unique<UI::Label>(font, std::to_string(static_cast<int>(value)), RowTextSize);
	valueLabel->SetFillColor(sf::Color::White);
	setting.valueLabel = valueLabel.get();
	row->Add(std::move(valueLabel));

	parent.Add(std::move(row));
}

void SettingsState::UpdateSelection()
{
	for (const SelectableElement& element : selectableElements)
	{
		if (element.type == SelectableType::Button)
		{
			element.button->SetSelected(false);
		}
	}

	auto resetSlider = [](SliderSetting& setting)
		{
			setting.nameLabel->SetFillColor(sf::Color::White);
			setting.valueLabel->SetFillColor(sf::Color::White);
		};

	resetSlider(frameRateSetting);
	resetSlider(soundSetting);
	resetSlider(musicSetting);

	const SelectableElement& selected = selectableElements[selectedIndex];

	if (selected.type == SelectableType::Button)
	{
		selected.button->SetSelected(true);
		return;
	}

	auto highlightSlider = [](SliderSetting& setting)
		{
			setting.nameLabel->SetFillColor(sf::Color::Yellow);
			setting.valueLabel->SetFillColor(sf::Color::Yellow);
		};

	if (selected.slider == frameRateSetting.slider)
	{
		highlightSlider(frameRateSetting);
	}
	else if (selected.slider == soundSetting.slider)
	{
		highlightSlider(soundSetting);
	}
	else if (selected.slider == musicSetting.slider)
	{
		highlightSlider(musicSetting);
	}
}

void SettingsState::UpdateSliderLabels()
{
	frameRateSetting.valueLabel->SetString(
		FormatFrameRate(static_cast<int>(frameRateSetting.slider->GetValue()))
	);

	soundSetting.valueLabel->SetString(
		std::to_string(static_cast<int>(soundSetting.slider->GetValue()))
	);

	musicSetting.valueLabel->SetString(
		std::to_string(static_cast<int>(musicSetting.slider->GetValue()))
	);
}

void SettingsState::SelectPrevious()
{
	selectedIndex--;

	if (selectedIndex < 0)
	{
		selectedIndex = static_cast<int>(selectableElements.size()) - 1;
	}

	UpdateSelection();

	context.audioPlayer.Restart(Assets::SoundID::MenuItemSelected);
}

void SettingsState::SelectNext()
{
	selectedIndex++;

	if (selectedIndex >= static_cast<int>(selectableElements.size()))
	{
		selectedIndex = 0;
	}

	UpdateSelection();

	context.audioPlayer.Restart(Assets::SoundID::MenuItemSelected);
}

void SettingsState::IncreaseCurrentSlider()
{
	const SelectableElement& selected = selectableElements[selectedIndex];

	if (selected.type != SelectableType::Slider)
	{
		return;
	}

	selected.slider->Increase();
	UpdateSliderLabels();
	ApplyAndSaveSettings();

	context.audioPlayer.Restart(Assets::SoundID::MenuItemSelected);
}

void SettingsState::DecreaseCurrentSlider()
{
	const SelectableElement& selected = selectableElements[selectedIndex];

	if (selected.type != SelectableType::Slider)
	{
		return;
	}

	selected.slider->Decrease();
	UpdateSliderLabels();
	ApplyAndSaveSettings();

	context.audioPlayer.Restart(Assets::SoundID::MenuItemSelected);
}

void SettingsState::ActivateCurrentElement()
{
	const SelectableElement& selected = selectableElements[selectedIndex];

	if (selected.type != SelectableType::Button)
	{
		return;
	}

	if (selected.button == verticalSyncButton)
	{
		GameSettings& settings = context.settings.GetSettings();

		settings.verticalSyncEnabled = !settings.verticalSyncEnabled;

		verticalSyncButton->GetLabel()->SetString(
			settings.verticalSyncEnabled ? "Vertical synchronization: ON" : "Vertical synchronization: OFF"
		);

		ApplyAndSaveSettings();
	}
	else if (selected.button == blockStyleButton)
	{
		GameSettings& settings = context.settings.GetSettings();

		settings.blockRenderStyle =
			settings.blockRenderStyle == BlockRenderStyle::WithOutline ? BlockRenderStyle::WithoutOutline : BlockRenderStyle::WithOutline;

		const bool withOutline = settings.blockRenderStyle == BlockRenderStyle::WithOutline;

		blockStyleButton->GetLabel()->SetString(
			withOutline ? "Block style: With outline" : "Block style: Without outline"
		);

		ApplyAndSaveSettings();
	}

	context.audioPlayer.Play(Assets::SoundID::MenuItemPressed);
}

void SettingsState::ApplyAndSaveSettings()
{
	GameSettings& settings = context.settings.GetSettings();

	settings.frameRateLimit = static_cast<unsigned int>(frameRateSetting.slider->GetValue());
	settings.soundVolume = static_cast<unsigned int>(soundSetting.slider->GetValue());
	settings.musicVolume = static_cast<unsigned int>(musicSetting.slider->GetValue());

	context.settings.Apply(context);
	context.settings.Save();
}

void SettingsState::UpdateLayout()
{
	const sf::Vector2f viewSize = context.window.getView().getSize();
	rootLayout.Arrange({ 0.f, 0.f }, viewSize);
}

void SettingsState::HandleEvent(const sf::Event& event)
{
	switch (MenuInput::Resolve(event, context.gamepad))
	{
	case MenuInput::Action::Back:    RequestChange(std::make_unique<MainMenuState>(context)); return;
	case MenuInput::Action::Up:      SelectPrevious();          return;
	case MenuInput::Action::Down:    SelectNext();              return;
	case MenuInput::Action::Left:    DecreaseCurrentSlider();   return;
	case MenuInput::Action::Right:   IncreaseCurrentSlider();   return;
	case MenuInput::Action::Confirm: ActivateCurrentElement();  return;
	default:                                                    break;
	}

	if (const auto* moved = event.getIf<sf::Event::MouseMoved>())
	{
		HandlePointer(context.window.mapPixelToCoords(moved->position), false);
	}
	else if (const auto* clicked = event.getIf<sf::Event::MouseButtonPressed>())
	{
		if (clicked->button == sf::Mouse::Button::Left)
		{
			HandlePointer(context.window.mapPixelToCoords(clicked->position), true);
		}
	}
}

void SettingsState::HandlePointer(sf::Vector2f point, bool clicked)
{
	// Dragging a held slider follows the cursor even when it leaves the track.
	const bool dragging = !clicked && sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
	if (dragging &&
		selectedIndex >= 0 &&
		selectedIndex < static_cast<int>(selectableElements.size()) &&
		selectableElements[selectedIndex].type == SelectableType::Slider)
	{
		selectableElements[selectedIndex].slider->SetValueFromPointer(point);
		UpdateSliderLabels();
		ApplyAndSaveSettings();
		return;
	}

	for (std::size_t index = 0; index < selectableElements.size(); index++)
	{
		const SelectableElement& element = selectableElements[index];

		const bool overButton = element.type == SelectableType::Button && element.button->Contains(point);
		const bool overSlider = element.type == SelectableType::Slider && element.slider->Contains(point);
		if (!overButton && !overSlider)
		{
			continue;
		}

		if (static_cast<int>(index) != selectedIndex)
		{
			selectedIndex = static_cast<int>(index);
			UpdateSelection();
		}

		if (clicked && overButton)
		{
			ActivateCurrentElement();
		}
		else if (clicked && overSlider)
		{
			element.slider->SetValueFromPointer(point);
			UpdateSliderLabels();
			ApplyAndSaveSettings();
		}

		return;
	}
}

void SettingsState::Update(float /* deltaTime */)
{
	// Static screen; nothing advances per frame.
}

void SettingsState::Render(sf::RenderTarget& target)
{
	target.draw(backgroundSprite);
	rootLayout.Render(target);
}