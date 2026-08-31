#include "MenuShell.h"

#include <algorithm>
#include <cstdint>
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

	// The window header a sub-screen sits under (the grown menu entry lands here).
	constexpr unsigned int HeaderTextSize = 110;
	constexpr sf::Vector2f HeaderCentre{ 960.f, 120.f };

	// Seconds for the header to fade fully in or out.
	constexpr float HeaderFadeDuration = 0.22f;

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
	, headerText(context.fonts.Get(Assets::FontID::Menu), "", HeaderTextSize)
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

bool MenuShell::IsTransitioning() const
{
	return phase != Phase::Steady;
}

void MenuShell::BeginForward(std::unique_ptr<MenuScreen> next, const sf::String& label, sf::Color colour)
{
	if (phase != Phase::Steady || onSubScreen || !screen)
	{
		return;
	}

	nextScreen = std::move(next);

	headerColour = colour;
	headerText.setString(label);
	const sf::FloatRect bounds = headerText.getLocalBounds();
	headerText.setOrigin({ bounds.position.x + bounds.size.x * 0.5f, bounds.position.y + bounds.size.y * 0.5f });
	headerText.setPosition(HeaderCentre);

	screen->StartExit();
	phase = Phase::Forward;
}

void MenuShell::BeginBack()
{
	if (phase != Phase::Steady || !onSubScreen || !screen)
	{
		return;
	}

	screen->StartExit();
	mainRebuilt = false;
	phase = Phase::Back;
}

void MenuShell::DriveHeaderAlpha(float target, float deltaTime)
{
	const float step = deltaTime / HeaderFadeDuration;
	if (headerAlpha < target)
	{
		headerAlpha = std::min(target, headerAlpha + step);
	}
	else
	{
		headerAlpha = std::max(target, headerAlpha - step);
	}
}

void MenuShell::AdvanceTransition(float deltaTime)
{
	switch (phase)
	{
	case Phase::Steady:
		DriveHeaderAlpha(onSubScreen ? 1.f : 0.f, deltaTime);
		break;

	case Phase::Forward:
		DriveHeaderAlpha(1.f, deltaTime);
		if (screen && screen->ExitFinished())
		{
			screen = std::move(nextScreen);
			screen->PlayIntro();
			onSubScreen = true;
			phase = Phase::Steady;
		}
		break;

	case Phase::Back:
		if (!mainRebuilt && screen && screen->ExitFinished())
		{
			screen = std::make_unique<MainMenuScreen>(*this, false);
			onSubScreen = false;
			mainRebuilt = true;
		}
		if (mainRebuilt)
		{
			DriveHeaderAlpha(0.f, deltaTime);
			if (headerAlpha <= 0.f)
			{
				phase = Phase::Steady;
			}
		}
		break;
	}
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

	AdvanceTransition(deltaTime);
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

	if (headerAlpha > 0.f)
	{
		const auto alpha = static_cast<std::uint8_t>(std::clamp(headerAlpha, 0.f, 1.f) * 255.f);
		headerText.setFillColor(sf::Color(headerColour.r, headerColour.g, headerColour.b, alpha));
		target.draw(headerText);
	}

	versionText.setPosition(target.getView().getSize() - VersionMargin);
	target.draw(versionText);
}

bool MenuShell::ShowsCursor() const
{
	return screen ? screen->ShowsCursor() : true;
}
