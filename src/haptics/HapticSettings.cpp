#include "HapticSettings.h"

#include <algorithm>
#include <array>
#include <fstream>
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
			return;

		if (const auto low = object->find("low"); low != object->end() && low->is_number())
			rumble.lowMotor = ClampMotor(low->get<float>());
		if (const auto high = object->find("high"); high != object->end() && high->is_number())
			rumble.highMotor = ClampMotor(high->get<float>());
		if (const auto duration = object->find("duration"); duration != object->end() && duration->is_number())
			rumble.duration = ClampSeconds(duration->get<float>());
	}

	// Reads a color written as [r, g, b] (0..255 each).
	void ReadColor(const Json& parent, std::string_view key, HapticSettings::Color& color)
	{
		const auto array = parent.find(key);
		if (array == parent.end() || !array->is_array() || array->size() != 3)
			return;

		std::array<std::uint8_t, 3> channels = { color.r, color.g, color.b };
		for (std::size_t i = 0; i < 3; ++i)
			if ((*array)[i].is_number())
				channels[i] = static_cast<std::uint8_t>(std::clamp((*array)[i].get<int>(), 0, 255));

		color = { channels[0], channels[1], channels[2] };
	}

	void ReadSeconds(const Json& parent, std::string_view key, float& out)
	{
		const auto value = parent.find(key);
		if (value != parent.end() && value->is_number())
			out = ClampSeconds(value->get<float>());
	}
}

HapticSettings::Color HapticSettings::LightbarFor(std::string_view key, Color fallback) const
{
	const auto entry = lightbarByKey.find(std::string(key));
	return entry != lightbarByKey.end() ? entry->second : fallback;
}

HapticSettings::HapticSettings(const std::filesystem::path& path)
{
	// A missing or invalid file just leaves every field at its compiled-in
	// default -- haptics.json is an optional feel override, not a requirement.
	std::ifstream file(path);
	if (!file.is_open())
	{
		return;
	}

	Json data;

	try
	{
		data = Json::parse(file);
	}
	catch (const Json::exception&)
	{
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
		ReadRumble(*rumble, "hold", hold);
		ReadRumble(*rumble, "t_spin", tSpin);
		ReadRumble(*rumble, "back_to_back", backToBack);
		ReadRumble(*rumble, "perfect_clear", perfectClear);
		ReadRumble(*rumble, "speed_surge", speedSurge);
		ReadRumble(*rumble, "garbage_row", garbageRow);
	}

	if (const auto lightbar = data.find("lightbar"); lightbar != data.end() && lightbar->is_object())
	{
		ReadColor(*lightbar, "menu", menuLightbar);
		ReadColor(*lightbar, "row_clear", rowClearLightbar);
		ReadColor(*lightbar, "game_over", gameOverLightbar);
		ReadColor(*lightbar, "perfect_clear", perfectClearLightbar);

		// Keep every [r,g,b] entry by key, so menus can look their own up.
		for (const auto& [key, value] : lightbar->items())
		{
			if (value.is_array() && value.size() == 3)
			{
				Color color{};
				ReadColor(*lightbar, key, color);
				lightbarByKey[key] = color;
			}
		}
	}

	if (const auto feel = data.find("feel"); feel != data.end() && feel->is_object())
	{
		ReadColor(*feel, "active_piece_glow", activePieceGlow);
		ReadSeconds(*feel, "delayed_auto_shift", delayedAutoShift);
		ReadSeconds(*feel, "auto_repeat_rate", autoRepeatRate);
	}
}
