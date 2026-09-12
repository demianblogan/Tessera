#pragma once

#include <SFML/Graphics/Rect.hpp>

#include "../resources/Assets.h"
#include "GamepadManager.h"

// Button-prompt icon lookup for the two gamepad atlases (xbox_gamepad_layout /
// ps_gamepad_layout), shared by the read-only Controls > Gamepad options
// panel and the in-game HUD controls legend (which swaps to icons the moment
// GamepadManager reports the player is using a gamepad). Layout::Generic has
// no icon set -- callers fall back to text for it.
namespace GamepadPrompts
{
	enum class Action
	{
		MoveLeft,
		MoveRight,
		SoftDrop,
		HardDrop,
		RotateClockwise,
		RotateCounterClockwise,
		Hold,
		Pause
	};

	[[nodiscard]] Assets::TextureID AtlasFor(GamepadManager::Layout layout);
	[[nodiscard]] sf::IntRect IconFor(GamepadManager::Layout layout, Action action);
}
