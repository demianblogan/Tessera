#include "MenuScreen.h"

#include "ScreenHost.h"

MenuScreen::MenuScreen(ScreenHost& host)
	: host(host)
	, context(host.GetContext())
{
	// No code -- see ScreenHost for the shared state screens draw on top of.
}
