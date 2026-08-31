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
#include "MenuShell.h"
#include "PlaceholderCategoryPanel.h"

namespace
{
	constexpr unsigned int ButtonTextSize = 38;
	constexpr sf::Vector2f ColumnTopLeft{ 130.f, 300.f };
	constexpr float RowGap = 120.f;

	constexpr float PreviewFadeDuration = 0.18f;

	// Row order in the column.
	enum Row : std::size_t { Gameplay = 0, Graphics = 1, Audio = 2, Controls = 3, Language = 4, Back = 5 };
}

OptionsScreen::OptionsScreen(MenuShell& shell, sf::Color accent)
	: MenuScreen(shell)
	, accent(accent)
	, column(context.fonts.Get(Assets::FontID::MenuList), ButtonTextSize,
		context.shaders.Get(Assets::ShaderID::NeonDilate), context.shaders.Get(Assets::ShaderID::NeonBlur))
{
	const LocalizationManager& text = context.localization;

	column.AddButton(text.GetText(TextKey::Options::Gameplay), nullptr, false);
	column.AddButton(text.GetText(TextKey::Options::Graphics), [this] { OpenCategory(Row::Graphics); }, true);
	column.AddButton(text.GetText(TextKey::Options::Audio), [this] { OpenCategory(Row::Audio); }, true);
	column.AddButton(text.GetText(TextKey::Options::Controls), nullptr, false);
	column.AddButton(text.GetText(TextKey::Options::Language), nullptr, false);
	column.AddButton(text.GetText(TextKey::Options::Back), [this] { Leave(); }, true);

	column.SetLayout(ColumnTopLeft, RowGap);
	column.SetSelectionChangedCallback([this](std::size_t)
		{
			context.audioPlayer.Restart(Assets::SoundID::MenuItemSelected);
		});

	panels[Row::Graphics] = std::make_unique<PlaceholderCategoryPanel>(
		context, text.GetText(TextKey::Options::Graphics), accent);
	panels[Row::Audio] = std::make_unique<PlaceholderCategoryPanel>(
		context, text.GetText(TextKey::Options::Audio), accent);

	previewIndex = Row::Graphics;   // the column starts focused on the first enabled row
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
	context.audioPlayer.Play(Assets::SoundID::MenuItemSelected);
}

void OptionsScreen::HandleEvent(const sf::Event& event)
{
	if (leaving)
	{
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

	if (!openIndex && column.SelectedIndex() != previewIndex)
	{
		previewIndex = column.SelectedIndex();
		previewFade = 0.f;
	}

	previewFade = std::min(1.f, previewFade + deltaTime / PreviewFadeDuration);

	if (openIndex && panels[*openIndex])
	{
		panels[*openIndex]->Update(deltaTime);
	}
	else if (previewIndex < RowCount && panels[previewIndex])
	{
		panels[previewIndex]->Update(deltaTime);
	}
}

void OptionsScreen::Render(sf::RenderTarget& target)
{
	column.Render(target);

	if (openIndex && panels[*openIndex])
	{
		panels[*openIndex]->RenderContent(target);
	}
	else if (previewIndex < RowCount && panels[previewIndex])
	{
		panels[previewIndex]->RenderPreview(target, previewFade);
	}
}
