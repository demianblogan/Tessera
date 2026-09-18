#pragma once

#include <algorithm>
#include <string>

namespace TimeFormat
{
	// Formats elapsed run time as "M:SS" (no hour rollover -- a Tetris run never gets there).
	// Takes float because callers pass sf::Time::asSeconds() directly, without an intermediate
	// int conversion; negative/fractional input is just clamped and truncated here.
	[[nodiscard]] inline std::string GetTimeString(float seconds)
	{
		const int total = std::max(0, static_cast<int>(seconds));
		const int minutes = total / 60;
		const int rest = total % 60;
		const std::string padding = rest < 10 ? "0" : "";

		return std::to_string(minutes) + ":" + padding + std::to_string(rest);
	}
}
