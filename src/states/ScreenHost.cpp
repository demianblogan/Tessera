#include "ScreenHost.h"

#include <optional>

#include <SFML/Graphics/RenderTarget.hpp>

#include "../core/Context.h"
#include "../input/gamepad/GamepadHaptics.h"
#include "../resources/Assets.h"
#include "../ui/screens/MenuScreen.h"

namespace
{
	// Forward transition: how long the press pulse plays before the current
	// screen exits and the header starts to rise.
	constexpr float ImpulseLead = 0.16f;

	[[nodiscard]] Haptics::RGBColor ToRgb(sf::Color color) noexcept
	{
		return { color.r, color.g, color.b };
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

Context& ScreenHost::GetContext()
{
	return context;
}

void ScreenHost::SetHeaderText(const sf::String& text)
{
	header.SetText(text);
}

void ScreenHost::OnNavigate(float /*direction*/)
{
}

void ScreenHost::BeginPlay()
{
}

bool ScreenHost::HasPersistentHeader() const
{
	return false;
}

void ScreenHost::OnHomeRebuilt()
{
}

void ScreenHost::SetInitialScreen(std::unique_ptr<MenuScreen> initial)
{
	screen = std::move(initial);
}

MenuScreen* ScreenHost::GetCurrentScreen()
{
	return screen.get();
}

UI::MenuHeader& ScreenHost::GetHeader()
{
	return header;
}

void ScreenHost::RenderOverlay(sf::RenderTarget& /*target*/)
{
}

void ScreenHost::BeginForward(std::unique_ptr<MenuScreen> next, const sf::String& label, sf::Color color,
	sf::Vector2f fromCenter, float fromHeight, std::size_t entryIndex)
{
	if (phase != Phase::Steady || isOnSubScreen || !screen)
	{
		return;
	}

	nextScreen = std::move(next);
	pendingLabel = label;
	pendingColor = color;
	pendingFromCenter = fromCenter;
	pendingFromHeight = fromHeight;
	returnEntryIndex = entryIndex;

	screen->PlayActivatePulse();
	hasForwardStarted = false;
	forwardTimer = 0.f;
	phase = Phase::Forward;
}

void ScreenHost::BeginBack()
{
	if (phase != Phase::Steady || !isOnSubScreen || !screen)
	{
		return;
	}

	screen->StartExit();
	hasRebuiltMain = false;
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
		if (!hasForwardStarted && forwardTimer >= ImpulseLead)
		{
			header.RiseFrom(pendingFromCenter, pendingFromHeight, pendingLabel, pendingColor);
			if (screen)
			{
				screen->StartExit();
			}
			hasForwardStarted = true;
		}
		if (hasForwardStarted && screen && screen->IsExitFinished())
		{
			screen = std::move(nextScreen);
			screen->PlayIntro();
			isOnSubScreen = true;
			phase = Phase::Steady;
		}
		break;

	case Phase::Back:
		if (!hasRebuiltMain && screen && screen->IsExitFinished())
		{
			auto home = BuildHomeScreen(returnEntryIndex);
			if (!HasPersistentHeader())
			{
				header.SinkTo(home->GetHeaderReturnCenter(), home->GetHeaderReturnHeight());
			}
			screen = std::move(home);
			isOnSubScreen = false;
			hasRebuiltMain = true;
			OnHomeRebuilt();
		}
		if (hasRebuiltMain && (header.IsIdle() || header.IsSettled()))
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
		if (const std::optional<sf::Color> color = screen->GetLightbarColor(); color.has_value())
		{
			context.gamepadHaptics.SetLightbarColor(ToRgb(*color));
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

bool ScreenHost::IsCursorVisible() const
{
	return screen ? screen->IsCursorVisible() : true;
}
