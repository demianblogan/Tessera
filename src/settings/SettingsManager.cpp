#include "SettingsManager.h"

#include <fstream>

#include "../audio/AudioPlayer.h"
#include "../core/Context.h"

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
        Save();
        return;
    }

    // Parse into a scratch copy. Only adopt it if every field read and every
    // value is in range; a truncated or hand-edited file falls back to the
    // defaults already in `settings` and is rewritten clean.
    GameSettings parsed;
    int blockRenderStyleValue = 0;

    file >> parsed.verticalSyncEnabled
        >> parsed.frameRateLimit
        >> blockRenderStyleValue
        >> parsed.soundVolume
        >> parsed.musicVolume;

    const bool valuesAreValid =
        static_cast<bool>(file) &&
        (blockRenderStyleValue == 0 || blockRenderStyleValue == 1) &&
        parsed.frameRateLimit <= MaxFrameRateLimit &&
        parsed.soundVolume <= MaxVolumeStep &&
        parsed.musicVolume <= MaxVolumeStep;

    if (!valuesAreValid)
    {
        Save();
        return;
    }

    parsed.blockRenderStyle = static_cast<BlockRenderStyle>(blockRenderStyleValue);
    settings = parsed;
}

void SettingsManager::Save() const
{
    std::ofstream file(filepath);

    file << settings.verticalSyncEnabled << '\n';
    file << settings.frameRateLimit << '\n';
    file << static_cast<int>(settings.blockRenderStyle) << '\n';
    file << settings.soundVolume << '\n';
    file << settings.musicVolume << '\n';
}

void SettingsManager::Apply(Context& context) const
{
	// --- Graphics settings ---

	// SFML warns against combining vsync with a manual frame-rate limit, so the
	// limit is only applied when vsync is off.
    context.window.setVerticalSyncEnabled(settings.verticalSyncEnabled);
    context.window.setFramerateLimit(settings.verticalSyncEnabled ? 0u : settings.frameRateLimit);

	// --- Audio settings ---

    float musicVolume = settings.musicVolume * 10.f;
    context.music.ForEach([musicVolume](sf::Music& music)
        {
            music.setVolume(musicVolume);
        }
    );

    float soundVolume = settings.soundVolume * 10.f;
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
