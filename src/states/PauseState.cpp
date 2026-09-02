#include "PauseState.h"

#include <algorithm>
#include <cstdint>
#include <memory>

#include <SFML/Graphics/Glsl.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Shader.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/System/String.hpp>
#include <SFML/Window/Event.hpp>

#include "../audio/AudioPlayer.h"
#include "../core/Context.h"
#include "../display/DisplayManager.h"
#include "../localization/LocalizationManager.h"
#include "../localization/TextKeys.h"
#include "../resources/Assets.h"
#include "GameplayState.h"
#include "MenuShell.h"
#include "PauseMenuScreen.h"

namespace
{
	// "Solidified" look of the frozen frame.
	constexpr float MosaicCellPx = 22.f;
	constexpr float MosaicDarken = 0.58f;
	constexpr float MosaicFrontSoft = 46.f;

	// How long the frame takes to solidify (and, in reverse, to melt back).
	constexpr float SolidifyDuration = 0.32f;

	// Where the "PAUSE" header flies in from / out to: just above the top edge.
	constexpr sf::Vector2f HeaderFrom{ 960.f, -150.f };
	constexpr float HeaderFromHeight = 72.f;
}

PauseState::PauseState(Context& context, std::unique_ptr<sf::RenderTexture> frozenFrame)
	: ScreenHost(context)
	, frozenFrame(std::move(frozenFrame))
{
	SetInitialScreen(std::make_unique<PauseMenuScreen>(*this, 0));
}

PauseState::~PauseState() = default;

void PauseState::RequestResume()
{
	if (resuming)
	{
		return;
	}

	resuming = true;
	context.audioPlayer.Play(Assets::SoundID::MenuItemPressed, 0.9f);
	Header().SinkTo(HeaderFrom, HeaderFromHeight);
	if (MenuScreen* screen = CurrentScreen())
	{
		screen->StartExit();
	}
}

void PauseState::RequestRestart()
{
	context.audioPlayer.Play(Assets::SoundID::MenuItemPressed);
	RequestClear();
	RequestPush(std::make_unique<GameplayState>(context));
}

void PauseState::RequestQuitToMainMenu()
{
	context.audioPlayer.Play(Assets::SoundID::MenuItemPressed);
	RequestClear();
	RequestPush(std::make_unique<MenuShell>(context));
}

void PauseState::HandleEvent(const sf::Event& event)
{
	// Ignore input until the frame has finished solidifying and once Resume is
	// under way.
	if (resuming || reveal < 1.f)
	{
		return;
	}

	ScreenHost::HandleEvent(event);
}

void PauseState::UpdateBackground(float deltaTime)
{
	const float step = deltaTime / SolidifyDuration;

	if (resuming)
	{
		reveal -= step;
		if (reveal <= 0.f)
		{
			reveal = 0.f;
			RequestPop();
		}
		return;
	}

	reveal = std::min(1.f, reveal + step);

	if (reveal >= 1.f && !introRaised)
	{
		introRaised = true;
		Header().RiseFrom(HeaderFrom, HeaderFromHeight,
			context.localization.GetText(TextKey::Pause::Title), Accent);
		if (MenuScreen* screen = CurrentScreen())
		{
			screen->PlayIntro();
		}
	}
}

void PauseState::RenderBackground(sf::RenderTarget& target)
{
	if (!frozenFrame)
	{
		sf::RectangleShape overlay(target.getView().getSize());
		overlay.setFillColor(sf::Color(0, 0, 0, static_cast<std::uint8_t>(190.f * reveal)));
		target.draw(overlay);
		return;
	}

	sf::Shader& mosaic = context.shaders.Get(Assets::ShaderID::Mosaic);
	mosaic.setUniform("texture", sf::Shader::CurrentTexture);
	mosaic.setUniform("resolution", sf::Glsl::Vec2(Display::DisplayManager::VirtualSize));
	mosaic.setUniform("cellPx", MosaicCellPx);
	mosaic.setUniform("darken", MosaicDarken);
	mosaic.setUniform("reveal", reveal);
	mosaic.setUniform("frontSoft", MosaicFrontSoft);

	sf::Sprite frame(frozenFrame->getTexture());
	target.draw(frame, &mosaic);
}

std::unique_ptr<MenuScreen> PauseState::BuildHomeScreen(std::size_t returnEntryIndex)
{
	return std::make_unique<PauseMenuScreen>(*this, returnEntryIndex);
}
