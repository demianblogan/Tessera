#include "MenuShellState.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/View.hpp>

#include "../audio/MusicPlayer.h"
#include "../core/Context.h"
#include "../core/GameVersion.h"
#include "../display/DisplayManager.h"
#include "../resources/Assets.h"
#include "../ui/screens/MainMenuScreen.h"
#include "PlayTransitionState.h"

namespace
{
	constexpr unsigned int VersionTextSize = 34;
	constexpr sf::Vector2f VersionMargin{ 28.f, 22.f };
	const sf::Color VersionTextColor{ 150, 160, 170 };
}

MenuShellState::MenuShellState(Context& context)
	: ScreenHost(context)
	, backgroundSprite(context.textures.Get(Assets::TextureID::MenuBackground))
	, aurora(context.shaders.Get(Assets::ShaderID::MenuAurora))
	, backdrop(context.textures.Get(Assets::TextureID::BlockSpritesheetWithOutline))
	, versionText(context.fonts.Get(Assets::FontID::Main), std::string(GameVersion::Text), VersionTextSize)
{
	versionText.setFillColor(VersionTextColor);
	const sf::FloatRect versionBounds = versionText.getLocalBounds();
	versionText.setOrigin(
		{
			versionBounds.position.x + versionBounds.size.x,
			versionBounds.position.y + versionBounds.size.y
		});

	context.musicPlayer.PlayMainMenu();

	SetInitialScreen(std::make_unique<MainMenuScreen>(*this));
}

namespace
{
	// How long the "Play" press pulse plays on the live menu before it freezes.
	constexpr float PlayPressLead = 0.14f;

	// How long the shell takes to fade up from black on entry.
	constexpr float EnterFadeDuration = 0.28f;
}

void MenuShellState::HandleEvent(const sf::Event& event)
{
	if (isPlayPending)
	{
		return;   // the menu is on its way out
	}

	ScreenHost::HandleEvent(event);
}

void MenuShellState::Update(float deltaTime)
{
	ScreenHost::Update(deltaTime);

	if (enterFade > 0.f)
	{
		enterFade = std::max(0.f, enterFade - deltaTime / EnterFadeDuration);
	}

	if (!isPlayPending)
	{
		return;
	}

	playLead += deltaTime;
	if (playLead < PlayPressLead)
	{
		return;
	}

	isPlayPending = false;

	// Freeze the whole current frame -- ring, title, ambient and all -- so the
	// transition can solidify and lift it away over the arriving gameplay.
	std::unique_ptr<sf::RenderTexture> snapshot;
	auto capture = std::make_unique<sf::RenderTexture>();
	if (capture->resize(sf::Vector2u(Display::VirtualSize)))
	{
		capture->setView(sf::View(sf::FloatRect({ 0.f, 0.f }, Display::VirtualSize)));
		capture->clear(sf::Color::Black);
		Render(*capture);
		capture->display();
		snapshot = std::move(capture);
	}

	RequestChange(std::make_unique<PlayTransitionState>(context, std::move(snapshot)));
}

void MenuShellState::OnNavigate(float direction)
{
	backdrop.Push(direction);
}

void MenuShellState::BeginPlay()
{
	if (isPlayPending || !GetCurrentScreen())
	{
		return;
	}

	isPlayPending = true;
	playLead = 0.f;
	GetCurrentScreen()->PlayActivatePulse();
}

void MenuShellState::UpdateBackground(float deltaTime)
{
	aurora.Update(deltaTime);
	backdrop.Update(deltaTime);
	sparks.Update(deltaTime);
}

void MenuShellState::RenderBackground(sf::RenderTarget& target)
{
	target.clear(sf::Color::Black);
	target.draw(backgroundSprite);
	aurora.Render(target);
	backdrop.Render(target);
	sparks.Render(target);
}

void MenuShellState::RenderOverlay(sf::RenderTarget& target)
{
	versionText.setPosition(target.getView().getSize() - VersionMargin);
	target.draw(versionText);

	if (enterFade > 0.f)
	{
		sf::RectangleShape blackout(target.getView().getSize());
		blackout.setFillColor(sf::Color(0, 0, 0, static_cast<std::uint8_t>(std::clamp(enterFade, 0.f, 1.f) * 255.f)));
		target.draw(blackout);
	}
}

std::unique_ptr<MenuScreen> MenuShellState::BuildHomeScreen(std::size_t returnEntryIndex)
{
	return std::make_unique<MainMenuScreen>(*this, false, returnEntryIndex);
}
