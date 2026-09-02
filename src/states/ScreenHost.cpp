#include "ScreenHost.h"

#include <optional>

#include <SFML/Graphics/RenderTarget.hpp>

#include "../core/Context.h"
#include "../input/gamepad/GamepadHaptics.h"
#include "../resources/Assets.h"
#include "MenuScreen.h"

namespace
{
	// Forward transition: how long the press pulse plays before the current
	// screen exits and the header starts to rise.
	constexpr float ImpulseLead = 0.16f;

	[[nodiscard]] Haptics::RGBColor ToRgb(sf::Color colour) noexcept
	{
		return { colour.r, colour.g, colour.b };
	}
}

ScreenHost::ScreenHost(Context& context)
	: State(context.stateMachine)
	, context(context)
	, header(context.fonts.Get(Assets::FontID::Menu),
		context.shaders.Get(Assets::ShaderID::NeonDilate),
		context.shaders.Get(Assets::ShaderID::NeonBlur))
{
}

ScreenHost::~ScreenHost()
{
	// Hand the lightbar back to "off" so the next state starts clean.
	context.gamepadHaptics.SetLightbarColor({});
}

void ScreenHost::SetInitialScreen(std::unique_ptr<MenuScreen> initial)
{
	screen = std::move(initial);
}

void ScreenHost::ExitTo(std::unique_ptr<State> state)
{
	RequestChange(std::move(state));
}

void ScreenHost::BeginForward(std::unique_ptr<MenuScreen> next, const sf::String& label, sf::Color colour,
	sf::Vector2f fromCentre, float fromHeight, std::size_t entryIndex)
{
	if (phase != Phase::Steady || onSubScreen || !screen)
	{
		return;
	}

	nextScreen = std::move(next);
	pendingLabel = label;
	pendingColour = colour;
	pendingFromCentre = fromCentre;
	pendingFromHeight = fromHeight;
	returnEntryIndex = entryIndex;

	screen->PlayActivatePulse();
	forwardStarted = false;
	forwardTimer = 0.f;
	phase = Phase::Forward;
}

void ScreenHost::BeginBack()
{
	if (phase != Phase::Steady || !onSubScreen || !screen)
	{
		return;
	}

	screen->StartExit();
	mainRebuilt = false;
	phase = Phase::Back;
}

void ScreenHost::AdvanceTransition(float deltaTime)
{
	switch (phase)
	{
	case Phase::Steady:
		break;

	case Phase::Forward:
		forwardTimer += deltaTime;
		if (!forwardStarted && forwardTimer >= ImpulseLead)
		{
			header.RiseFrom(pendingFromCentre, pendingFromHeight, pendingLabel, pendingColour);
			if (screen)
			{
				screen->StartExit();
			}
			forwardStarted = true;
		}
		if (forwardStarted && screen && screen->ExitFinished())
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
			auto home = BuildHomeScreen(returnEntryIndex);
			if (!HomeDrivesHeader())
			{
				header.SinkTo(home->HeaderReturnCentre(), home->HeaderReturnHeight());
			}
			screen = std::move(home);
			onSubScreen = false;
			mainRebuilt = true;
			OnHomeRebuilt();
		}
		if (mainRebuilt && (header.IsIdle() || header.IsSettled()))
		{
			phase = Phase::Steady;
		}
		break;
	}
}

void ScreenHost::HandleEvent(const sf::Event& event)
{
	if (screen)
	{
		screen->HandleEvent(event);
	}
}

void ScreenHost::Update(float deltaTime)
{
	UpdateBackground(deltaTime);

	if (screen)
	{
		screen->Update(deltaTime);
	}

	header.Update(deltaTime);
	AdvanceTransition(deltaTime);

	if (screen)
	{
		if (const std::optional<sf::Color> colour = screen->LightbarColour())
		{
			context.gamepadHaptics.SetLightbarColor(ToRgb(*colour));
		}
	}
}

void ScreenHost::Render(sf::RenderTarget& target)
{
	RenderBackground(target);

	if (screen)
	{
		screen->Render(target);
	}

	header.Render(target);

	RenderOverlay(target);
}

bool ScreenHost::ShowsCursor() const
{
	return screen ? screen->ShowsCursor() : true;
}
