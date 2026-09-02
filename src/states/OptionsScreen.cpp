#include "OptionsScreen.h"

#include <algorithm>

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Mouse.hpp>

#include "../audio/AudioPlayer.h"
#include "../core/Context.h"
#include "../input/MenuInput.h"
#include "../localization/LocalizationManager.h"
#include "../localization/TextKeys.h"
#include "../resources/Assets.h"
#include "AudioCategoryPanel.h"
#include "GraphicsCategoryPanel.h"
#include "KeyboardCategoryPanel.h"
#include "MenuShell.h"

namespace
{
	constexpr unsigned int ButtonTextSize = 38;
	constexpr sf::Vector2f ColumnTopLeft{ 130.f, 300.f };
	constexpr float RowGap = 120.f;

	constexpr float ControlsRowGap = 96.f;   // tighter spacing for the 3-button column
	constexpr sf::Vector2f FlyoutTopLeft{ 470.f, 564.f };
	constexpr float FlyoutDim = 0.42f;

	constexpr float PreviewFadeDuration = 0.18f;
	constexpr float SlideDuration = 0.34f;
	constexpr float ColumnExitShiftX = -1500.f;

	// Row order in the category column.
	enum Row : std::size_t { Gameplay = 0, Graphics = 1, Audio = 2, Controls = 3, Language = 4, Back = 5 };

	constexpr sf::Color GraphicsColour{ 90, 200, 255 };    // sky blue
	constexpr sf::Color AudioColour{ 120, 220, 130 };      // green
	constexpr sf::Color ControlsColour{ 190, 130, 240 };   // violet

	[[nodiscard]] float SmoothStep(float t) noexcept
	{
		t = std::clamp(t, 0.f, 1.f);
		return t * t * (3.f - 2.f * t);
	}

	[[nodiscard]] sf::Vector2f Lerp(sf::Vector2f a, sf::Vector2f b, float t) noexcept
	{
		return { a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t };
	}
}

OptionsScreen::OptionsScreen(MenuShell& shell, sf::Color accent)
	: MenuScreen(shell)
	, accent(accent)
	, column(context.fonts.Get(Assets::FontID::MenuList), ButtonTextSize,
		context.shaders.Get(Assets::ShaderID::NeonDilate), context.shaders.Get(Assets::ShaderID::NeonBlur))
	, controlsColumn(context.fonts.Get(Assets::FontID::MenuList), ButtonTextSize,
		context.shaders.Get(Assets::ShaderID::NeonDilate), context.shaders.Get(Assets::ShaderID::NeonBlur))
{
	const LocalizationManager& text = context.localization;

	column.AddButton(text.GetText(TextKey::Options::Gameplay), nullptr, false);
	column.AddButton(text.GetText(TextKey::Options::Graphics), [this] { OpenCategory(Row::Graphics); }, true, GraphicsColour);
	column.AddButton(text.GetText(TextKey::Options::Audio), [this] { OpenCategory(Row::Audio); }, true, AudioColour);
	column.AddButton(text.GetText(TextKey::Options::Controls), [this] { OpenControls(); }, true, ControlsColour);
	column.AddButton(text.GetText(TextKey::Options::Language), nullptr, false);
	column.AddButton(text.GetText(TextKey::Options::Back), [this] { Leave(); }, true);   // white

	column.SetLayout(ColumnTopLeft, RowGap);
	column.SetSelectionChangedCallback([this](std::size_t)
		{
			context.audioPlayer.Restart(Assets::SoundID::MenuItemSelected);
		});
	column.SetSwooshCallback([this](std::size_t index)
		{
			// Same fly-in swoosh as the main-menu ring, pitched a little higher.
			context.audioPlayer.Play(Assets::SoundID::MenuItemAppeared, 1.02f + 0.05f * static_cast<float>(index));
		});

	controlsColumn.AddButton(text.GetText(TextKey::Options::ControlsKeyboard), [this] { OpenKeyboard(); }, true, ControlsColour);
	controlsColumn.AddButton(text.GetText(TextKey::Options::ControlsGamepad), nullptr, false);
	controlsColumn.AddButton(text.GetText(TextKey::Options::ControlsBack), [this] { CloseControls(); }, true, ControlsColour);
	controlsColumn.SetLayout(ColumnTopLeft, ControlsRowGap);
	controlsColumn.SetSelectionChangedCallback([this](std::size_t)
		{
			context.audioPlayer.Restart(Assets::SoundID::MenuItemSelected);
		});
	controlsColumn.AppearInstantly();

	panels[Row::Graphics] = std::make_unique<GraphicsCategoryPanel>(context, GraphicsColour);
	panels[Row::Audio] = std::make_unique<AudioCategoryPanel>(context, AudioColour);
	keyboardPanel = std::make_unique<KeyboardCategoryPanel>(context, ControlsColour);

	previewIndex = Row::Graphics;   // the column starts focused on the first enabled row

	ApplyColumnShifts();
}

void OptionsScreen::PlayIntro()
{
	column.Begin();
}

void OptionsScreen::StartExit()
{
	column.PlayExit();
}

bool OptionsScreen::ExitFinished() const
{
	return column.IsExitDone();
}

void OptionsScreen::Leave()
{
	if (leaving)
	{
		return;
	}

	leaving = true;
	context.audioPlayer.Play(Assets::SoundID::MenuItemPressed);
	shell.BeginBack();
}

void OptionsScreen::OpenCategory(std::size_t index)
{
	if (index >= RowCount || !panels[index])
	{
		return;
	}

	openIndex = index;
	column.SetCompact(true, index);
	panels[index]->Open();
	context.audioPlayer.Play(Assets::SoundID::MenuItemPressed, 1.05f);
}

void OptionsScreen::CloseCategory()
{
	if (!openIndex)
	{
		return;
	}

	panels[*openIndex]->Close();
	openIndex.reset();
	column.SetCompact(false, 0);
	context.audioPlayer.Play(Assets::SoundID::MenuItemSelected, 0.78f);
}

void OptionsScreen::OpenControls()
{
	if (page != Page::Categories)
	{
		return;
	}

	page = Page::ToControls;
	pageT = 0.f;
	context.audioPlayer.Play(Assets::SoundID::MenuItemPressed, 1.05f);
}

void OptionsScreen::CloseControls()
{
	if (page != Page::Controls || keyboardOpen)
	{
		return;
	}

	page = Page::ToCategories;
	pageT = 0.f;
	context.audioPlayer.Play(Assets::SoundID::MenuItemSelected, 0.78f);
}

void OptionsScreen::OpenKeyboard()
{
	if (page != Page::Controls || keyboardOpen || !keyboardPanel)
	{
		return;
	}

	keyboardOpen = true;
	controlsColumn.SetCompact(true, 0);
	keyboardPanel->Open();
	context.audioPlayer.Play(Assets::SoundID::MenuItemPressed, 1.05f);
}

void OptionsScreen::CloseKeyboard()
{
	if (!keyboardOpen)
	{
		return;
	}

	keyboardPanel->Close();
	keyboardOpen = false;
	controlsColumn.SetCompact(false, 0);
	context.audioPlayer.Play(Assets::SoundID::MenuItemSelected, 0.78f);
}

void OptionsScreen::ApplyColumnShifts()
{
	const bool hoveringControls = column.SelectedIndex() == Row::Controls;
	const sf::Vector2f flyoutShift = FlyoutTopLeft - ColumnTopLeft;

	sf::Vector2f categoryShift{ 0.f, 0.f };
	sf::Vector2f controlsShift = flyoutShift;
	float controlsDim = FlyoutDim;

	switch (page)
	{
	case Page::Categories:
		controlsDim = hoveringControls ? FlyoutDim : 0.f;
		break;
	case Page::ToControls:
	{
		const float e = SmoothStep(pageT);
		categoryShift = { ColumnExitShiftX * e, 0.f };
		controlsShift = Lerp(flyoutShift, { 0.f, 0.f }, e);
		controlsDim = FlyoutDim + (1.f - FlyoutDim) * e;
		break;
	}
	case Page::Controls:
		categoryShift = { ColumnExitShiftX, 0.f };
		controlsShift = { 0.f, 0.f };
		controlsDim = 1.f;
		break;
	case Page::ToCategories:
	{
		const float e = SmoothStep(pageT);
		categoryShift = { ColumnExitShiftX * (1.f - e), 0.f };
		controlsShift = Lerp({ 0.f, 0.f }, flyoutShift, e);
		controlsDim = 1.f - (1.f - FlyoutDim) * e;
		break;
	}
	}

	column.SetRenderShift(categoryShift);
	controlsColumn.SetRenderShift(controlsShift);
	controlsColumn.SetRenderDim(controlsDim);
}

void OptionsScreen::HandleEvent(const sf::Event& event)
{
	if (leaving)
	{
		return;
	}

	if (page == Page::ToControls || page == Page::ToCategories)
	{
		return;   // no input mid-slide
	}

	if (page == Page::Controls)
	{
		if (keyboardOpen)
		{
			OptionsCategoryPanel& panel = *keyboardPanel;
			if (panel.HandleEvent(event))
			{
				return;
			}
			if (MenuInput::Resolve(event, context.gamepad) == MenuInput::Action::Back && !panel.WantsToStayOpen())
			{
				CloseKeyboard();
			}
			return;
		}

		switch (MenuInput::Resolve(event, context.gamepad))
		{
		case MenuInput::Action::Up:      controlsColumn.SelectPrevious(); return;
		case MenuInput::Action::Down:    controlsColumn.SelectNext();     return;
		case MenuInput::Action::Confirm: controlsColumn.Activate();       return;
		case MenuInput::Action::Back:    CloseControls();                 return;
		default:                                                          break;
		}

		if (const auto* moved = event.getIf<sf::Event::MouseMoved>())
		{
			controlsColumn.PointerMoved(context.window.mapPixelToCoords(moved->position));
		}
		else if (const auto* clicked = event.getIf<sf::Event::MouseButtonPressed>())
		{
			if (clicked->button == sf::Mouse::Button::Left)
			{
				controlsColumn.PointerPressed(context.window.mapPixelToCoords(clicked->position));
			}
		}
		return;
	}

	if (openIndex)
	{
		OptionsCategoryPanel& panel = *panels[*openIndex];
		if (panel.HandleEvent(event))
		{
			return;
		}

		if (MenuInput::Resolve(event, context.gamepad) == MenuInput::Action::Back && !panel.WantsToStayOpen())
		{
			CloseCategory();
		}
		return;
	}

	switch (MenuInput::Resolve(event, context.gamepad))
	{
	case MenuInput::Action::Up:      column.SelectPrevious(); return;
	case MenuInput::Action::Down:    column.SelectNext();     return;
	case MenuInput::Action::Confirm: column.Activate();       return;
	case MenuInput::Action::Back:    Leave();                 return;
	default:                                                  break;
	}

	if (const auto* moved = event.getIf<sf::Event::MouseMoved>())
	{
		column.PointerMoved(context.window.mapPixelToCoords(moved->position));
	}
	else if (const auto* clicked = event.getIf<sf::Event::MouseButtonPressed>())
	{
		if (clicked->button == sf::Mouse::Button::Left)
		{
			column.PointerPressed(context.window.mapPixelToCoords(clicked->position));
		}
	}
}

void OptionsScreen::Update(float deltaTime)
{
	column.Update(deltaTime);
	controlsColumn.Update(deltaTime);

	if (page == Page::ToControls || page == Page::ToCategories)
	{
		pageT += deltaTime / SlideDuration;
		if (pageT >= 1.f)
		{
			pageT = 0.f;
			page = (page == Page::ToControls) ? Page::Controls : Page::Categories;
		}
	}

	ApplyColumnShifts();

	const bool categoriesActive = page == Page::Categories;

	if (categoriesActive && !openIndex && column.SelectedIndex() != previewIndex)
	{
		previewIndex = column.SelectedIndex();
		previewFade = 0.f;
	}

	previewFade = std::min(1.f, previewFade + deltaTime / PreviewFadeDuration);

	for (std::size_t i = 0; i < RowCount; ++i)
	{
		if (!panels[i])
		{
			continue;
		}

		OptionsCategoryPanel::Visibility visibility = OptionsCategoryPanel::Visibility::Hidden;
		if (categoriesActive && openIndex && *openIndex == i)
		{
			visibility = OptionsCategoryPanel::Visibility::Open;
		}
		else if (categoriesActive && !openIndex && previewIndex == i)
		{
			visibility = OptionsCategoryPanel::Visibility::Preview;
		}

		panels[i]->SetVisibility(visibility, previewFade);
		panels[i]->Update(deltaTime);
	}

	if (openIndex && panels[*openIndex] && panels[*openIndex]->WantsToClose())
	{
		CloseCategory();
	}

	if (keyboardPanel)
	{
		keyboardPanel->SetVisibility(
			keyboardOpen ? OptionsCategoryPanel::Visibility::Open : OptionsCategoryPanel::Visibility::Hidden, 1.f);
		keyboardPanel->Update(deltaTime);
		if (keyboardOpen && keyboardPanel->WantsToClose())
		{
			CloseKeyboard();
		}
	}
}

void OptionsScreen::Render(sf::RenderTarget& target)
{
	column.Render(target);

	const bool showControls = page != Page::Categories || column.SelectedIndex() == Row::Controls;
	if (showControls)
	{
		controlsColumn.Render(target);
	}

	for (const std::unique_ptr<OptionsCategoryPanel>& panel : panels)
	{
		if (panel)
		{
			panel->Render(target);
		}
	}

	if (keyboardPanel)
	{
		keyboardPanel->Render(target);
	}
}
