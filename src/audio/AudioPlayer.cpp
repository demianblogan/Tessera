#include "AudioPlayer.h"

#include <algorithm>

AudioPlayer::AudioPlayer(SoundBufferManager& soundBuffers)
	: soundBuffers(soundBuffers)
{
	// No code
}

void AudioPlayer::Play(Assets::SoundID soundID, float pitch)
{
	if (activeSounds.size() >= MaxActiveSounds)
	{
		RemoveStoppedSounds();
	}

	if (activeSounds.size() >= MaxActiveSounds)
	{
		activeSounds.erase(activeSounds.begin());
	}

	activeSounds.emplace_back(soundID, soundBuffers.Get(soundID));

	ActiveSound& activeSound = activeSounds.back();
	activeSound.sound.setVolume(globalVolume);
	activeSound.sound.setPitch(pitch);
	activeSound.sound.play();
}

void AudioPlayer::Restart(Assets::SoundID soundID)
{
	for (ActiveSound& activeSound : activeSounds)
	{
		if (activeSound.id == soundID)
		{
			activeSound.sound.stop();
			activeSound.sound.setPlayingOffset(sf::Time::Zero);
			activeSound.sound.play();
			return;
		}
	}

	Play(soundID);
}

void AudioPlayer::RemoveStoppedSounds()
{
	activeSounds.erase(
		std::remove_if(
			activeSounds.begin(), activeSounds.end(),
			[](const ActiveSound& activeSound)
			{
				return activeSound.sound.getStatus() == sf::Sound::Status::Stopped;
			}
		),
		activeSounds.end()
	);
}

void AudioPlayer::SetGlobalVolume(float volume)
{
	globalVolume = volume;

	for (ActiveSound& activeSound : activeSounds)
	{
		activeSound.sound.setVolume(globalVolume);
	}
}