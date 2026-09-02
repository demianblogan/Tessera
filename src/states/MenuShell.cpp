#include "MenuShell.h"

#include <string>

#include <SFML/Audio/Music.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

#include "../core/Context.h"
#include "../core/GameVersion.h"
#include "../resources/Assets.h"
#include "MainMenuScreen.h"

namespace
{
	constexpr unsigned int VersionTextSize = 34;
	constexpr sf::Vector2f VersionMargin{ 28.f, 22.f };
}

MenuShell::MenuShell(Context& context)
	: ScreenHost(context)
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

	SetInitialScreen(std::make_unique<MainMenuScreen>(*this));
}

void MenuShell::OnNavigate(float direction)
{
	backdrop.Push(direction);
}

void MenuShell::UpdateBackground(float deltaTime)
{
	aurora.Update(deltaTime);
	backdrop.Update(deltaTime);
	sparks.Update(deltaTime);
}

void MenuShell::RenderBackground(sf::RenderTarget& target)
{
	target.clear(sf::Color::Black);
	target.draw(backgroundSprite);
	aurora.Render(target);
	backdrop.Render(target);
	sparks.Render(target);
}

void MenuShell::RenderOverlay(sf::RenderTarget& target)
{
	versionText.setPosition(target.getView().getSize() - VersionMargin);
	target.draw(versionText);
}

std::unique_ptr<MenuScreen> MenuShell::BuildHomeScreen(std::size_t returnEntryIndex)
{
	return std::make_unique<MainMenuScreen>(*this, false, returnEntryIndex);
}
