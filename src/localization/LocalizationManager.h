#pragma once

#include <filesystem>
#include <initializer_list>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

#include <SFML/System/String.hpp>

// Loads a flat `section.key = value` catalog and hands localized text to the
// UI. Only English exists today; the catalog format and the key scheme are
// the part that would be expensive to retrofit, so they go in now. Adding
// languages (a language enum, per-language fonts, a live-switch revision
// counter) is a later, self-contained job.
//
// A missing key is not fatal: GetText returns the key wrapped in guillemets so
// the gap is obvious on screen without crashing.
class LocalizationManager
{
public:
	// Loads `<directory>/en.txt`. Returns false if the file can't be read;
	// whatever parsed before the failure is kept.
	bool Load(const std::filesystem::path& directory);

	[[nodiscard]] sf::String GetText(std::string_view key) const;

	// GetText with `token` (e.g. "{score}") replaced by `value` throughout.
	[[nodiscard]] sf::String FormatText(std::string_view key, std::string_view token, const sf::String& value) const;

	// GetText with several tokens replaced in one pass-through.
	[[nodiscard]] sf::String FormatText(std::string_view key,
		std::initializer_list<std::pair<std::string_view, sf::String>> replacements) const;

private:
	// key -> value, decoded from the catalog's UTF-8.
	std::unordered_map<std::string, sf::String> catalog;
};
