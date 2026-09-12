#pragma once

#include <filesystem>
#include <initializer_list>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

#include <SFML/System/String.hpp>

#include "Language.h"

// Loads a flat `section.key = value` catalog and hands localized text to the
// UI. English is always loaded first as the fallback layer; the requested
// language (if not English) is then merged on top, key by key, so a missing
// or not-yet-translated line quietly shows English instead of the raw key.
//
// A key missing from every catalog is not fatal either: GetText returns the
// key wrapped in angle brackets so the gap is obvious on screen without
// crashing.
class LocalizationManager
{
public:
	// Loads `<directory>/en.txt`, then `<directory>/<code>.txt` over it if
	// `language` isn't English. Returns false if even the English catalog
	// can't be read; whatever parsed before a failure is kept.
	bool Load(const std::filesystem::path& directory, Language language = Language::English);

	// Switches to `language`, reloading from the directory last passed to
	// Load(), and bumps Revision(). No-op (no reload, no revision bump) if
	// `language` is already active.
	void SetLanguage(Language language);

	[[nodiscard]] Language GetLanguage() const { return language; }

	// Bumped every time the active language actually changes. Screens that
	// cache localized text compare this against the value they last saw to
	// know they need to refresh.
	[[nodiscard]] unsigned int Revision() const { return revision; }

	[[nodiscard]] sf::String GetText(std::string_view key) const;

	// GetText with `token` (e.g. "{score}") replaced by `value` throughout.
	[[nodiscard]] sf::String FormatText(std::string_view key, std::string_view token, const sf::String& value) const;

	// GetText with several tokens replaced in one pass-through.
	[[nodiscard]] sf::String FormatText(std::string_view key,
		std::initializer_list<std::pair<std::string_view, sf::String>> replacements) const;

private:
	bool LoadCatalogFile(const std::filesystem::path& path);

	// key -> value, decoded from the catalog's UTF-8.
	std::unordered_map<std::string, sf::String> catalog;

	std::filesystem::path directory;
	Language language = Language::English;
	unsigned int revision = 0;
};
