#include "SettingsCategoryPanel.h"

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

	constexpr unsigned int PanelSourceBorder = 28u;
	constexpr sf::Vector2f PanelTargetBorder{ 44.f, 44.f };

	constexpr unsigned int ButtonSize = 46;
	constexpr float ButtonGap = 108.f;
	constexpr sf::Vector2f ButtonBoxPadding{ 84.f, 52.f };

	constexpr float FadeSpeed = 9.f;
	constexpr float PreviewOpacity = 0.55f;

	// Apply green, Reset orange, Back plain -- with a dim disabled variant.
	const sf::Color ButtonColour[3] = { { 70, 200, 110 }, { 255, 162, 62 }, { 236, 240, 246 } };
	const sf::Color ButtonDisabled[3] = { { 34, 82, 50 }, { 110, 72, 36 }, { 120, 124, 132 } };
}

SettingsCategoryPanel::SettingsCategoryPanel(Context& context, sf::Color accent, sf::FloatRect panelBounds)
	: context(context)
	, accent(accent)
	, panelBounds(panelBounds)
	, frame(context.textures.Get(Assets::TextureID::UiFrame), panelBounds, PanelSourceBorder, PanelTargetBorder)
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
	LayOutButtons();
}

void SettingsCategoryPanel::LayOutRows(float rowsTop, float rowMargin, float rowHeight, float rowGap)
{
	for (std::size_t i = 0; i < rows.size(); ++i)
	{
		rows[i]->SetLayout(
			{ panelBounds.position.x + rowMargin, rowsTop + static_cast<float>(i) * (rowHeight + rowGap) },
			panelBounds.size.x - 2.f * rowMargin, rowHeight);
		rows[i]->SetAccent(accent);
	}
}

void SettingsCategoryPanel::LayOutButtons()
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
	const float rowY = panelBounds.position.y + panelBounds.size.y - 86.f;
	float x = panelBounds.position.x + panelBounds.size.x * 0.5f - totalWidth * 0.5f;

	for (std::size_t i = 0; i < ButtonId::ButtonCount; ++i)
	{
		buttonPositions[i] = { x + boxSize.x * 0.5f, rowY };
		buttonBoxes[i] = { { x, rowY - boxSize.y * 0.5f }, boxSize };
		x += boxSize.x + ButtonGap;
	}
}

bool SettingsCategoryPanel::IsDirty() const { return !SettingsEqual(working, applied); }
bool SettingsCategoryPanel::IsAtDefaults() const { return SettingsEqual(working, DefaultSettings()); }

bool SettingsCategoryPanel::ButtonEnabled(std::size_t index) const
{
	switch (index)
	{
	case ButtonId::Apply: return IsDirty();
	case ButtonId::Reset: return !IsAtDefaults();
	default:              return true;
	}
}

std::size_t SettingsCategoryPanel::FirstEnabledButton() const
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

std::size_t SettingsCategoryPanel::LastEnabledRow() const
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

void SettingsCategoryPanel::AdjustRow(std::size_t index, int direction)
{
	if (index < rows.size())
	{
		rows[index]->Adjust(direction);
		Sfx::Step(context.audioPlayer, direction);
	}
}

void SettingsCategoryPanel::ActivateRow(std::size_t index)
{
	if (index < rows.size())
	{
		rows[index]->Activate();
	}
}

void SettingsCategoryPanel::RowClicked(std::size_t /*index*/)
{
	Sfx::Step(context.audioPlayer, 1);
}

void SettingsCategoryPanel::Open()
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

void SettingsCategoryPanel::Close()
{
	active = false;
	closeRequested = false;
}

void SettingsCategoryPanel::MoveVertical(int direction)
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

void SettingsCategoryPanel::MoveButtons(int direction)
{
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
			Sfx::Nav(context.audioPlayer, direction);
			return;
		}
	}
}

void SettingsCategoryPanel::ConfirmFocused()
{
	AudioPlayer& audio = context.audioPlayer;

	if (focus == Focus::Rows)
	{
		ActivateRow(selectedRow);
		return;
	}

	switch (selectedButton)
	{
	case ButtonId::Apply: if (ButtonEnabled(ButtonId::Apply)) { DoApply(); Sfx::Apply(audio); } break;
	case ButtonId::Reset: if (ButtonEnabled(ButtonId::Reset)) { DoReset(); Sfx::Reset(audio); } break;
	default:              BackPressed(); break;
	}
}

void SettingsCategoryPanel::BackPressed()
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

void SettingsCategoryPanel::DoApply()
{
	ApplyWorking();
	applied = context.settings.GetSettings();

	if (focus == Focus::Buttons && !ButtonEnabled(selectedButton))
	{
		selectedButton = ButtonId::Back;
	}
}

void SettingsCategoryPanel::DoReset()
{
	ResetWorking();

	if (focus == Focus::Buttons && !ButtonEnabled(selectedButton))
	{
		selectedButton = FirstEnabledButton();
	}
}

void SettingsCategoryPanel::SetVisibility(Visibility visibility, float previewFade)
{
	switch (visibility)
	{
	case Visibility::Open:    targetAlpha = 1.f; break;
	case Visibility::Preview: targetAlpha = PreviewOpacity * std::clamp(previewFade, 0.f, 1.f); break;
	case Visibility::Hidden:  targetAlpha = 0.f; break;
	}

	active = visibility == Visibility::Open;
}

bool SettingsCategoryPanel::WantsToStayOpen() const
{
	return dialog.IsOpen();
}

void SettingsCategoryPanel::Update(float deltaTime)
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

void SettingsCategoryPanel::Render(sf::RenderTarget& target)
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

		RenderExtra(target, alpha);

		for (std::size_t i = 0; i < ButtonId::ButtonCount; ++i)
		{
			const bool buttonOn = ButtonEnabled(i);
			const bool focused = active && focus == Focus::Buttons && selectedButton == i && !dialog.IsOpen();
			const sf::Color colour = buttonOn ? ButtonColour[i] : ButtonDisabled[i];

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

bool SettingsCategoryPanel::HandleEvent(const sf::Event& event)
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
	case MenuInput::Action::Left:
		if (focus == Focus::Rows) { AdjustRow(selectedRow, -1); } else { MoveButtons(-1); }
		return true;
	case MenuInput::Action::Right:
		if (focus == Focus::Rows) { AdjustRow(selectedRow, 1); } else { MoveButtons(1); }
		return true;
	case MenuInput::Action::Confirm: ConfirmFocused(); return true;
	case MenuInput::Action::Back:    BackPressed();    return true;
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
					RowClicked(i);
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
