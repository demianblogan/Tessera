#include "SettingsState.h"

#include <string>

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Mouse.hpp>

#include <memory>

#include "../audio/AudioPlayer.h"
#include "../core/StateMachine.h"
#include "../input/MenuInput.h"
#include "../localization/LocalizationManager.h"
#include "../localization/TextKeys.h"
#include "../settings/GameSettings.h"
#include "../settings/SettingsManager.h"
#include "../resources/Assets.h"
#include "../ui/Button.h"
#include "../ui/Label.h"
#include "../ui/Slider.h"
#include "../ui/Spacer.h"
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
	constexpr unsigned int TitleSize = 120;
	constexpr unsigned int SectionTitleSize = 60;
	constexpr unsigned int RowTextSize = 40;
	constexpr unsigned int FooterTextSize = 50;
	constexpr float SectionWidth = 900.f;
	constexpr float FrameRateSliderStep = 10.f;
	constexpr float VolumeSliderStep = 1.f;

}

SettingsState::SettingsState(Context& context)
	: State(context.stateMachine)
	, context(context)
	, rootLayout(UI::Layout::Orientation::Vertical)
	, backgroundSprite(context.textures.Get(Assets::TextureID::MenuBackground))
{
	backgroundSprite.setColor(sf::Color(255, 255, 255, 180));

	rows.onSelectionChanged = [this] { this->context.audioPlayer.Restart(Assets::SoundID::MenuItemSelected); };
	rows.onSliderAdjusted = [this](UI::Slider&)
		{
			UpdateSliderLabels();
			ApplyAndSaveSettings();
		};
	rows.onButtonActivated = [this](UI::Button& button) { OnButtonActivated(button); };

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
		auto title = std::make_unique<UI::Label>(context.fonts.Get(Assets::FontID::Main), context.localization.GetText(TextKey::Settings::Title), TitleSize);
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
			context.localization.GetText(TextKey::Settings::FooterReturn),
			FooterTextSize
		);

		label->SetFillColor(sf::Color(180, 180, 180));
		footer->Add(std::move(label));

		rootLayout.Add(std::move(footer));
	}

	UpdateSliderLabels();
	UpdateLayout();
}

void SettingsState::CreateGraphicsSection(UI::Layout& parent)
{
	const auto& font = context.fonts.Get(Assets::FontID::Main);

	auto section = std::make_unique<UI::Layout>(UI::Layout::Orientation::Vertical);
	section->SetWidthPixels(SectionWidth);
	section->SetGap(RowGap);

	// =====================================================
	// Section title
	// =====================================================

	section->SetHorizontalAlignment(UI::Layout::Alignment::Start);

	{
		auto label = std::make_unique<UI::Label>(font, context.localization.GetText(TextKey::Settings::SectionGraphics), SectionTitleSize);
		label->SetFillColor(sf::Color::White);
		section->Add(std::move(label));
	}

	{
		const GameSettings& settings = context.settings.GetSettings();

		verticalSyncButton = &CreateToggleRow(*section,
			context.localization.GetText(settings.verticalSyncEnabled ? TextKey::Settings::VsyncOn : TextKey::Settings::VsyncOff));

		showFpsButton = &CreateToggleRow(*section,
			context.localization.GetText(settings.showFps ? TextKey::Settings::ShowFpsOn : TextKey::Settings::ShowFpsOff));
	}

	CreateSliderRow(
		*section,
		context.localization.GetText(TextKey::Settings::FpsLimit),
		0.f,
		240.f,
		static_cast<float>(context.settings.GetSettings().frameRateLimit),
		FrameRateSliderStep,
		frameRateSetting
	);

	{
		const bool withOutline = context.settings.GetSettings().blockRenderStyle == BlockRenderStyle::WithOutline;

		blockStyleButton = &CreateToggleRow(*section,
			context.localization.GetText(withOutline ? TextKey::Settings::BlockStyleOutline : TextKey::Settings::BlockStyleNoOutline));
	}

	parent.Add(std::move(section));
}

UI::Button& SettingsState::CreateToggleRow(UI::Layout& section, const sf::String& text)
{
	const auto& font = context.fonts.Get(Assets::FontID::Main);

	auto button = std::make_unique<UI::Button>(sf::Vector2f{ ToggleButtonWidth, ToggleButtonHeight });
	button->SetTextAlignment(UI::Button::TextAlignment::Left);
	button->SetLabel(std::make_unique<UI::Label>(font, text, RowTextSize));
	button->SetNormalStyle({ .backgroundColor = sf::Color::Transparent, .textColor = sf::Color::White });
	button->SetSelectedStyle({ .backgroundColor = sf::Color::Transparent, .textColor = sf::Color::Yellow });

	UI::Button& reference = *button;
	rows.AddButton(reference);
	section.Add(std::move(button));

	return reference;
}

void SettingsState::CreateAudioSection(UI::Layout& parent)
{
	const auto& font = context.fonts.Get(Assets::FontID::Main);

	auto section = std::make_unique<UI::Layout>(UI::Layout::Orientation::Vertical);
	section->SetWidthPixels(SectionWidth);
	section->SetGap(RowGap);

	// =====================================================
	// Title
	// =====================================================

	section->SetHorizontalAlignment(UI::Layout::Alignment::Center);

	{
		auto label = std::make_unique<UI::Label>(font, context.localization.GetText(TextKey::Settings::SectionAudio), SectionTitleSize);
		label->SetFillColor(sf::Color::White);
		section->Add(std::move(label));
	}

	section->SetHorizontalAlignment(UI::Layout::Alignment::Start);

	// =====================================================
	// Sliders
	// =====================================================

	CreateSliderRow(
		*section,
		context.localization.GetText(TextKey::Settings::Sounds),
		0.f,
		static_cast<float>(MaxVolumeStep),
		static_cast<float>(context.settings.GetSettings().soundVolume),
		VolumeSliderStep,
		soundSetting
	);

	CreateSliderRow(
		*section,
		context.localization.GetText(TextKey::Settings::Music),
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

	row->Add(std::move(slider));

	// =====================================================
	// Value
	// =====================================================

	auto valueLabel = std::make_unique<UI::Label>(font, std::to_string(static_cast<int>(value)), RowTextSize);
	valueLabel->SetFillColor(sf::Color::White);
	setting.valueLabel = valueLabel.get();
	row->Add(std::move(valueLabel));

	rows.AddSlider(*setting.slider, *setting.nameLabel, *setting.valueLabel);

	parent.Add(std::move(row));
}

sf::String SettingsState::FormatFrameRate(int value) const
{
	return value <= 0
		? context.localization.GetText(TextKey::Settings::FpsUnlimited)
		: sf::String(std::to_string(value));
}

void SettingsState::UpdateSliderLabels()
{
	frameRateSetting.valueLabel->SetString(FormatFrameRate(static_cast<int>(frameRateSetting.slider->GetValue())));

	soundSetting.valueLabel->SetString(std::to_string(static_cast<int>(soundSetting.slider->GetValue())));
	musicSetting.valueLabel->SetString(std::to_string(static_cast<int>(musicSetting.slider->GetValue())));
}

void SettingsState::OnButtonActivated(UI::Button& button)
{
	if (&button == verticalSyncButton)
	{
		ToggleVerticalSync();
	}
	else if (&button == showFpsButton)
	{
		ToggleShowFps();
	}
	else if (&button == blockStyleButton)
	{
		ToggleBlockStyle();
	}

	context.audioPlayer.Play(Assets::SoundID::MenuItemPressed);
}

void SettingsState::ToggleShowFps()
{
	GameSettings& settings = context.settings.GetSettings();

	settings.showFps = !settings.showFps;

	showFpsButton->GetLabel()->SetString(
		context.localization.GetText(settings.showFps ? TextKey::Settings::ShowFpsOn : TextKey::Settings::ShowFpsOff)
	);

	ApplyAndSaveSettings();
}

void SettingsState::ToggleVerticalSync()
{
	GameSettings& settings = context.settings.GetSettings();

	settings.verticalSyncEnabled = !settings.verticalSyncEnabled;

	verticalSyncButton->GetLabel()->SetString(
		context.localization.GetText(settings.verticalSyncEnabled ? TextKey::Settings::VsyncOn : TextKey::Settings::VsyncOff)
	);

	ApplyAndSaveSettings();
}

void SettingsState::ToggleBlockStyle()
{
	GameSettings& settings = context.settings.GetSettings();

	settings.blockRenderStyle = settings.blockRenderStyle == BlockRenderStyle::WithOutline
		? BlockRenderStyle::WithoutOutline
		: BlockRenderStyle::WithOutline;

	const bool withOutline = settings.blockRenderStyle == BlockRenderStyle::WithOutline;

	blockStyleButton->GetLabel()->SetString(
		context.localization.GetText(withOutline ? TextKey::Settings::BlockStyleOutline : TextKey::Settings::BlockStyleNoOutline)
	);

	ApplyAndSaveSettings();
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
	switch (const MenuInput::Action action = MenuInput::Resolve(event, context.gamepad))
	{
	case MenuInput::Action::Back:
		RequestChange(std::make_unique<MainMenuState>(context));
		return;

	case MenuInput::Action::Up:      rows.SelectPrevious(); return;
	case MenuInput::Action::Down:    rows.SelectNext();     return;

	case MenuInput::Action::Left:
	case MenuInput::Action::Right:
		if (rows.CurrentSlider() != nullptr)
		{
			rows.AdjustCurrent(action == MenuInput::Action::Right ? 1 : -1);
			context.audioPlayer.Restart(Assets::SoundID::MenuItemSelected);
		}
		return;

	case MenuInput::Action::Confirm: rows.ActivateCurrent(); return;

	default:
		break;
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

	if (dragging && rows.CurrentSlider() != nullptr)
	{
		rows.DragCurrentTo(point);
		return;
	}

	if (clicked)
	{
		rows.PressAt(point);
	}
	else
	{
		rows.SelectAt(point);
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