#include "LocalizationManager.h"

#include <fstream>
#include <string>

namespace
{
	std::string_view Trim(std::string_view text)
	{
		const auto isSpace = [](char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; };

		while (!text.empty() && isSpace(text.front()))
		{
			text.remove_prefix(1);
		}

		while (!text.empty() && isSpace(text.back()))
		{
			text.remove_suffix(1);
		}

		return text;
	}

	// Turns the catalog's literal "\n" into a real newline. No other escapes.
	std::string Unescape(std::string_view value)
	{
		std::string result;
		result.reserve(value.size());

		for (std::size_t i = 0; i < value.size(); i++)
		{
			if (value[i] == '\\' && i + 1 < value.size() && value[i + 1] == 'n')
			{
				result.push_back('\n');
				i++;
			}
			else
			{
				result.push_back(value[i]);
			}
		}

		return result;
	}
}

bool LocalizationManager::Load(const std::filesystem::path& directory)
{
	std::ifstream file(directory / "en.txt");

	if (!file.is_open())
	{
		return false;
	}

	std::string line;

	while (std::getline(file, line))
	{
		const std::string_view trimmed = Trim(line);

		if (trimmed.empty() || trimmed.front() == '#')
		{
			continue;
		}

		const std::size_t separator = trimmed.find('=');

		if (separator == std::string_view::npos)
		{
			continue;
		}

		const std::string_view key = Trim(trimmed.substr(0, separator));

		if (key.empty())
		{
			continue;
		}

		const std::string value = Unescape(Trim(trimmed.substr(separator + 1)));
		catalog[std::string(key)] = sf::String::fromUtf8(value.begin(), value.end());
	}

	return true;
}

sf::String LocalizationManager::GetText(std::string_view key) const
{
	const auto entry = catalog.find(std::string(key));

	if (entry == catalog.end())
	{
		// Visible on screen, so a missing key gets noticed, but never a crash.
		return "<" + std::string(key) + ">";
	}

	return entry->second;
}

sf::String LocalizationManager::FormatText(std::string_view key, std::string_view token, const sf::String& value) const
{
	return FormatText(key, { { token, value } });
}

sf::String LocalizationManager::FormatText(std::string_view key,
	std::initializer_list<std::pair<std::string_view, sf::String>> replacements) const
{
	sf::String text = GetText(key);

	for (const auto& [token, value] : replacements)
	{
		const sf::String needle = std::string(token);

		std::size_t position = text.find(needle);

		while (position != sf::String::InvalidPos)
		{
			text.replace(position, needle.getSize(), value);
			position = text.find(needle, position + value.getSize());
		}
	}

	return text;
}
