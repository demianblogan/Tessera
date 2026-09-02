#include "MainMenuScreen.h"

#include <array>
#include <cstddef>
#include <memory>
#include <string_view>

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
#include "../ui/TetrominoPalette.h"
#include "CreditsScreen.h"
#include "ModeSelectScreen.h"
#include "OptionsScreen.h"
#include "ScreenHost.h"
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
	constexpr sf::Color ModeSelectColour{ 80, 200, 140 };   // emerald
	constexpr sf::Color OptionsColour{ 240, 60, 70 };    // red
	constexpr sf::Color CreditsColour{ 255, 194, 92 };   // warm gold

	// haptics.json keys for the DualSense resting colour, one per ring entry
	// in AddItem order. A missing key falls back to the on-screen hue.
	constexpr std::array<std::string_view, 6> RingLightbarKeys{
		"menu_start_game", "menu_options", "menu_records",
		"menu_achievements", "menu_credits", "menu_quit" };

	// How long each title letter tints the lightbar as it lands.
	constexpr float LetterLightbarDuration = 0.22f;
}

MainMenuScreen::MainMenuScreen(ScreenHost& host, bool animate, std::size_t frontEntry)
	: MenuScreen(host)
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

			// ...and the lightbar snaps to that letter's colour as it lands.
			const sf::Color hue = UI::TetrominoColours[letter % UI::TetrominoColours.size()];
			context.gamepadHaptics.PulseLightbar({ hue.r, hue.g, hue.b }, LetterLightbarDuration);
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
		[this]
		{
			this->host.BeginForward(std::make_unique<ModeSelectScreen>(this->host, ModeSelectColour),
				context.localization.GetText(TextKey::ModeSelect::Title), ModeSelectColour,
				carousel.FrontEntryCentre(), carousel.FrontEntryHeight(), carousel.CurrentFrontIndex());
		},
		true, ModeSelectColour);
	carousel.AddItem(context.localization.GetText(TextKey::MainMenu::Options),
		[this]
		{
			this->host.BeginForward(std::make_unique<OptionsScreen>(this->host, OptionsColour),
				context.localization.GetText(TextKey::Options::Title), OptionsColour,
				carousel.FrontEntryCentre(), carousel.FrontEntryHeight(), carousel.CurrentFrontIndex());
		},
		true, OptionsColour);
	carousel.AddItem(context.localization.GetText(TextKey::MainMenu::Records),
		[this] { this->host.ExitTo(std::make_unique<StatisticsState>(context)); });
	carousel.AddItem(context.localization.GetText(TextKey::MainMenu::Achievements), nullptr, false);
	carousel.AddItem(context.localization.GetText(TextKey::MainMenu::Credits),
		[this]
		{
			this->host.BeginForward(std::make_unique<CreditsScreen>(this->host, CreditsColour),
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

sf::Vector2f MainMenuScreen::HeaderReturnCentre() const
{
	return carousel.FrontEntryCentre();
}

float MainMenuScreen::HeaderReturnHeight() const
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
		host.OnNavigate(1.f);
		context.audioPlayer.Play(Assets::SoundID::MenuItemSelected, NavPitchLow);
		return;
	case MenuInput::Action::Right:
	case MenuInput::Action::Down:
		carousel.RotateRight();
		host.OnNavigate(-1.f);
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
			host.OnNavigate(1.f);
			context.audioPlayer.Play(Assets::SoundID::MenuItemSelected, NavPitchLow);
			break;
		case UI::CarouselMenu::PointerHit::RotatedRight:
			host.OnNavigate(-1.f);
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

	// The on-screen hue is the fallback; haptics.json can override per entry.
	const sf::Color screenHue = carousel.FrontColour();
	const HapticSettings::Colour fallback{ screenHue.r, screenHue.g, screenHue.b };
	const std::size_t index = carousel.CurrentFrontIndex();
	const HapticSettings::Colour hue = index < RingLightbarKeys.size()
		? context.hapticSettings.LightbarFor(RingLightbarKeys[index], fallback)
		: fallback;
	return sf::Color(hue.r, hue.g, hue.b);
}
