#include "SettingsManager.h"

#include <fstream>
#include <system_error>

#include "../audio/AudioPlayer.h"
#include "../core/Context.h"
#include "../utils/SafeFileWrite.h"

SettingsManager::SettingsManager(const std::filesystem::path& filepath)
	: filepath(filepath)
{
	// No code
}

void SettingsManager::Load()
{
	std::ifstream file(filepath);

	if (!file.is_open())
	{
		// First run -- write the defaults `settings` already holds.
		Save();
		return;
	}

	// Parse into a scratch copy. Only adopt it if the format version matches,
	// every field reads, and every value is in range; otherwise the file is
	// kept as .corrupt and replaced with defaults.
	int formatVersion = 0;
	GameSettings parsed;
	int blockRenderStyleValue = 0;

	file >> formatVersion
		>> parsed.verticalSyncEnabled
		>> parsed.frameRateLimit
		>> blockRenderStyleValue
		>> parsed.soundVolume
		>> parsed.musicVolume;

	const bool fileIsUsable =
		static_cast<bool>(file) &&
		formatVersion == GameSettings::FormatVersion &&
		(blockRenderStyleValue == 0 || blockRenderStyleValue == 1) &&
		parsed.frameRateLimit <= MaxFrameRateLimit &&
		parsed.soundVolume <= MaxVolumeStep &&
		parsed.musicVolume <= MaxVolumeStep;

	if (!fileIsUsable)
	{
		file.close();
		static_cast<void>(SafeFileWrite::PreserveCorruptFile(filepath));
		Save();
		return;
	}

	parsed.blockRenderStyle = static_cast<BlockRenderStyle>(blockRenderStyleValue);
	settings = parsed;
}

void SettingsManager::Save() const
{
	std::error_code error;
	std::filesystem::create_directories(filepath.parent_path(), error);

	std::filesystem::path temporaryPath(filepath);
	temporaryPath += ".tmp";

	{
		std::ofstream file(temporaryPath, std::ios::trunc);
		if (!file.is_open())
		{
			return;
		}

		file << GameSettings::FormatVersion << '\n';
		file << settings.verticalSyncEnabled << '\n';
		file << settings.frameRateLimit << '\n';
		file << static_cast<int>(settings.blockRenderStyle) << '\n';
		file << settings.soundVolume << '\n';
		file << settings.musicVolume << '\n';
	}

	static_cast<void>(SafeFileWrite::ReplaceFileAtomically(temporaryPath, filepath));
}

void SettingsManager::Apply(Context& context) const
{
	// --- Graphics settings ---

	// SFML warns against combining vsync with a manual frame-rate limit, so the
	// limit is only applied when vsync is off.
	context.window.setVerticalSyncEnabled(settings.verticalSyncEnabled);
	context.window.setFramerateLimit(settings.verticalSyncEnabled ? 0u : settings.frameRateLimit);

	// --- Audio settings ---

	const float musicVolume = settings.musicVolume * 10.f;
	context.music.ForEach([musicVolume](sf::Music& music)
		{
			music.setVolume(musicVolume);
		}
	);

	const float soundVolume = settings.soundVolume * 10.f;
	context.audioPlayer.SetGlobalVolume(soundVolume);
}

GameSettings& SettingsManager::GetSettings()
{
	return settings;
}

const GameSettings& SettingsManager::GetSettings() const
{
	return settings;
}
