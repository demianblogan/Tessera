#include "HapticSettings.h"

#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <string_view>
#include <utility>

#include <nlohmann/json.hpp>

namespace
{
	using Json = nlohmann::json;

	// A motor strength never exceeds 1; a fade can't be negative.
	[[nodiscard]] float ClampMotor(float value) noexcept
	{
		return std::clamp(value, 0.f, 1.f);
	}

	[[nodiscard]] float ClampSeconds(float value) noexcept
	{
		return std::max(value, 0.f);
	}

	// Overwrites `rumble` in place from an object like
	// { "low": 0.2, "high": 0.4, "duration": 0.06 }; every field is optional.
	void ReadRumble(const Json& parent, std::string_view key, HapticSettings::Rumble& rumble)
	{
		const auto object = parent.find(key);
		if (object == parent.end() || !object->is_object())
		{
			return;
		}

		if (const auto low = object->find("low"); low != object->end() && low->is_number())
		{
			rumble.lowMotor = ClampMotor(low->get<float>());
		}
		if (const auto high = object->find("high"); high != object->end() && high->is_number())
		{
			rumble.highMotor = ClampMotor(high->get<float>());
		}
		if (const auto duration = object->find("duration"); duration != object->end() && duration->is_number())
		{
			rumble.duration = ClampSeconds(duration->get<float>());
		}
	}

	// Reads a colour written as [r, g, b] (0..255 each).
	void ReadColour(const Json& parent, std::string_view key, HapticSettings::Colour& colour)
	{
		const auto array = parent.find(key);
		if (array == parent.end() || !array->is_array() || array->size() != 3)
		{
			return;
		}

		std::array<std::uint8_t, 3> channels = { colour.r, colour.g, colour.b };
		for (std::size_t i = 0; i < 3; ++i)
		{
			if ((*array)[i].is_number())
			{
				channels[i] = static_cast<std::uint8_t>(
					std::clamp((*array)[i].get<int>(), 0, 255));
			}
		}
		colour = { channels[0], channels[1], channels[2] };
	}

	void ReadSeconds(const Json& parent, std::string_view key, float& out)
	{
		const auto value = parent.find(key);
		if (value != parent.end() && value->is_number())
		{
			out = ClampSeconds(value->get<float>());
		}
	}
}

HapticSettings::HapticSettings(const std::filesystem::path& path)
{
	std::ifstream file(path);
	if (!file.is_open())
	{
		std::cerr << "WARNING: haptics file not found at \"" << path.string()
			<< "\" -- using the built-in feel defaults.\n";
		return;
	}

	Json data;
	try
	{
		data = Json::parse(file);
	}
	catch (const Json::exception& exception)
	{
		std::cerr << "WARNING: haptics file \"" << path.string()
			<< "\" is invalid (" << exception.what() << ") -- using the built-in feel defaults.\n";
		return;
	}

	if (const auto rumble = data.find("rumble"); rumble != data.end() && rumble->is_object())
	{
		ReadRumble(*rumble, "menu_navigation", menuNavigation);
		ReadRumble(*rumble, "title_letter_base", titleLetterBase);
		ReadRumble(*rumble, "title_letter_grow", titleLetterGrow);
		ReadRumble(*rumble, "menu_entry_fly_in", menuEntryFlyIn);
		ReadRumble(*rumble, "piece_landed", pieceLanded);
		ReadRumble(*rumble, "hard_drop", hardDrop);
		ReadRumble(*rumble, "wall_hit", wallHit);
		ReadRumble(*rumble, "row_cleared", rowCleared);
		ReadRumble(*rumble, "tetris", tetris);
		ReadRumble(*rumble, "level_up", levelUp);
		ReadRumble(*rumble, "game_over", gameOver);
	}

	if (const auto lightbar = data.find("lightbar"); lightbar != data.end() && lightbar->is_object())
	{
		ReadColour(*lightbar, "menu", menuLightbar);
		ReadColour(*lightbar, "row_clear", rowClearLightbar);
		ReadColour(*lightbar, "game_over", gameOverLightbar);
	}

	if (const auto feel = data.find("feel"); feel != data.end() && feel->is_object())
	{
		ReadColour(*feel, "active_piece_glow", activePieceGlow);
		ReadSeconds(*feel, "delayed_auto_shift", delayedAutoShift);
		ReadSeconds(*feel, "auto_repeat_rate", autoRepeatRate);
	}
}
