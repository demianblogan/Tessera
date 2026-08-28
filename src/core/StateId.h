#pragma once

// Stable identity for every concrete State. Replaces run-time type checks
// (dynamic_cast) in the render pipeline and gives future code (state stack
// queries, analytics, save/restore) a cheap way to ask "which screen is this"
// without RTTI.
enum class StateId
{
	MainMenu,
	Game,
	Pause,
	GameOver,
	Settings,
	Statistics
};
