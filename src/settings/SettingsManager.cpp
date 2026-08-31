#include "SettingsManager.h"

#include <algorithm>
#include <array>
#include <fstream>
#include <system_error>

#include "../audio/AudioBalance.h"
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
	int windowModeValue = 0;
	unsigned int resolutionWidth = 0;
	unsigned int resolutionHeight = 0;

	file >> formatVersion
		>> parsed.verticalSyncEnabled
		>> parsed.showFps
		>> parsed.crtFilterEnabled
		>> parsed.soundVolume
		>> parsed.musicVolume
		>> windowModeValue
		>> resolutionWidth
		>> resolutionHeight;

	const bool fileIsUsable =
		static_cast<bool>(file) &&
		formatVersion == GameSettings::FormatVersion &&
		windowModeValue >= 0 && windowModeValue <= 2 &&
		parsed.soundVolume <= MaxVolumeStep &&
		parsed.musicVolume <= MaxVolumeStep;

	if (!fileIsUsable)
	{
		file.close();
		static_cast<void>(SafeFileWrite::PreserveCorruptFile(filepath));
		Save();
		return;
	}

	parsed.display.windowMode = static_cast<Display::WindowMode>(windowModeValue);
	parsed.display.resolution = { resolutionWidth, resolutionHeight };
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
		file << settings.showFps << '\n';
		file << settings.crtFilterEnabled << '\n';
		file << settings.soundVolume << '\n';
		file << settings.musicVolume << '\n';
		file << static_cast<int>(settings.display.windowMode) << '\n';
		file << settings.display.resolution.x << '\n';
		file << settings.display.resolution.y << '\n';
	}

	static_cast<void>(SafeFileWrite::ReplaceFileAtomically(temporaryPath, filepath));
}

void SettingsManager::Apply(Context& context) const
{
	// --- Graphics settings ---
	// The window mode / resolution are applied by Application (they recreate the
	// window); this only touches per-window toggles.

	context.window.setVerticalSyncEnabled(settings.verticalSyncEnabled);

	// --- Audio settings ---

	// Player slider (0-100) combined with each track's own balance coefficient.
	const float musicSlider = settings.musicVolume * 10.f;
	constexpr std::array musicIds{ Assets::MusicID::MainMenu, Assets::MusicID::Gameplay, Assets::MusicID::GameOver };
	for (const Assets::MusicID id : musicIds)
	{
		if (context.music.Contains(id))
		{
			context.music.Get(id).setVolume(
				std::clamp(musicSlider * context.audioBalance.ForMusic(id) / 100.f, 0.f, 400.f));
		}
	}

	// The sound slider is stored on the AudioPlayer; per-sound balance is
	// applied there per instance.
	context.audioPlayer.SetGlobalVolume(settings.soundVolume * 10.f);
}

GameSettings& SettingsManager::GetSettings()
{
	return settings;
}

const GameSettings& SettingsManager::GetSettings() const
{
	return settings;
}
