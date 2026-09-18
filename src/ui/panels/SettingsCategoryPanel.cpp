#include "SettingsCategoryPanel.h"

#include <algorithm>
#include <cstdint>
#include <optional>

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Mouse.hpp>

#include "../../audio/AudioPlayer.h"
#include "../../core/Context.h"
#include "../../input/MenuInput.h"
#include "../../localization/LocalizationManager.h"
#include "../../localization/TextKeys.h"
#include "../../resources/Assets.h"
#include "../../settings/SettingsManager.h"
#include "../ColorUtils.h"
#include "OptionsSfx.h"

namespace
{
	namespace Sfx = OptionsSfx;

	constexpr unsigned int PanelSourceBorder = MenuFrameSourceBorder;
	constexpr sf::Vector2f PanelTargetBorder{ 44.f, 44.f };

	constexpr unsigned int ButtonSize = 46;
	constexpr float ButtonGap = 108.f;
	constexpr sf::Vector2f ButtonBoxPadding{ 84.f, 52.f };

	constexpr float FadeSpeed = 9.f;
	constexpr float PreviewOpacity = 0.55f;

	// Apply green, Reset orange, Back plain -- with a dim disabled variant.
	const sf::Color ButtonColor[3] = { { 70, 200, 110 }, { 255, 162, 62 }, { 236, 240, 246 } };
	const sf::Color ButtonDisabled[3] = { { 34, 82, 50 }, { 110, 72, 36 }, { 120, 124, 132 } };

	// Height of the Apply/Reset/Back row above the panel's bottom edge. Shared
	// (by value, not by code) with GamepadCategoryPanel's Back button row.
	constexpr float BackButtonRowHeight = 86.f;

	// Focused-button halo: a few outlined bands growing outward, each fainter
	// than the last. Shared naming (by value, not by code) with
	// GamepadCategoryPanel's equivalent halo.
	constexpr int FocusHaloBandCount = 3;
	constexpr int FocusHaloInnerBand = 1;
	constexpr float FocusHaloInflateStep = 5.f;
	constexpr float FocusHaloOutlineThickness = 3.f;
	constexpr float FocusHaloAlphaBase = 70.f;
	constexpr float FocusHaloAlphaStep = 16.f;
	constexpr float FocusScale = 1.04f;
	constexpr float FocusMixToWhiteFraction = 0.2f;
}

SettingsCategoryPanel::SettingsCategoryPanel(Context& context, sf::Color accent, sf::FloatRect panelBounds,
	const sf::Texture& frameTexture)
	: context(context)
	, accent(accent)
	, panelBounds(panelBounds)
	, frame(frameTexture, panelBounds, PanelSourceBorder, PanelTargetBorder)
	, buttons{ {
		{ context.fonts.Get(Assets::FontID::Main), ButtonSize },
		{ context.fonts.Get(Assets::FontID::Main), ButtonSize },
		{ context.fonts.Get(Assets::FontID::Main), ButtonSize } } }
	, dialog(context.fonts.Get(Assets::FontID::Main), context.fonts.Get(Assets::FontID::Menu),
		context.textures.Get(Assets::TextureID::UiFrameWarning),
		context.shaders.Get(Assets::ShaderID::NeonDilate), context.shaders.Get(Assets::ShaderID::NeonBlur),
		context.audioPlayer)
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
		labelMax.x = std::max(labelMax.x, button.GetInkSize().x);
		labelMax.y = std::max(labelMax.y, button.GetInkSize().y);
	}

	// Apply/Reset/Back can be much wider in some languages than in English;
	// shrink the whole row together (never below 60%) rather than let it spill
	// past the panel or crowd together.
	constexpr float count = static_cast<float>(ButtonId::ButtonCount);
	constexpr float EdgeMargin = 40.f;
	const float available = panelBounds.size.x - 2.f * EdgeMargin;

	const sf::Vector2f naturalBox{ labelMax.x + ButtonBoxPadding.x, labelMax.y + ButtonBoxPadding.y };
	const float naturalTotal = count * naturalBox.x + (count - 1.f) * ButtonGap;

	buttonScale = (naturalTotal > available && naturalTotal > 0.f)
		? std::max(0.6f, available / naturalTotal)
		: 1.f;

	const sf::Vector2f boxSize{ naturalBox.x * buttonScale, naturalBox.y * buttonScale };
	const float gap = ButtonGap * buttonScale;
	const float totalWidth = count * boxSize.x + (count - 1.f) * gap;
	const float rowY = panelBounds.position.y + panelBounds.size.y - BackButtonRowHeight;
	float x = panelBounds.position.x + panelBounds.size.x * 0.5f - totalWidth * 0.5f;

	for (std::size_t i = 0; i < ButtonId::ButtonCount; ++i)
	{
		buttonPositions[i] = { x + boxSize.x * 0.5f, rowY };
		buttonBoxes[i] = { { x, rowY - boxSize.y * 0.5f }, boxSize };
		x += boxSize.x + gap;
	}
}

bool SettingsCategoryPanel::IsDirty() const { return !IsSettingsEqual(working, applied); }
bool SettingsCategoryPanel::IsAtDefaults() const { return IsSettingsEqual(working, GetDefaultSettings()); }

bool SettingsCategoryPanel::IsCloseRequested() const
{
	return isCloseRequested;
}

sf::FloatRect SettingsCategoryPanel::GetPanelBounds() const
{
	return panelBounds;
}

void SettingsCategoryPanel::RenderExtra(sf::RenderTarget& /*target*/, float /*alpha*/)
{
}

bool SettingsCategoryPanel::IsButtonEnabled(std::size_t index) const
{
	switch (index)
	{
	case ButtonId::Apply: return IsDirty();
	case ButtonId::Reset: return !IsAtDefaults();
	default:              return true;
	}
}

std::size_t SettingsCategoryPanel::GetFirstEnabledButton() const
{
	for (std::size_t i = 0; i < ButtonId::ButtonCount; ++i)
	{
		if (IsButtonEnabled(i))
		{
			return i;
		}
	}
	return ButtonId::Back;
}

std::size_t SettingsCategoryPanel::GetLastEnabledRow() const
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

void SettingsCategoryPanel::RowClicked(std::size_t /*index*/, int direction)
{
	Sfx::Step(context.audioPlayer, direction);
}

void SettingsCategoryPanel::AdjustRowByType(UI::OptionRow& row, int direction)
{
	if (auto* toggle = dynamic_cast<UI::ToggleRow*>(&row))
	{
		toggle->Adjust(direction);
		Sfx::Toggle(context.audioPlayer, toggle->IsOn());
		return;
	}

	if (auto* carousel = dynamic_cast<UI::CarouselRow*>(&row))
	{
		const std::size_t before = carousel->GetCurrent();
		carousel->Adjust(direction);
		if (carousel->GetCurrent() != before)
		{
			Sfx::Step(context.audioPlayer, direction);
		}
		return;
	}

	row.Adjust(direction);
	Sfx::Step(context.audioPlayer, direction);
}

void SettingsCategoryPanel::ActivateRowByType(UI::OptionRow& row)
{
	row.Activate();
	if (auto* toggle = dynamic_cast<UI::ToggleRow*>(&row))
	{
		Sfx::Toggle(context.audioPlayer, toggle->IsOn());
	}
}

void SettingsCategoryPanel::RowClickedByType(UI::OptionRow& row, int direction)
{
	if (auto* toggle = dynamic_cast<UI::ToggleRow*>(&row))
	{
		Sfx::Toggle(context.audioPlayer, toggle->IsOn());
	}
	else
	{
		Sfx::Step(context.audioPlayer, direction);
	}
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
	isCloseRequested = false;
	isActive = true;
}

void SettingsCategoryPanel::Close()
{
	isActive = false;
	isCloseRequested = false;
}

void SettingsCategoryPanel::RefreshText()
{
	const LocalizationManager& text = context.localization;
	buttons[ButtonId::Apply].SetText(text.GetText(TextKey::Options::Apply));
	buttons[ButtonId::Reset].SetText(text.GetText(TextKey::Options::Reset));
	buttons[ButtonId::Back].SetText(text.GetText(TextKey::Options::BackButton));

	BuildRows();
	LayOutButtons();
}

void SettingsCategoryPanel::MoveVertical(int direction)
{
	if (focus == Focus::Buttons)
	{
		if (direction < 0)
		{
			focus = Focus::Rows;
			selectedRow = GetLastEnabledRow();
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
			selectedButton = GetFirstEnabledButton();
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
		if (IsButtonEnabled(static_cast<std::size_t>(index)))
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
	case ButtonId::Apply: if (IsButtonEnabled(ButtonId::Apply)) { DoApply(); Sfx::Apply(audio); } break;
	case ButtonId::Reset: if (IsButtonEnabled(ButtonId::Reset)) { DoReset(); Sfx::Reset(audio); } break;
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
		isCloseRequested = true;
	}
}

void SettingsCategoryPanel::DoApply()
{
	ApplyWorking();
	applied = context.settings.GetSettings();

	if (focus == Focus::Buttons && !IsButtonEnabled(selectedButton))
	{
		selectedButton = ButtonId::Back;
	}
}

void SettingsCategoryPanel::DoReset()
{
	ResetWorking();

	if (focus == Focus::Buttons && !IsButtonEnabled(selectedButton))
	{
		selectedButton = GetFirstEnabledButton();
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

	isActive = visibility == Visibility::Open;
}

bool SettingsCategoryPanel::IsStayOpenRequested() const
{
	return dialog.IsOpen();
}

void SettingsCategoryPanel::Update(float deltaTime)
{
	alpha += (targetAlpha - alpha) * std::min(1.f, deltaTime * FadeSpeed);

	dialog.Update(deltaTime);
	if (const std::optional<bool> answer = dialog.TakeResult(); answer.has_value())
	{
		if (*answer)
		{
			DoApply();
		}
		isCloseRequested = true;
	}

	for (UI::MenuLabel& button : buttons)
	{
		button.Update(deltaTime);
	}

	for (std::size_t i = 0; i < rows.size(); ++i)
	{
		rows[i]->SetSelected(isActive && focus == Focus::Rows && i == selectedRow && !dialog.IsOpen());
		rows[i]->Update(deltaTime);
	}
}

void SettingsCategoryPanel::Render(sf::RenderTarget& target)
{
	if (alpha > 0.01f)
	{
		const auto alphaByte = static_cast<std::uint8_t>(std::clamp(alpha, 0.f, 1.f) * 255.f);
		const float alphaFraction = std::clamp(alpha, 0.f, 1.f);

		frame.SetColor(sf::Color(255, 255, 255, alphaByte));
		frame.Draw(target);

		for (const std::unique_ptr<UI::OptionRow>& row : rows)
		{
			row->Render(target, alpha);
		}

		RenderExtra(target, alpha);

		for (std::size_t i = 0; i < ButtonId::ButtonCount; ++i)
		{
			const bool isButtonOn = IsButtonEnabled(i);
			const bool isFocused = isActive && focus == Focus::Buttons && selectedButton == i && !dialog.IsOpen();
			const sf::Color color = isButtonOn ? ButtonColor[i] : ButtonDisabled[i];

			if (isFocused)
			{
				for (int band = FocusHaloBandCount; band >= FocusHaloInnerBand; --band)
				{
					const float inflate = static_cast<float>(band) * FocusHaloInflateStep;
					sf::RectangleShape halo({ buttonBoxes[i].size.x + 2.f * inflate, buttonBoxes[i].size.y + 2.f * inflate });
					halo.setOrigin(halo.getSize() * 0.5f);
					halo.setPosition(buttonPositions[i]);
					halo.setFillColor(sf::Color::Transparent);
					halo.setOutlineThickness(FocusHaloOutlineThickness);
					halo.setOutlineColor(sf::Color(color.r, color.g, color.b,
						static_cast<std::uint8_t>(alphaFraction * (FocusHaloAlphaBase - static_cast<float>(band) * FocusHaloAlphaStep))));
					target.draw(halo);
				}
			}

			buttons[i].Draw(target, buttonPositions[i], (isFocused ? FocusScale : 1.f) * buttonScale,
				isFocused ? UI::MixToWhite(color, FocusMixToWhiteFraction) : color, alpha);
		}
	}

	dialog.Render(target);
}

bool SettingsCategoryPanel::HandleEvent(const sf::Event& event)
{
	if (!isActive)
	{
		return false;
	}

	const MenuInput::Action action = MenuInput::ResolveAction(event, context.gamepad);

	if (dialog.IsOpen())
	{
		dialog.Navigate(action);   // plays its own nav / press sounds

		if (const auto* moved = event.getIf<sf::Event::MouseMoved>())
		{
			dialog.PointerMoved(context.window.mapPixelToCoords(moved->position));
		}
		else if (const auto* pressed = event.getIf<sf::Event::MouseButtonPressed>())
		{
			if (pressed->button == sf::Mouse::Button::Left)
			{
				dialog.PointerPressed(context.window.mapPixelToCoords(pressed->position));
			}
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
			if (rows[i]->IsEnabled() && rows[i]->GetBounds().contains(point))
			{
				focus = Focus::Rows;
				selectedRow = i;
			}
			rows[i]->HandlePointer(point, false);
		}
		for (std::size_t i = 0; i < ButtonId::ButtonCount; ++i)
		{
			if (IsButtonEnabled(i) && buttonBoxes[i].contains(point))
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
					RowClicked(i, rows[i]->GetHoveredArrow());
				}
			}
			for (std::size_t i = 0; i < ButtonId::ButtonCount; ++i)
			{
				if (IsButtonEnabled(i) && buttonBoxes[i].contains(point))
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
