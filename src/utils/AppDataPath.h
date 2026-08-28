#pragma once

#include <filesystem>
#include <string_view>

// Where per-player save/config files live. Used by every manager that persists
// something to disk (settings, high scores, and later achievements / progress).
namespace AppDataPath
{
	// Full path to a per-player file named fileName. Prefers
	// %LOCALAPPDATA%\Alone Bull Company\Tessera\<fileName> -- the standard
	// per-user, non-roaming location Windows recommends for this kind of data.
	// Falls back to a user_data\ folder next to the executable if LOCALAPPDATA
	// can't be read.
	[[nodiscard]] std::filesystem::path Resolve(std::string_view fileName);
}
