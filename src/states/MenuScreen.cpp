#include "MenuScreen.h"

#include "MenuShell.h"

MenuScreen::MenuScreen(MenuShell& shell)
	: shell(shell)
	, context(shell.GetContext())
{
	// No code -- see MenuShell for the shared state screens draw on top of.
}
