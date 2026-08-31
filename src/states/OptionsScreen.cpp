#include "OptionsScreen.h"

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

namespace
{
	constexpr unsigned int ButtonTextSize = 38;
	constexpr sf::Vector2f ColumnTopLeft{ 240.f, 300.f };
	constexpr float RowGap = 120.f;
}

OptionsScreen::OptionsScreen(MenuShell& shell, sf::Color accent)
	: MenuScreen(shell)
	, accent(accent)
	, column(context.fonts.Get(Assets::FontID::MenuList), ButtonTextSize,
		context.shaders.Get(Assets::ShaderID::NeonDilate), context.shaders.Get(Assets::ShaderID::NeonBlur))
{
	const LocalizationManager& text = context.localization;

	// Phase 1: categories are disabled placeholders; only Back is live.
	column.AddButton(text.GetText(TextKey::Options::Gameplay), nullptr, false);
	column.AddButton(text.GetText(TextKey::Options::Graphics), nullptr, false);
	column.AddButton(text.GetText(TextKey::Options::Audio), nullptr, false);
	column.AddButton(text.GetText(TextKey::Options::Controls), nullptr, false);
	column.AddButton(text.GetText(TextKey::Options::Language), nullptr, false);
	column.AddButton(text.GetText(TextKey::Options::Back), [this] { Leave(); });

	column.SetLayout(ColumnTopLeft, RowGap);
	column.SetSelectionChangedCallback([this](std::size_t)
		{
			context.audioPlayer.Restart(Assets::SoundID::MenuItemSelected);
		});
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

void OptionsScreen::HandleEvent(const sf::Event& event)
{
	if (leaving)
	{
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
}

void OptionsScreen::Render(sf::RenderTarget& target)
{
	column.Render(target);
}
