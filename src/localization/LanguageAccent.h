#pragma once

#include <SFML/Graphics/Color.hpp>

#include "Language.h"

// One accent hue per language, used everywhere a language is named on
// screen -- the Options > Language list and the first-run picker -- so a
// player learns to recognise a language by its colour, not just its name.
[[nodiscard]] constexpr sf::Color LanguageAccent(Language language) noexcept
{
	switch (language)
	{
	case Language::English:   return { 90, 200, 255 };    // sky blue
	case Language::Spanish:   return { 255, 150, 70 };     // warm orange
	case Language::German:    return { 255, 205, 60 };     // gold
	case Language::Russian:   return { 235, 90, 100 };     // rose red
	case Language::Ukrainian: return { 110, 220, 140 };    // green
	}

	return sf::Color::White;
}
