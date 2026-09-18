#include "AudioBalance.h"

#include <algorithm>
#include <array>
#include <fstream>
#include <string_view>
#include <utility>

#include <nlohmann/json.hpp>

namespace
{
	using Json = nlohmann::json;

	constexpr float DefaultVolume = 100.f;
	constexpr float MaxVolume = 400.f;   // SFML amplifies past 100

	// The JSON key each resource is listed under.
	constexpr std::array<std::pair<Assets::SoundID, std::string_view>, Assets::SoundIDCount> SoundNames =
	{ {
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
		{ Assets::SoundID::GameOver, "game_over" },
	} };

	constexpr std::array<std::pair<Assets::MusicID, std::string_view>, Assets::MusicIDCount> MusicNames =
	{ {
		{ Assets::MusicID::MainMenu, "main_menu_music" },
		{ Assets::MusicID::Gameplay1, "gameplay_music_1" },
		{ Assets::MusicID::Gameplay2, "gameplay_music_2" },
		{ Assets::MusicID::Gameplay3, "gameplay_music_3" },
	} };

	// Shared body for both categories -- only the id type (SoundID/MusicID) and
	// the id<->JSON-key table differ between them, so this is templated on
	// `IdType` (deduced from `names`/`out`) instead of writing the same loop
	// twice. `Size` is deduced too, from SoundNames'/MusicNames' own array
	// length (Assets::SoundIDCount/MusicIDCount) -- it just has to match `names`.
	//
	// Called as: ReadCategory(data, "music", MusicNames, musicVolumes);
	//   root     = the whole parsed JSON document.
	//   category = the key its object sits under ("music" / "sounds").
	//   names    = id<->key table to look each entry up by (SoundNames/MusicNames).
	//   out      = the map to fill in place; entries missing from the JSON are
	//              simply left out of it, so GetSoundBalance/GetMusicBalance
	//              fall back to DefaultVolume for them.
	template <typename IdType, std::size_t Size>
	void ReadCategory(const Json& root, const char* category,
		const std::array<std::pair<IdType, std::string_view>, Size>& names,
		std::unordered_map<IdType, float>& out)
	{
		const auto object = root.find(category);
		if (object == root.end() || !object->is_object())
			return;   // whole category missing -> everything defaults

		for (const auto& [id, key] : names)
		{
			const auto value = object->find(key);
			if (value != object->end() && value->is_number())
				out[id] = std::clamp(value->get<float>(), 0.f, MaxVolume);
		}
	}
}

AudioBalance::AudioBalance(const std::filesystem::path& path)
{
	// A missing or invalid file just leaves every track / sound at its
	// authored level -- audio_balance.json is an optional override.
	std::ifstream file(path);
	if (!file.is_open())
		return;

	try
	{
		const Json data = Json::parse(file);
		ReadCategory(data, "music", MusicNames, musicVolumes);
		ReadCategory(data, "sounds", SoundNames, soundVolumes);
	}
	catch (const Json::exception&)
	{
		// Same fallback as a missing file -- authored levels.
	}
}

float AudioBalance::GetSoundBalance(Assets::SoundID id) const noexcept
{
	const auto it = soundVolumes.find(id);
	return it != soundVolumes.end() ? it->second : DefaultVolume;
}

float AudioBalance::GetMusicBalance(Assets::MusicID id) const noexcept
{
	const auto it = musicVolumes.find(id);
	return it != musicVolumes.end() ? it->second : DefaultVolume;
}
