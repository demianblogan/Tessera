#include "MainMenuScreen.h"

#include <array>
#include <cstddef>
#include <memory>
#include <string_view>

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Mouse.hpp>

#include "../../audio/AudioPlayer.h"
#include "../../core/Context.h"
#include "../../haptics/HapticSettings.h"
#include "../../input/MenuInput.h"
#include "../../input/gamepad/GamepadHaptics.h"
#include "../../input/gamepad/HapticPulse.h"
#include "../../localization/LocalizationManager.h"
#include "../../localization/TextKeys.h"
#include "../../resources/Assets.h"
#include "../TetrominoPalette.h"
#include "CreditsScreen.h"
#include "OptionsScreen.h"
#include "RecordsScreen.h"
#include "../../states/ScreenHost.h"

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

	// Every ring entry pins its own hue -- the per-slot tetromino fallback would
	// otherwise hand Quit the same red as Options now that the ring has 5 items.
	constexpr sf::Color PlayColor{ 80, 200, 140 };      // emerald
	constexpr sf::Color OptionsColor{ 240, 60, 70 };    // red
	constexpr sf::Color RecordsColor{ 180, 60, 240 };   // purple
	constexpr sf::Color CreditsColor{ 255, 194, 92 };   // warm gold
	constexpr sf::Color QuitColor{ 70, 110, 240 };      // blue

	// haptics.json keys for the DualSense resting color, one per ring entry
	// in AddItem order. A missing key falls back to the on-screen hue.
	constexpr std::array<std::string_view, 5> RingLightbarKeys{
		"menu_play", "menu_options", "menu_records", "menu_credits", "menu_quit" };

	// How long each title letter tints the lightbar as it lands.
	constexpr float LetterLightbarDuration = 0.22f;

	// "TESSERA" is 7 letters; the land callback divides by (letter count - 1)
	// so the 0..1 sweep below lands exactly on 1.f at the final letter.
	constexpr float TitleLetterCount = 7.f;
}

MainMenuScreen::MainMenuScreen(ScreenHost& host, bool isAnimated, std::size_t frontEntry)
	: MenuScreen(host)
	, title(context.fonts.Get(Assets::FontID::Main), context.localization.GetText(TextKey::MainMenu::Title), TitleCharSize)
	, titleGlow(context.shaders.Get(Assets::ShaderID::NeonDilate), context.shaders.Get(Assets::ShaderID::NeonBlur))
	, entryGlow(context.shaders.Get(Assets::ShaderID::NeonDilate), context.shaders.Get(Assets::ShaderID::NeonBlur))
	, carousel(context.fonts.Get(Assets::FontID::Menu), MenuCharSize, context.textures.Get(Assets::TextureID::UiArrow))
{
	title.SetCenter(TitleCenter);
	title.SetLandCallback([this](std::size_t letter)
		{
			const float letterFraction = static_cast<float>(letter) / (TitleLetterCount - 1.f);   // 0..1 across "TESSERA"
			context.audioPlayer.Play(Assets::SoundID::TitleButtonDrop,
				LandBasePitch + static_cast<float>(letter) * LandPitchStep);

			// A short pulse that grows from `base` (first letter) to base + grow.
			const HapticSettings::Rumble& base = context.hapticSettings.titleLetterBase;
			const HapticSettings::Rumble& grow = context.hapticSettings.titleLetterGrow;
			context.gamepadHaptics.PulseVibration(
				base.lowMotor + grow.lowMotor * letterFraction,
				base.highMotor + grow.highMotor * letterFraction,
				base.duration + grow.duration * letterFraction);

			// ...and the lightbar snaps to that letter's color as it lands.
			const sf::Color hue = UI::TetrominoColors[letter % UI::TetrominoColors.size()];
			context.gamepadHaptics.PulseLightbar({ hue.r, hue.g, hue.b }, LetterLightbarDuration);
		});

	carousel.SetSwooshCallback([this](std::size_t entry)
		{
			context.audioPlayer.Play(Assets::SoundID::MenuItemAppeared,
				SwooshBasePitch + static_cast<float>(entry) * SwooshPitchStep);
			Haptics::TriggerPulse(context.gamepadHaptics, context.hapticSettings.menuEntryFlyIn);
		});

	// Ring order: Play / Options / Records / Credits / Quit.
	carousel.SetCenter(TitleCenter);
	carousel.AddItem(context.localization.GetText(TextKey::MainMenu::Play),
		[this] { this->host.BeginPlay(); },
		true, PlayColor);
	carousel.AddItem(context.localization.GetText(TextKey::MainMenu::Options),
		[this]
		{
			this->host.BeginForward(std::make_unique<OptionsScreen>(this->host, OptionsColor),
				context.localization.GetText(TextKey::Options::Title), OptionsColor,
				carousel.GetFrontEntryCenter(), carousel.GetFrontEntryHeight(), carousel.GetCurrentFrontIndex());
		},
		true, OptionsColor);
	carousel.AddItem(context.localization.GetText(TextKey::MainMenu::Records),
		[this]
		{
			this->host.BeginForward(std::make_unique<RecordsScreen>(this->host, RecordsColor),
				context.localization.GetText(TextKey::Records::Title), RecordsColor,
				carousel.GetFrontEntryCenter(), carousel.GetFrontEntryHeight(), carousel.GetCurrentFrontIndex());
		},
		true, RecordsColor);
	carousel.AddItem(context.localization.GetText(TextKey::MainMenu::Credits),
		[this]
		{
			this->host.BeginForward(std::make_unique<CreditsScreen>(this->host, CreditsColor),
				context.localization.GetText(TextKey::Credits::Title), CreditsColor,
				carousel.GetFrontEntryCenter(), carousel.GetFrontEntryHeight(), carousel.GetCurrentFrontIndex());
		},
		true, CreditsColor);
	carousel.AddItem(context.localization.GetText(TextKey::MainMenu::Quit),
		[this] { context.window.close(); },
		true, QuitColor);

	if (!isAnimated)
	{
		title.Skip();
		carousel.Skip();
		carousel.SetFrontImmediate(frontEntry);
		hasCarouselStarted = true;
	}
}

std::size_t MainMenuScreen::GetCurrentFrontIndex() const
{
	return carousel.GetCurrentFrontIndex();
}

void MainMenuScreen::PlayActivatePulse()
{
	carousel.PulseActivate();
}

void MainMenuScreen::StartExit()
{
	isExiting = true;
	exitTimer = 0.f;
	title.PlayExit();
	carousel.StartExit();
}

bool MainMenuScreen::IsExitFinished() const
{
	return isExiting && exitTimer >= ExitDuration && title.IsExitComplete();
}

sf::Vector2f MainMenuScreen::GetHeaderReturnCenter() const
{
	return carousel.GetFrontEntryCenter();
}

float MainMenuScreen::GetHeaderReturnHeight() const
{
	return carousel.GetFrontEntryHeight();
}

void MainMenuScreen::HandleEvent(const sf::Event& event)
{
	// Ignore input once the screen is on its way out.
	if (isExiting)
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
			hasCarouselStarted = true;
		}
		return;
	}

	const MenuInput::Action action = MenuInput::ResolveAction(event, context.gamepad);

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
	if (isExiting)
	{
		exitTimer += deltaTime;
	}

	title.Update(deltaTime);
	titleGlow.Update(deltaTime);
	entryGlow.Update(deltaTime);

	if (!hasCarouselStarted && title.IsFinished())
	{
		carousel.Begin();
		hasCarouselStarted = true;
	}

	carousel.Update(deltaTime);
}

void MainMenuScreen::Render(sf::RenderTarget& target)
{
	carousel.RenderBack(target);
	title.Render(target, &titleGlow);
	carousel.RenderFront(target, &entryGlow);
}

bool MainMenuScreen::IsCursorVisible() const
{
	return carousel.IsReady();
}

std::optional<sf::Color> MainMenuScreen::GetLightbarColor() const
{
	if (!carousel.IsReady())
	{
		return std::nullopt;
	}

	// The on-screen hue is the fallback; haptics.json can override per entry.
	const sf::Color screenHue = carousel.GetFrontColor();
	const HapticSettings::Color fallback{ screenHue.r, screenHue.g, screenHue.b };
	const std::size_t index = carousel.GetCurrentFrontIndex();
	const HapticSettings::Color hue = index < RingLightbarKeys.size()
		? context.hapticSettings.LightbarFor(RingLightbarKeys[index], fallback)
		: fallback;
	return sf::Color(hue.r, hue.g, hue.b);
}
