#include "MenuScreen.h"

#include "../../states/ScreenHost.h"

MenuScreen::MenuScreen(ScreenHost& host)
	: host(host)
	, context(host.GetContext())
{
	// No code -- see ScreenHost for the shared state screens draw on top of.
}

void MenuScreen::PlayActivatePulse()
{
}

void MenuScreen::StartExit()
{
}

bool MenuScreen::IsExitFinished() const
{
	return true;
}

void MenuScreen::PlayIntro()
{
}

sf::Vector2f MenuScreen::GetHeaderReturnCenter() const
{
	return DefaultHeaderReturnCenter;
}

float MenuScreen::GetHeaderReturnHeight() const
{
	return DefaultHeaderReturnHeight;
}

std::optional<sf::Color> MenuScreen::GetLightbarColor() const
{
	return std::nullopt;
}

bool MenuScreen::IsCursorVisible() const
{
	return true;
}
