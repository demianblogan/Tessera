#include "MenuShell.h"

#include <optional>
#include <string>

#include <SFML/Audio/Music.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

#include "../core/Context.h"
#include "../core/GameVersion.h"
#include "../input/gamepad/GamepadHaptics.h"
#include "../resources/Assets.h"
#include "MainMenuScreen.h"
#include "MenuScreen.h"

namespace
{
	constexpr unsigned int VersionTextSize = 34;
	constexpr sf::Vector2f VersionMargin{ 28.f, 22.f };

	[[nodiscard]] Haptics::RGBColor ToRgb(sf::Color colour) noexcept
	{
		return { colour.r, colour.g, colour.b };
	}
}

MenuShell::MenuShell(Context& context)
	: State(context.stateMachine)
	, context(context)
	, backgroundSprite(context.textures.Get(Assets::TextureID::GameBackground))
	, aurora(context.shaders.Get(Assets::ShaderID::MenuAurora))
	, backdrop(context.textures.Get(Assets::TextureID::BlockSpritesheetWithOutline))
	, versionText(context.fonts.Get(Assets::FontID::Main), std::string(GameVersion::Text), VersionTextSize)
{
	versionText.setFillColor(sf::Color(150, 160, 170));
	const sf::FloatRect versionBounds = versionText.getLocalBounds();
	versionText.setOrigin(
		{
			versionBounds.position.x + versionBounds.size.x,
			versionBounds.position.y + versionBounds.size.y
		});

	context.music.Get(Assets::MusicID::Gameplay).stop();
	context.music.Get(Assets::MusicID::GameOver).stop();

	sf::Music& menuMusic = context.music.Get(Assets::MusicID::MainMenu);
	menuMusic.setLooping(true);
	if (menuMusic.getStatus() != sf::Music::Status::Playing)
	{
		menuMusic.play();
	}

	screen = std::make_unique<MainMenuScreen>(*this);
}

MenuShell::~MenuShell()
{
	// Hand the lightbar back to "off" so the next state starts clean.
	context.gamepadHaptics.SetLightbarColor({});
}

void MenuShell::ShowScreen(std::unique_ptr<MenuScreen> newScreen)
{
	pendingScreen = std::move(newScreen);
	screenChangePending = true;
}

void MenuShell::ExitTo(std::unique_ptr<State> state)
{
	RequestChange(std::move(state));
}

void MenuShell::ApplyPendingScreen()
{
	if (!screenChangePending)
	{
		return;
	}

	screen = std::move(pendingScreen);
	screenChangePending = false;
}

void MenuShell::HandleEvent(const sf::Event& event)
{
	if (screen)
	{
		screen->HandleEvent(event);
	}

	ApplyPendingScreen();
}

void MenuShell::Update(float deltaTime)
{
	ApplyPendingScreen();

	aurora.Update(deltaTime);
	backdrop.Update(deltaTime);
	sparks.Update(deltaTime);

	if (screen)
	{
		screen->Update(deltaTime);
	}

	ApplyPendingScreen();

	if (screen)
	{
		if (const std::optional<sf::Color> colour = screen->LightbarColour())
		{
			context.gamepadHaptics.SetLightbarColor(ToRgb(*colour));
		}
	}
}

void MenuShell::Render(sf::RenderTarget& target)
{
	target.clear(sf::Color::Black);
	target.draw(backgroundSprite);
	aurora.Render(target);
	backdrop.Render(target);
	sparks.Render(target);

	if (screen)
	{
		screen->Render(target);
	}

	versionText.setPosition(target.getView().getSize() - VersionMargin);
	target.draw(versionText);
}

bool MenuShell::ShowsCursor() const
{
	return screen ? screen->ShowsCursor() : true;
}
