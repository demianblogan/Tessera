#include "PauseMenuScreen.h"

#include <algorithm>

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Mouse.hpp>

#include "../../audio/AudioPlayer.h"
#include "../../core/Context.h"
#include "../../input/MenuInput.h"
#include "../../localization/LocalizationManager.h"
#include "../../localization/TextKeys.h"
#include "../../resources/Assets.h"
#include "../../utils/Easing.h"
#include "../../states/PauseState.h"

namespace
{
	constexpr unsigned int ButtonTextSize = 38;

	// Low in the frame, where the solidified backdrop is darkest and the text
	// reads best. Left edge lines up with the Options column.
	constexpr sf::Vector2f ColumnTopLeft{ 130.f, 560.f };
	constexpr float RowGap = 116.f;

	constexpr float SlideOffX = 1600.f;      // how far left the column parks when hidden
	constexpr float SlideDuration = 0.30f;

	enum Row : std::size_t { Resume = 0, Restart = 1, Options = 2, MainMenu = 3 };

	constexpr sf::Color ResumeColor{ 120, 220, 130 };    // green
	constexpr sf::Color RestartColor{ 90, 200, 255 };    // sky blue
	constexpr sf::Color OptionsColor{ 240, 60, 70 };     // the Options red
	// "Back to Main Menu" keeps the default white.

	// Navigation ticks: higher when moving forward / down, lower backward / up --
	// mirrors MainMenuScreen's NavPitchLow / NavPitchHigh.
	constexpr float NavPitchLow = 0.9f;
	constexpr float NavPitchHigh = 1.14f;

	using Easing::SmoothStep;
}

std::size_t PauseMenuScreen::GetOptionsRow()
{
	return Row::Options;
}

PauseMenuScreen::PauseMenuScreen(PauseState& owner, std::size_t focusRow)
	: MenuScreen(owner)
	, pause(owner)
	, column(context.fonts.Get(Assets::FontID::MenuList), ButtonTextSize,
		context.shaders.Get(Assets::ShaderID::NeonDilate), context.shaders.Get(Assets::ShaderID::NeonBlur))
{
	const LocalizationManager& text = context.localization;

	column.AddButton(text.GetText(TextKey::Pause::Resume), [this] { pause.RequestResume(); }, true, ResumeColor);
	column.AddButton(text.GetText(TextKey::Pause::Restart), [this] { pause.RequestRestart(); }, true, RestartColor);
	column.AddButton(text.GetText(TextKey::Pause::Options),
		[this] { pause.OpenOptions(column.GetEntryCenter(Row::Options), column.GetEntryHeight(Row::Options)); },
		true, OptionsColor);
	column.AddButton(text.GetText(TextKey::Pause::MainMenu), [this] { pause.RequestQuitToMainMenu(); }, true);   // white

	column.SetLayout(ColumnTopLeft, RowGap);
	column.AppearInstantly();

	// Focus the row we return to (Options, when coming back from it) before
	// wiring the selection sound, so rebuilding the screen is silent.
	for (std::size_t i = 0; i < focusRow && i + 1 < column.GetButtonCount(); ++i)
	{
		column.SelectNext();
	}

	column.SetSelectionChangedCallback([this](std::size_t, int direction)
		{
			context.audioPlayer.Restart(Assets::SoundID::MenuItemSelected, direction >= 0 ? NavPitchHigh : NavPitchLow);
		});

	ApplySlide();
}

void PauseMenuScreen::PlayIntro()
{
	hasIntroStarted = true;
	isExiting = false;
}

void PauseMenuScreen::StartExit()
{
	isExiting = true;
}

bool PauseMenuScreen::IsExitFinished() const
{
	return isExiting && slideFraction <= 0.f;
}

std::optional<sf::Color> PauseMenuScreen::GetLightbarColor() const
{
	return PauseState::Accent;
}

void PauseMenuScreen::ApplySlide()
{
	column.SetRenderShift({ (SmoothStep(slideFraction) - 1.f) * SlideOffX, 0.f });
}

void PauseMenuScreen::HandleEvent(const sf::Event& event)
{
	if (!hasIntroStarted || isExiting || slideFraction < 1.f)
	{
		return;   // no input while the column is sliding
	}

	switch (MenuInput::ResolveAction(event, context.gamepad))
	{
	case MenuInput::Action::Up:      column.SelectPrevious(); return;
	case MenuInput::Action::Down:    column.SelectNext();     return;
	case MenuInput::Action::Confirm: column.Activate();       return;
	case MenuInput::Action::Back:    pause.RequestResume();   return;
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

void PauseMenuScreen::Update(float deltaTime)
{
	column.Update(deltaTime);

	if (hasIntroStarted)
	{
		const float step = deltaTime / SlideDuration;
		slideFraction = std::clamp(slideFraction + (isExiting ? -step : step), 0.f, 1.f);
		ApplySlide();
	}
}

void PauseMenuScreen::Render(sf::RenderTarget& target)
{
	column.Render(target);
}
