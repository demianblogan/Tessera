#include "AudioBalance.h"

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

	constexpr float DefaultVolume = 100.f;
	constexpr float MaxVolume = 400.f;   // SFML amplifies past 100

	// The JSON key each resource is listed under.
	constexpr std::array<std::pair<Assets::SoundID, std::string_view>, 10> SoundNames{ {
		{ Assets::SoundID::TitleButtonDrop, "title_button_drop" },
		{ Assets::SoundID::MenuItemAppeared, "menu_item_appeared" },
		{ Assets::SoundID::MenuItemSelected, "menu_item_selected" },
		{ Assets::SoundID::MenuItemPressed, "menu_item_pressed" },
		{ Assets::SoundID::DropPiece, "drop_piece" },
		{ Assets::SoundID::MovePiece, "move_piece" },
		{ Assets::SoundID::RotatePiece, "rotate_piece" },
		{ Assets::SoundID::PieceHitWall, "piece_hit_wall" },
		{ Assets::SoundID::NextLevel, "next_level" },
		{ Assets::SoundID::RowCleared, "row_cleared" },
	} };

	constexpr std::array<std::pair<Assets::MusicID, std::string_view>, 3> MusicNames{ {
		{ Assets::MusicID::MainMenu, "main_menu_music" },
		{ Assets::MusicID::Gameplay, "gameplay_music" },
		{ Assets::MusicID::GameOver, "game_over_music" },
	} };

	template <typename IdType, std::size_t Size>
	void ReadCategory(const Json& root, const char* category,
		const std::array<std::pair<IdType, std::string_view>, Size>& names,
		std::unordered_map<IdType, float>& out)
	{
		const auto object = root.find(category);
		if (object == root.end() || !object->is_object())
		{
			return;   // whole category missing -> everything defaults
		}

		for (const auto& [id, key] : names)
		{
			const auto value = object->find(key);
			if (value != object->end() && value->is_number())
			{
				out[id] = std::clamp(value->get<float>(), 0.f, MaxVolume);
			}
		}
	}
}

AudioBalance::AudioBalance(const std::filesystem::path& path)
{
	std::ifstream file(path);
	if (!file.is_open())
	{
		std::cerr << "WARNING: audio balance file not found at \"" << path.string()
			<< "\" -- every track / sound plays at its authored level.\n";
		return;
	}

	try
	{
		const Json data = Json::parse(file);
		ReadCategory(data, "music", MusicNames, musicVolumes);
		ReadCategory(data, "sounds", SoundNames, soundVolumes);
	}
	catch (const Json::exception& exception)
	{
		std::cerr << "WARNING: audio balance file \"" << path.string()
			<< "\" is invalid (" << exception.what() << ") -- using authored levels.\n";
	}
}

float AudioBalance::ForSound(Assets::SoundID id) const noexcept
{
	const auto it = soundVolumes.find(id);
	return it != soundVolumes.end() ? it->second : DefaultVolume;
}

float AudioBalance::ForMusic(Assets::MusicID id) const noexcept
{
	const auto it = musicVolumes.find(id);
	return it != musicVolumes.end() ? it->second : DefaultVolume;
}
