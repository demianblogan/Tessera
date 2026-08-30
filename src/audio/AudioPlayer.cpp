#include "AudioPlayer.h"

#include <algorithm>

namespace
{
	// Slack past a sound's own length before it is reclaimed. SFML 3's
	// sf::Sound::getStatus() briefly reports Stopped right after play(), so a
	// per-frame status poll would kill short sounds before they are heard --
	// instead each instance is kept for its full (pitch-adjusted) duration.
	const sf::Time ReclaimMargin = sf::milliseconds(80);
}

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

	auto& active = activeSounds.emplace_back(
		std::make_unique<ActiveSound>(soundID, soundBuffers.Get(soundID), pitch));
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
			active->age.restart();
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
			return active->age.getElapsedTime() >= active->lifespan + ReclaimMargin;
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
