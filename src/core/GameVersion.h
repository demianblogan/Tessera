#pragma once

#include <string_view>

// The single source of truth for the game's version string. Shown in the
// main menu; bump it in step with each release tag.
namespace GameVersion
{
	inline constexpr std::string_view Text = "v1.1.0";
}
