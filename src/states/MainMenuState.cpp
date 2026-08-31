#include "MainMenuState.h"

#include <memory>
#include <string>

#include <SFML/Audio/Music.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Mouse.hpp>

#include "../audio/AudioPlayer.h"
#include "../core/Context.h"
#include "../core/GameVersion.h"
#include "../input/MenuInput.h"
#include "../input/gamepad/GamepadHaptics.h"
#include "../localization/LocalizationManager.h"
#include "../localization/TextKeys.h"
#include "../resources/Assets.h"
#include "GameplayState.h"
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

	// Haptics. Title letters give a short pulse that grows across the word;
	// each ring entry flying in gives a steady medium one.
	constexpr float LandRumbleBaseLow = 0.12f;
	constexpr float LandRumbleBaseHigh = 0.24f;
	constexpr float LandRumbleGrowLow = 0.40f;
	constexpr float LandRumbleGrowHigh = 0.50f;
	constexpr float LandRumbleDuration = 0.05f;

	constexpr float EntryRumbleLow = 0.28f;
	constexpr float EntryRumbleHigh = 0.42f;
	constexpr float EntryRumbleDuration = 0.06f;

	[[nodiscard]] Haptics::RGBColor ToRgb(sf::Color colour) noexcept
	{
		return { colour.r, colour.g, colour.b };
	}

	constexpr unsigned int VersionTextSize = 34;
	constexpr sf::Vector2f VersionMargin{ 28.f, 22.f };
}

MainMenuState::MainMenuState(Context& context)
	: State(context.stateMachine)
	, context(context)
	, backgroundSprite(context.textures.Get(Assets::TextureID::GameBackground))
	, aurora(context.shaders.Get(Assets::ShaderID::MenuAurora))
	, backdrop(context.textures.Get(Assets::TextureID::BlockSpritesheetWithOutline))
	, title(context.fonts.Get(Assets::FontID::Main), context.localization.GetText(TextKey::MainMenu::Title), TitleCharSize)
	, titleGlow(context.shaders.Get(Assets::ShaderID::NeonDilate), context.shaders.Get(Assets::ShaderID::NeonBlur))
	, entryGlow(context.shaders.Get(Assets::ShaderID::NeonDilate), context.shaders.Get(Assets::ShaderID::NeonBlur))
	, carousel(context.fonts.Get(Assets::FontID::Menu), MenuCharSize, context.textures.Get(Assets::TextureID::UiArrow))
	, versionText(context.fonts.Get(Assets::FontID::Main), std::string(GameVersion::Text), VersionTextSize)
{
	title.SetCenter(TitleCenter);
	title.SetLandCallback([this](std::size_t letter)
		{
			const float t = static_cast<float>(letter) / 6.f;   // 0..1 across "TESSERA"
			this->context.audioPlayer.Play(Assets::SoundID::TitleButtonDrop,
				LandBasePitch + static_cast<float>(letter) * LandPitchStep);
			this->context.gamepadHaptics.PulseVibration(
				LandRumbleBaseLow + LandRumbleGrowLow * t,
				LandRumbleBaseHigh + LandRumbleGrowHigh * t,
				LandRumbleDuration);
		});

	versionText.setFillColor(sf::Color(150, 160, 170));
	const sf::FloatRect versionBounds = versionText.getLocalBounds();
	versionText.setOrigin(
		{
			versionBounds.position.x + versionBounds.size.x,
			versionBounds.position.y + versionBounds.size.y
		});

	carousel.SetSwooshCallback([this](std::size_t entry)
		{
			this->context.audioPlayer.Play(Assets::SoundID::MenuItemAppeared,
				SwooshBasePitch + static_cast<float>(entry) * SwooshPitchStep);
			this->context.gamepadHaptics.PulseVibration(
				EntryRumbleLow, EntryRumbleHigh, EntryRumbleDuration);
		});

	// Ring order; Achievements and Credits are placeholders for now (disabled).
	carousel.SetCenter(TitleCenter);
	carousel.AddItem(context.localization.GetText(TextKey::MainMenu::StartGame),
		[this] { RequestChange(std::make_unique<GameplayState>(this->context)); });
	carousel.AddItem(context.localization.GetText(TextKey::MainMenu::Options),
		[this] { RequestChange(std::make_unique<SettingsState>(this->context)); });
	carousel.AddItem(context.localization.GetText(TextKey::MainMenu::Records),
		[this] { RequestChange(std::make_unique<StatisticsState>(this->context)); });
	carousel.AddItem(context.localization.GetText(TextKey::MainMenu::Achievements), nullptr, false);
	carousel.AddItem(context.localization.GetText(TextKey::MainMenu::Credits), nullptr, false);
	carousel.AddItem(context.localization.GetText(TextKey::MainMenu::Quit),
		[this] { this->context.window.close(); });

	context.music.Get(Assets::MusicID::Gameplay).stop();
	context.music.Get(Assets::MusicID::GameOver).stop();

	sf::Music& menuMusic = context.music.Get(Assets::MusicID::MainMenu);
	menuMusic.setLooping(true);
	if (menuMusic.getStatus() != sf::Music::Status::Playing)
	{
		menuMusic.play();
	}
}

MainMenuState::~MainMenuState()
{
	// Hand the lightbar back to "off" so the next screen starts clean.
	context.gamepadHaptics.SetLightbarColor({});
}

void MainMenuState::HandleEvent(const sf::Event& event)
{
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
		backdrop.Push(1.f);
		context.audioPlayer.Play(Assets::SoundID::MenuItemSelected, NavPitchLow);
		return;
	case MenuInput::Action::Right:
	case MenuInput::Action::Down:
		carousel.RotateRight();
		backdrop.Push(-1.f);
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
			backdrop.Push(1.f);
			context.audioPlayer.Play(Assets::SoundID::MenuItemSelected, NavPitchLow);
			break;
		case UI::CarouselMenu::PointerHit::RotatedRight:
			backdrop.Push(-1.f);
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

void MainMenuState::Update(float deltaTime)
{
	aurora.Update(deltaTime);
	backdrop.Update(deltaTime);
	sparks.Update(deltaTime);
	title.Update(deltaTime);
	titleGlow.Update(deltaTime);
	entryGlow.Update(deltaTime);

	if (!carouselStarted && title.IsFinished())
	{
		carousel.Begin();
		carouselStarted = true;
	}

	carousel.Update(deltaTime);

	// The controller lightbar tracks the selected entry's colour.
	if (carousel.IsReady())
	{
		context.gamepadHaptics.SetLightbarColor(ToRgb(carousel.FrontColour()));
	}
}

void MainMenuState::Render(sf::RenderTarget& target)
{
	target.clear(sf::Color::Black);
	target.draw(backgroundSprite);
	aurora.Render(target);
	backdrop.Render(target);
	sparks.Render(target);

	carousel.RenderBack(target);
	title.Render(target, &titleGlow);
	carousel.RenderFront(target, &entryGlow);

	versionText.setPosition(target.getView().getSize() - VersionMargin);
	target.draw(versionText);
}
