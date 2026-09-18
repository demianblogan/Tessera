#pragma once

#include <SFML/Graphics/Rect.hpp>

// Shared frame geometry for the two Controls sub-panels (Keyboard and Gamepad),
// so they sit in exactly the same place, at the same size, with the action list
// starting at the same spot.
namespace ControlsPanel
{
	// Height grew by one row pitch to fit the Hold action added in v1.6.0.
	// Top sits 16px lower than that would otherwise suggest, matching Gameplay/
	// HUD: a language whose "OPTIONS" translation has a tall diacritic
	// (Russian's "Й") pushes the header's visual baseline down a little, and
	// this is one of the panels closest to it. Bottom edge unchanged.
	inline constexpr sf::FloatRect Bounds{ { 600.f, 226.f }, { 1240.f, 830.f } };

	inline constexpr float RowMargin = 96.f;
	inline constexpr float RowsTop = Bounds.position.y + 100.f;

	// Row box height and the gap to the next row -- the pitch both panels step by.
	inline constexpr float RowHeight = 78.f;
	inline constexpr float RowGap = 8.f;
	inline constexpr float RowPitch = RowHeight + RowGap;

	// Where an action label starts, matching UI::OptionRow (row left + 26).
	inline constexpr float LabelX = Bounds.position.x + RowMargin + 26.f;

	// Action-label character size, matching UI::OptionRow's LabelSize.
	inline constexpr unsigned int LabelSize = 46u;
}
