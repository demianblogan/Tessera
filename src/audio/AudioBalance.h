#pragma once

#include <filesystem>
#include <unordered_map>

#include "../resources/Assets.h"

// Per-resource relative volume, loaded once from assets/data/audio_balance.json
// -- design-time mixing data, not a player setting. It exists so a track that
// was authored too quiet (or a sound too loud) can be corrected without a
// rebuild: edit the JSON, restart.
//
// Values are a percentage of the resource's own level: 100 = as authored, 50 =
// half, 200 = doubled (SFML's setVolume amplifies past 100). A missing entry
// defaults to 100. This is one factor; the player's settings slider is another,
// and the two are multiplied.
class AudioBalance
{
public:
	explicit AudioBalance(const std::filesystem::path& path);

	[[nodiscard]] float ForSound(Assets::SoundID id) const noexcept;
	[[nodiscard]] float ForMusic(Assets::MusicID id) const noexcept;

private:
	std::unordered_map<Assets::SoundID, float> soundVolumes;
	std::unordered_map<Assets::MusicID, float> musicVolumes;
};
