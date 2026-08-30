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
#include "../localization/LocalizationManager.h"
#include "../localization/TextKeys.h"
#include "../resources/Assets.h"
#include "GameplayState.h"
#include "SettingsState.h"
#include "StatisticsState.h"

namespace
{
	constexpr unsigned int TitleCharSize = 200;
	constexpr sf::Vector2f TitleCenter{ 960.f, 430.f };

	constexpr unsigned int MenuCharSize = 58;

	constexpr unsigned int VersionTextSize = 34;
	constexpr sf::Vector2f VersionMargin{ 28.f, 22.f };
}

MainMenuState::MainMenuState(Context& context)
	: State(context.stateMachine)
	, context(context)
	, backgroundSprite(context.textures.Get(Assets::TextureID::GameBackground))
	, backdrop(context.textures.Get(Assets::TextureID::BlockSpritesheetWithOutline))
	, title(context.fonts.Get(Assets::FontID::Main), context.localization.GetText(TextKey::MainMenu::Title), TitleCharSize)
	, titleGlow(context.shaders.Get(Assets::ShaderID::NeonDilate), context.shaders.Get(Assets::ShaderID::NeonBlur))
	, carousel(context.fonts.Get(Assets::FontID::Menu), MenuCharSize, context.textures.Get(Assets::TextureID::UiArrow))
	, versionText(context.fonts.Get(Assets::FontID::Main), std::string(GameVersion::Text), VersionTextSize)
{
	title.SetCenter(TitleCenter);

	versionText.setFillColor(sf::Color(150, 160, 170));
	const sf::FloatRect versionBounds = versionText.getLocalBounds();
	versionText.setOrigin(
		{
			versionBounds.position.x + versionBounds.size.x,
			versionBounds.position.y + versionBounds.size.y
		});

	// Order around the ring: front, right, back, left.
	carousel.SetCenter(TitleCenter);
	carousel.AddItem(context.localization.GetText(TextKey::MainMenu::StartGame),
		[this] { RequestChange(std::make_unique<GameplayState>(this->context)); });
	carousel.AddItem(context.localization.GetText(TextKey::MainMenu::Options),
		[this] { RequestChange(std::make_unique<SettingsState>(this->context)); });
	carousel.AddItem(context.localization.GetText(TextKey::MainMenu::Quit),
		[this] { this->context.window.close(); });
	carousel.AddItem(context.localization.GetText(TextKey::MainMenu::Records),
		[this] { RequestChange(std::make_unique<StatisticsState>(this->context)); });

	context.music.Get(Assets::MusicID::Gameplay).stop();
	context.music.Get(Assets::MusicID::GameOver).stop();

	sf::Music& menuMusic = context.music.Get(Assets::MusicID::MainMenu);
	menuMusic.setLooping(true);
	if (menuMusic.getStatus() != sf::Music::Status::Playing)
	{
		menuMusic.play();
	}
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

	if (action == MenuInput::Action::Back)
	{
		context.window.close();
		return;
	}

	switch (action)
	{
	case MenuInput::Action::Left:
		carousel.RotateLeft();
		backdrop.Push(1.f);
		context.audioPlayer.Restart(Assets::SoundID::MenuItemSelected);
		return;
	case MenuInput::Action::Right:
		carousel.RotateRight();
		backdrop.Push(-1.f);
		context.audioPlayer.Restart(Assets::SoundID::MenuItemSelected);
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
			context.audioPlayer.Restart(Assets::SoundID::MenuItemSelected);
			break;
		case UI::CarouselMenu::PointerHit::RotatedRight:
			backdrop.Push(-1.f);
			context.audioPlayer.Restart(Assets::SoundID::MenuItemSelected);
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
	backdrop.Update(deltaTime);
	sparks.Update(deltaTime);
	title.Update(deltaTime);
	titleGlow.Update(deltaTime);

	if (!carouselStarted && title.IsFinished())
	{
		carousel.Begin();
		carouselStarted = true;
	}

	carousel.Update(deltaTime);
}

void MainMenuState::Render(sf::RenderTarget& target)
{
	target.clear(sf::Color::Black);
	target.draw(backgroundSprite);
	backdrop.Render(target);
	sparks.Render(target);

	carousel.RenderBack(target);
	title.Render(target, &titleGlow);
	carousel.RenderFront(target);

	versionText.setPosition(target.getView().getSize() - VersionMargin);
	target.draw(versionText);
}
