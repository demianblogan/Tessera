#include "PlaceholderScreen.h"

#include <SFML/Window/Event.hpp>

#include "../core/Context.h"
#include "../input/MenuInput.h"
#include "MenuShell.h"

void PlaceholderScreen::HandleEvent(const sf::Event& event)
{
	const MenuInput::Action action = MenuInput::Resolve(event, context.gamepad);
	if (action == MenuInput::Action::Back || action == MenuInput::Action::Confirm)
	{
		shell.BeginBack();
	}
}
