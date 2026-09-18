#pragma once

#include <SFML/Graphics/Rect.hpp>

#include "../../resources/Assets.h"
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
		Pause,
		Count   // Not a real action -- the number of entries above, for table sizing.
	};

	// Which spritesheet holds this layout's button icons -- xbox_gamepad_layout
	// or ps_gamepad_layout. Layout::Generic has no icon set of its own and
	// falls back to the Xbox sheet; callers are expected to show text instead
	// of calling this for Generic in the first place.
	[[nodiscard]] Assets::TextureID GetAtlasFor(GamepadManager::Layout layout);

	// The rectangle to cut out of GetAtlasFor(layout)'s texture for action's
	// button prompt icon on that layout.
	[[nodiscard]] sf::IntRect GetIconFor(GamepadManager::Layout layout, Action action);
}
