#include "MainMenuScreen.h"

#include <cstddef>
#include <memory>

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Mouse.hpp>

#include "../audio/AudioPlayer.h"
#include "../config/HapticSettings.h"
#include "../core/Context.h"
#include "../input/MenuInput.h"
#include "../input/gamepad/GamepadHaptics.h"
#include "../input/gamepad/HapticPulse.h"
#include "../localization/LocalizationManager.h"
#include "../localization/TextKeys.h"
#include "../resources/Assets.h"
#include "CreditsScreen.h"
#include "GameplayState.h"
#include "MenuShell.h"
#include "SettingsState.h"
#include "StatisticsState.h"

namespace
{
	constexpr unsigned int TitleCharSize = 240;
	constexpr sf::Vector2f TitleCenter{ 960.f, 420.f };

	constexpr unsigned int MenuCharSize = 58;

	// The title-letter landing sound: pitched well down, rising a little per
	// letter across "TESSERA".
	constexpr float LandBasePitch = 0.5f;
	constexpr float LandPitchStep = 0.06f;

	// Navigation ticks: higher when moving right / down, lower left / up.
	constexpr float NavPitchLow = 0.9f;
	constexpr float NavPitchHigh = 1.14f;

	constexpr float SwooshBasePitch = 0.94f;
	constexpr float SwooshPitchStep = 0.04f;

	// How long the exit animation runs before the shell swaps this screen out.
	constexpr float ExitDuration = 0.40f;

	// A couple of ring entries pin their own hue rather than taking the
	// per-slot tetromino colour.
	constexpr sf::Color OptionsColour{ 240, 60, 70 };    // red
	constexpr sf::Color CreditsColour{ 255, 194, 92 };   // warm gold
}

MainMenuScreen::MainMenuScreen(MenuShell& shell, bool animate, std::size_t frontEntry)
	: MenuScreen(shell)
	, title(context.fonts.Get(Assets::FontID::Main), context.localization.GetText(TextKey::MainMenu::Title), TitleCharSize)
	, titleGlow(context.shaders.Get(Assets::ShaderID::NeonDilate), context.shaders.Get(Assets::ShaderID::NeonBlur))
	, entryGlow(context.shaders.Get(Assets::ShaderID::NeonDilate), context.shaders.Get(Assets::ShaderID::NeonBlur))
	, carousel(context.fonts.Get(Assets::FontID::Menu), MenuCharSize, context.textures.Get(Assets::TextureID::UiArrow))
{
	title.SetCenter(TitleCenter);
	title.SetLandCallback([this](std::size_t letter)
		{
			const float t = static_cast<float>(letter) / 6.f;   // 0..1 across "TESSERA"
			context.audioPlayer.Play(Assets::SoundID::TitleButtonDrop,
				LandBasePitch + static_cast<float>(letter) * LandPitchStep);

			// A short pulse that grows from `base` (first letter) to base + grow.
			const HapticSettings::Rumble& base = context.hapticSettings.titleLetterBase;
			const HapticSettings::Rumble& grow = context.hapticSettings.titleLetterGrow;
			context.gamepadHaptics.PulseVibration(
				base.lowMotor + grow.lowMotor * t,
				base.highMotor + grow.highMotor * t,
				base.duration);
		});

	carousel.SetSwooshCallback([this](std::size_t entry)
		{
			context.audioPlayer.Play(Assets::SoundID::MenuItemAppeared,
				SwooshBasePitch + static_cast<float>(entry) * SwooshPitchStep);
			Haptics::Pulse(context.gamepadHaptics, context.hapticSettings.menuEntryFlyIn);
		});

	// Ring order; Achievements is still a disabled placeholder.
	carousel.SetCenter(TitleCenter);
	carousel.AddItem(context.localization.GetText(TextKey::MainMenu::StartGame),
		[this] { this->shell.ExitTo(std::make_unique<GameplayState>(context)); });
	carousel.AddItem(context.localization.GetText(TextKey::MainMenu::Options),
		[this] { this->shell.ExitTo(std::make_unique<SettingsState>(context)); },
		true, OptionsColour);
	carousel.AddItem(context.localization.GetText(TextKey::MainMenu::Records),
		[this] { this->shell.ExitTo(std::make_unique<StatisticsState>(context)); });
	carousel.AddItem(context.localization.GetText(TextKey::MainMenu::Achievements), nullptr, false);
	carousel.AddItem(context.localization.GetText(TextKey::MainMenu::Credits),
		[this]
		{
			this->shell.BeginForward(std::make_unique<CreditsScreen>(this->shell, CreditsColour),
				context.localization.GetText(TextKey::Credits::Title), CreditsColour,
				carousel.FrontEntryCentre(), carousel.FrontEntryHeight(), carousel.CurrentFrontIndex());
		},
		true, CreditsColour);
	carousel.AddItem(context.localization.GetText(TextKey::MainMenu::Quit),
		[this] { context.window.close(); });

	if (!animate)
	{
		title.Skip();
		carousel.Skip();
		carousel.SetFrontImmediate(frontEntry);
		carouselStarted = true;
	}
}

std::size_t MainMenuScreen::CurrentFrontIndex() const
{
	return carousel.CurrentFrontIndex();
}

void MainMenuScreen::PlayActivatePulse()
{
	carousel.PulseActivate();
}

void MainMenuScreen::StartExit()
{
	exiting = true;
	exitTimer = 0.f;
	title.PlayExit();
	carousel.StartExit();
}

bool MainMenuScreen::ExitFinished() const
{
	return exiting && exitTimer >= ExitDuration && title.ExitComplete();
}

sf::Vector2f MainMenuScreen::FrontEntryCentre() const
{
	return carousel.FrontEntryCentre();
}

float MainMenuScreen::FrontEntryHeight() const
{
	return carousel.FrontEntryHeight();
}

void MainMenuScreen::HandleEvent(const sf::Event& event)
{
	// Ignore input once the screen is on its way out.
	if (exiting)
	{
		return;
	}

	// While the build animation plays, any key / button / click skips it.
	if (!carousel.IsReady())
	{
		if (event.is<sf::Event::KeyPressed>()
			|| event.is<sf::Event::MouseButtonPressed>()
			|| event.is<sf::Event::JoystickButtonPressed>())
		{
			title.Skip();
			carousel.Skip();
			carouselStarted = true;
		}
		return;
	}

	const MenuInput::Action action = MenuInput::Resolve(event, context.gamepad);

	// Back (Escape / gamepad circle / B) is deliberately ignored here: the only
	// way out of the game is the "Quit" ring entry.

	switch (action)
	{
	case MenuInput::Action::Left:
	case MenuInput::Action::Up:
		carousel.RotateLeft();
		shell.Backdrop().Push(1.f);
		context.audioPlayer.Play(Assets::SoundID::MenuItemSelected, NavPitchLow);
		return;
	case MenuInput::Action::Right:
	case MenuInput::Action::Down:
		carousel.RotateRight();
		shell.Backdrop().Push(-1.f);
		context.audioPlayer.Play(Assets::SoundID::MenuItemSelected, NavPitchHigh);
		return;
	case MenuInput::Action::Confirm:
		context.audioPlayer.Play(Assets::SoundID::MenuItemPressed);
		carousel.Activate();
		return;
	default:
		break;
	}

	if (const auto* moved = event.getIf<sf::Event::MouseMoved>())
	{
		carousel.PointerMoved(context.window.mapPixelToCoords(moved->position));
	}
	else if (const auto* pressed = event.getIf<sf::Event::MouseButtonPressed>())
	{
		if (pressed->button != sf::Mouse::Button::Left)
		{
			return;
		}

		switch (carousel.PointerPressed(context.window.mapPixelToCoords(pressed->position)))
		{
		case UI::CarouselMenu::PointerHit::RotatedLeft:
			shell.Backdrop().Push(1.f);
			context.audioPlayer.Play(Assets::SoundID::MenuItemSelected, NavPitchLow);
			break;
		case UI::CarouselMenu::PointerHit::RotatedRight:
			shell.Backdrop().Push(-1.f);
			context.audioPlayer.Play(Assets::SoundID::MenuItemSelected, NavPitchHigh);
			break;
		case UI::CarouselMenu::PointerHit::Activated:
			context.audioPlayer.Play(Assets::SoundID::MenuItemPressed);
			break;
		case UI::CarouselMenu::PointerHit::None:
			break;
		}
	}
}

void MainMenuScreen::Update(float deltaTime)
{
	if (exiting)
	{
		exitTimer += deltaTime;
	}

	title.Update(deltaTime);
	titleGlow.Update(deltaTime);
	entryGlow.Update(deltaTime);

	if (!carouselStarted && title.IsFinished())
	{
		carousel.Begin();
		carouselStarted = true;
	}

	carousel.Update(deltaTime);
}

void MainMenuScreen::Render(sf::RenderTarget& target)
{
	carousel.RenderBack(target);
	title.Render(target, &titleGlow);
	carousel.RenderFront(target, &entryGlow);
}

std::optional<sf::Color> MainMenuScreen::LightbarColour() const
{
	if (!carousel.IsReady())
	{
		return std::nullopt;
	}

	return carousel.FrontColour();
}
