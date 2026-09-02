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
#include "../input/MenuInput.h"
#include "../localization/LocalizationManager.h"
#include "../localization/TextKeys.h"
#include "../resources/Assets.h"
#include "GameplayState.h"
#include "MenuShell.h"
#include "OptionsScreen.h"
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
	, confirmDialog(context.fonts.Get(Assets::FontID::Main))
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
	pendingAction = PendingAction::Restart;
	confirmDialog.Show(context.localization.GetText(TextKey::Pause::ConfirmRestart),
		context.localization.GetText(TextKey::Common::Yes),
		context.localization.GetText(TextKey::Common::No));
	context.audioPlayer.Play(Assets::SoundID::MenuItemPressed, 0.85f);
}

void PauseState::RequestQuitToMainMenu()
{
	pendingAction = PendingAction::QuitToMainMenu;
	confirmDialog.Show(context.localization.GetText(TextKey::Pause::ConfirmQuit),
		context.localization.GetText(TextKey::Common::Yes),
		context.localization.GetText(TextKey::Common::No));
	context.audioPlayer.Play(Assets::SoundID::MenuItemPressed, 0.85f);
}

void PauseState::OpenOptions(sf::Vector2f from, float fromHeight)
{
	BeginForward(std::make_unique<OptionsScreen>(*this, OptionsAccent),
		context.localization.GetText(TextKey::Options::Title), OptionsAccent,
		from, fromHeight, PauseMenuScreen::OptionsRow());
}

void PauseState::OnHomeRebuilt()
{
	// Back from Options: bring "PAUSE" down from the top again and slide the
	// pause column back in.
	Header().RiseFrom(HeaderFrom, HeaderFromHeight,
		context.localization.GetText(TextKey::Pause::Title), Accent);
	if (MenuScreen* screen = CurrentScreen())
	{
		screen->PlayIntro();
	}
}

void PauseState::PerformPendingAction()
{
	const PendingAction action = pendingAction;
	pendingAction = PendingAction::None;

	context.audioPlayer.Play(Assets::SoundID::MenuItemPressed);
	RequestClear();

	switch (action)
	{
	case PendingAction::Restart:
		RequestPush(std::make_unique<GameplayState>(context));
		break;
	case PendingAction::QuitToMainMenu:
		RequestPush(std::make_unique<MenuShell>(context));
		break;
	case PendingAction::None:
		break;
	}
}

void PauseState::HandleEvent(const sf::Event& event)
{
	// Ignore input until the frame has finished solidifying and once Resume is
	// under way.
	if (resuming || reveal < 1.f)
	{
		return;
	}

	if (confirmDialog.IsOpen())
	{
		confirmDialog.Navigate(MenuInput::Resolve(event, context.gamepad));
		return;
	}

	ScreenHost::HandleEvent(event);
}

void PauseState::Update(float deltaTime)
{
	confirmDialog.Update(deltaTime);
	if (const std::optional<bool> answer = confirmDialog.TakeResult())
	{
		if (*answer)
		{
			PerformPendingAction();
		}
		else
		{
			pendingAction = PendingAction::None;
			context.audioPlayer.Play(Assets::SoundID::MenuItemSelected, 0.8f);
		}
	}

	ScreenHost::Update(deltaTime);
}

void PauseState::Render(sf::RenderTarget& target)
{
	ScreenHost::Render(target);
	confirmDialog.Render(target);
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
