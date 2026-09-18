#pragma once

#include <array>
#include <cstddef>
#include <string_view>

// The 5 languages Tessera ships with. Values are also used as an index into
// GameSettings persistence, so the order is append-only -- never renumber.
enum class Language
{
	English,
	Spanish,
	German,
	Russian,
	Ukrainian
};

inline constexpr std::size_t LanguageCount = 5;

// Every language in enum order, for anything that needs to iterate (the
// first-run picker, Options > Language).
inline constexpr std::array<Language, LanguageCount> AllLanguages =
{
	Language::English,
	Language::Spanish,
	Language::German,
	Language::Russian,
	Language::Ukrainian
};

// The catalog file name (without ".txt") for a language, and the persisted
// GameSettings value for it. `en` is also the fallback catalog every other
// language is merged over.
[[nodiscard]] constexpr std::string_view LanguageCode(Language language) noexcept
{
	switch (language)
	{
	case Language::English:
		return "en";
	case Language::Spanish:
		return "es";
	case Language::German:
		return "de";
	case Language::Russian:
		return "ru";
	case Language::Ukrainian:
		return "uk";

	default:
		return "en";
	}
}
