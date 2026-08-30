#include "AudioPlayer.h"

#include <algorithm>

AudioPlayer::AudioPlayer(SoundBufferManager& soundBuffers)
	: soundBuffers(soundBuffers)
{
	activeSounds.reserve(MaxActiveSounds);
}

void AudioPlayer::Play(Assets::SoundID soundID, float pitch)
{
	// Sounds that were never loaded (an asset file the project doesn't ship
	// yet) are silently skipped rather than crashing.
	if (!soundBuffers.Contains(soundID))
	{
		return;
	}

	if (activeSounds.size() >= MaxActiveSounds)
	{
		RemoveStoppedSounds();
	}

	if (activeSounds.size() >= MaxActiveSounds)
	{
		activeSounds.erase(activeSounds.begin());
	}

	auto& active = activeSounds.emplace_back(std::make_unique<ActiveSound>(soundID, soundBuffers.Get(soundID)));
	active->sound.setVolume(globalVolume);
	active->sound.setPitch(pitch);
	active->sound.play();
}

void AudioPlayer::Restart(Assets::SoundID soundID)
{
	for (const std::unique_ptr<ActiveSound>& active : activeSounds)
	{
		if (active->id == soundID)
		{
			active->sound.stop();
			active->sound.setPlayingOffset(sf::Time::Zero);
			active->sound.play();
			return;
		}
	}

	Play(soundID);
}

void AudioPlayer::RemoveStoppedSounds()
{
	std::erase_if(activeSounds,
		[](const std::unique_ptr<ActiveSound>& active)
		{
			return active->sound.getStatus() == sf::Sound::Status::Stopped;
		});
}

void AudioPlayer::SetGlobalVolume(float volume)
{
	globalVolume = volume;

	for (const std::unique_ptr<ActiveSound>& active : activeSounds)
	{
		active->sound.setVolume(globalVolume);
	}
}
