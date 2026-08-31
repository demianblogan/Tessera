#include "AudioCategoryPanel.h"

#include <algorithm>
#include <cstdint>
#include <optional>

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Mouse.hpp>

#include "../audio/AudioPlayer.h"
#include "../core/Context.h"
#include "../input/MenuInput.h"
#include "../localization/LocalizationManager.h"
#include "../localization/TextKeys.h"
#include "../resources/Assets.h"
#include "../settings/SettingsManager.h"
#include "../ui/ColourUtils.h"
#include "OptionsSfx.h"

namespace
{
	namespace Sfx = OptionsSfx;

	constexpr sf::FloatRect PanelBounds{ { 700.f, 300.f }, { 1060.f, 452.f } };
	constexpr unsigned int PanelSourceBorder = 28u;
	constexpr sf::Vector2f PanelTargetBorder{ 44.f, 44.f };

	constexpr unsigned int ButtonSize = 46;

	constexpr float RowsTop = PanelBounds.position.y + 64.f;
	constexpr float RowMargin = 82.f;
	constexpr float RowHeight = 96.f;
	constexpr float RowGap = 18.f;
	constexpr float ButtonRowY = PanelBounds.position.y + PanelBounds.size.y - 88.f;
	constexpr float ButtonGap = 108.f;
	constexpr sf::Vector2f ButtonBoxPadding{ 84.f, 52.f };

	constexpr int VolumeSteps = 10;   // 0..100% in 10% steps
	constexpr float FadeSpeed = 9.f;
	constexpr float PreviewOpacity = 0.55f;

	const sf::Color ButtonColour[3] = { { 70, 200, 110 }, { 255, 162, 62 }, { 236, 240, 246 } };
	const sf::Color ButtonDisabled[3] = { { 34, 82, 50 }, { 110, 72, 36 }, { 120, 124, 132 } };
}

AudioCategoryPanel::AudioCategoryPanel(Context& context, sf::Color accent)
	: context(context)
	, accent(accent)
	, frame(context.textures.Get(Assets::TextureID::UiFrame), PanelBounds, PanelSourceBorder, PanelTargetBorder)
	, buttons{ {
		{ context.fonts.Get(Assets::FontID::Main), ButtonSize },
		{ context.fonts.Get(Assets::FontID::Main), ButtonSize },
		{ context.fonts.Get(Assets::FontID::Main), ButtonSize } } }
	, dialog(context.fonts.Get(Assets::FontID::Main))
{
	const LocalizationManager& text = context.localization;
	buttons[ButtonId::Apply].SetText(text.GetText(TextKey::Options::Apply));
	buttons[ButtonId::Reset].SetText(text.GetText(TextKey::Options::Reset));
	buttons[ButtonId::Back].SetText(text.GetText(TextKey::Options::BackButton));
	for (UI::MenuLabel& button : buttons)
	{
		button.SetWaveEnabled(false);
	}

	working = applied = context.settings.GetSettings();
	BuildRows();
	LayOutButtons();
}

bool AudioCategoryPanel::Equal(const GameSettings& a, const GameSettings& b) const
{
	return a.soundVolume == b.soundVolume && a.musicVolume == b.musicVolume;
}

bool AudioCategoryPanel::IsDirty() const { return !Equal(working, applied); }
bool AudioCategoryPanel::IsAtDefaults() const { return Equal(working, GameSettings{}); }

bool AudioCategoryPanel::ButtonEnabled(std::size_t index) const
{
	switch (index)
	{
	case ButtonId::Apply: return IsDirty();
	case ButtonId::Reset: return !IsAtDefaults();
	default:              return true;
	}
}

std::size_t AudioCategoryPanel::FirstEnabledButton() const
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

void AudioCategoryPanel::BuildRows()
{
	const LocalizationManager& text = context.localization;
	const sf::Font& font = context.fonts.Get(Assets::FontID::Main);
	const sf::Texture& arrow = context.textures.Get(Assets::TextureID::CarouselArrow);

	rows.clear();

	auto soundRow = std::make_unique<UI::SliderRow>(font, text.GetText(TextKey::Options::Sound), arrow,
		VolumeSteps, static_cast<int>(working.soundVolume),
		[this](int value) { working.soundVolume = static_cast<unsigned int>(value); });
	soundRowPtr = soundRow.get();
	rows.push_back(std::move(soundRow));

	auto musicRow = std::make_unique<UI::SliderRow>(font, text.GetText(TextKey::Options::Music), arrow,
		VolumeSteps, static_cast<int>(working.musicVolume),
		[this](int value) { working.musicVolume = static_cast<unsigned int>(value); });
	musicRowPtr = musicRow.get();
	rows.push_back(std::move(musicRow));

	for (std::size_t i = 0; i < rows.size(); ++i)
	{
		rows[i]->SetLayout(
			{ PanelBounds.position.x + RowMargin, RowsTop + static_cast<float>(i) * (RowHeight + RowGap) },
			PanelBounds.size.x - 2.f * RowMargin, RowHeight);
	}

	selectedRow = 0;
}

void AudioCategoryPanel::LayOutButtons()
{
	sf::Vector2f labelMax{ 0.f, 0.f };
	for (const UI::MenuLabel& button : buttons)
	{
		labelMax.x = std::max(labelMax.x, button.InkSize().x);
		labelMax.y = std::max(labelMax.y, button.InkSize().y);
	}
	const sf::Vector2f boxSize{ labelMax.x + ButtonBoxPadding.x, labelMax.y + ButtonBoxPadding.y };

	constexpr float count = static_cast<float>(ButtonId::ButtonCount);
	const float totalWidth = count * boxSize.x + (count - 1.f) * ButtonGap;
	float x = PanelBounds.position.x + PanelBounds.size.x * 0.5f - totalWidth * 0.5f;

	for (std::size_t i = 0; i < ButtonId::ButtonCount; ++i)
	{
		buttonPositions[i] = { x + boxSize.x * 0.5f, ButtonRowY };
		buttonBoxes[i] = { { x, ButtonRowY - boxSize.y * 0.5f }, boxSize };
		x += boxSize.x + ButtonGap;
	}
}

void AudioCategoryPanel::SyncRows()
{
	soundRowPtr->SetCurrent(static_cast<int>(working.soundVolume));
	musicRowPtr->SetCurrent(static_cast<int>(working.musicVolume));
}

void AudioCategoryPanel::Open()
{
	working = applied = context.settings.GetSettings();
	BuildRows();
	LayOutButtons();
	focus = Focus::Rows;
	selectedRow = 0;
	selectedButton = ButtonId::Back;
	closeRequested = false;
	active = true;
}

void AudioCategoryPanel::Close()
{
	active = false;
	closeRequested = false;
}

void AudioCategoryPanel::MoveVertical(int direction)
{
	if (focus == Focus::Buttons)
	{
		if (direction < 0)
		{
			focus = Focus::Rows;
			selectedRow = rows.empty() ? 0 : rows.size() - 1;
		}
		return;
	}

	const int next = static_cast<int>(selectedRow) + direction;
	if (next < 0)
	{
		return;
	}
	if (next >= static_cast<int>(rows.size()))
	{
		focus = Focus::Buttons;
		selectedButton = FirstEnabledButton();
		return;
	}
	selectedRow = static_cast<std::size_t>(next);
}

void AudioCategoryPanel::MoveHorizontal(int direction)
{
	AudioPlayer& audio = context.audioPlayer;

	if (focus == Focus::Rows)
	{
		if (selectedRow < rows.size())
		{
			auto* slider = static_cast<UI::SliderRow*>(rows[selectedRow].get());
			const int before = slider->Current();
			slider->Adjust(direction);
			if (slider->Current() != before)
			{
				Sfx::Step(audio, direction);
			}
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
			Sfx::Nav(audio, direction);
			return;
		}
	}
}

void AudioCategoryPanel::ConfirmFocused()
{
	AudioPlayer& audio = context.audioPlayer;

	if (focus == Focus::Rows)
	{
		return;
	}

	switch (selectedButton)
	{
	case ButtonId::Apply: if (ButtonEnabled(ButtonId::Apply)) { DoApply(); Sfx::Apply(audio); } break;
	case ButtonId::Reset: if (ButtonEnabled(ButtonId::Reset)) { DoReset(); Sfx::Reset(audio); } break;
	default:              BackPressed(); break;
	}
}

void AudioCategoryPanel::BackPressed()
{
	if (IsDirty())
	{
		const LocalizationManager& text = context.localization;
		dialog.Show(text.GetText(TextKey::Options::Unsaved),
			text.GetText(TextKey::Common::Yes), text.GetText(TextKey::Common::No));
		Sfx::DialogOpen(context.audioPlayer);
	}
	else
	{
		closeRequested = true;
	}
}

void AudioCategoryPanel::DoApply()
{
	GameSettings& saved = context.settings.GetSettings();
	saved.soundVolume = working.soundVolume;
	saved.musicVolume = working.musicVolume;

	context.settings.Apply(context);
	context.settings.Save();

	applied = saved;

	if (focus == Focus::Buttons && !ButtonEnabled(selectedButton))
	{
		selectedButton = ButtonId::Back;
	}
}

void AudioCategoryPanel::DoReset()
{
	const GameSettings defaults;
	working.soundVolume = defaults.soundVolume;
	working.musicVolume = defaults.musicVolume;
	SyncRows();

	if (focus == Focus::Buttons && !ButtonEnabled(selectedButton))
	{
		selectedButton = FirstEnabledButton();
	}
}

void AudioCategoryPanel::SetVisibility(Visibility visibility, float previewFade)
{
	switch (visibility)
	{
	case Visibility::Open:    targetAlpha = 1.f; break;
	case Visibility::Preview: targetAlpha = PreviewOpacity * std::clamp(previewFade, 0.f, 1.f); break;
	case Visibility::Hidden:  targetAlpha = 0.f; break;
	}

	active = visibility == Visibility::Open;
}

bool AudioCategoryPanel::WantsToStayOpen() const
{
	return dialog.IsOpen();
}

void AudioCategoryPanel::Update(float deltaTime)
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

	for (UI::MenuLabel& button : buttons)
	{
		button.Update(deltaTime);
	}

	for (std::size_t i = 0; i < rows.size(); ++i)
	{
		rows[i]->SetSelected(active && focus == Focus::Rows && i == selectedRow && !dialog.IsOpen());
		rows[i]->Update(deltaTime);
	}
}

void AudioCategoryPanel::Render(sf::RenderTarget& target)
{
	if (alpha > 0.01f)
	{
		const auto a = static_cast<std::uint8_t>(std::clamp(alpha, 0.f, 1.f) * 255.f);
		const float frac = std::clamp(alpha, 0.f, 1.f);

		frame.SetColor(sf::Color(255, 255, 255, a));
		frame.Draw(target);

		for (const std::unique_ptr<UI::OptionRow>& row : rows)
		{
			row->Render(target, alpha);
		}

		for (std::size_t i = 0; i < ButtonId::ButtonCount; ++i)
		{
			const bool enabled = ButtonEnabled(i);
			const bool focused = active && focus == Focus::Buttons && selectedButton == i && !dialog.IsOpen();
			const sf::Color colour = enabled ? ButtonColour[i] : ButtonDisabled[i];

			if (focused)
			{
				for (int band = 3; band >= 1; --band)
				{
					const float inflate = static_cast<float>(band) * 5.f;
					sf::RectangleShape halo({ buttonBoxes[i].size.x + 2.f * inflate, buttonBoxes[i].size.y + 2.f * inflate });
					halo.setOrigin(halo.getSize() * 0.5f);
					halo.setPosition(buttonPositions[i]);
					halo.setFillColor(sf::Color::Transparent);
					halo.setOutlineThickness(3.f);
					halo.setOutlineColor(sf::Color(colour.r, colour.g, colour.b,
						static_cast<std::uint8_t>(frac * (70.f - static_cast<float>(band) * 16.f))));
					target.draw(halo);
				}
			}

			buttons[i].Draw(target, buttonPositions[i], focused ? 1.04f : 1.f,
				focused ? UI::MixToWhite(colour, 0.2f) : colour, alpha);
		}
	}

	dialog.Render(target);
}

bool AudioCategoryPanel::HandleEvent(const sf::Event& event)
{
	if (!active)
	{
		return false;
	}

	const MenuInput::Action action = MenuInput::Resolve(event, context.gamepad);

	if (dialog.IsOpen())
	{
		dialog.Navigate(action);
		if (!dialog.IsOpen())
		{
			Sfx::DialogPick(context.audioPlayer);
		}
		else if (action == MenuInput::Action::Left || action == MenuInput::Action::Right)
		{
			Sfx::Nav(context.audioPlayer, 1);
		}
		return true;
	}

	switch (action)
	{
	case MenuInput::Action::Up:      MoveVertical(-1); Sfx::Nav(context.audioPlayer, -1); return true;
	case MenuInput::Action::Down:    MoveVertical(1);  Sfx::Nav(context.audioPlayer, 1);  return true;
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
			if (ButtonEnabled(i) && buttonBoxes[i].contains(point))
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
					Sfx::Step(context.audioPlayer, 1);
				}
			}
			for (std::size_t i = 0; i < ButtonId::ButtonCount; ++i)
			{
				if (ButtonEnabled(i) && buttonBoxes[i].contains(point))
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
