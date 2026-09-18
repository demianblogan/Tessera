#include "AudioPlayer.h"

#include <algorithm>

#include "AudioBalance.h"

namespace
{
	// Slack past a sound's own length before it is reclaimed. SFML 3's
	// sf::Sound::getStatus() briefly reports Stopped right after play(), so a
	// per-frame status poll would kill short sounds before they are heard --
	// instead each instance is kept for its full (pitch-adjusted) duration.
	const sf::Time ReclaimMargin = sf::milliseconds(80);

	// The ceiling sf::Sound::setVolume is clamped to. SFML accepts (and
	// amplifies) past its "authored" 100, so this isn't the API's own limit --
	// it just matches AudioBalance::MaxVolume, the highest per-sound balance
	// multiplier a sound can be authored at.
	constexpr float MaxVolume = 400.f;
}

AudioPlayer::AudioPlayer(SoundBufferManager& soundBuffers, const AudioBalance& balance)
	: soundBuffers(soundBuffers)
	, balance(balance)
{
	activeSounds.reserve(MaxActiveSounds);
}

float AudioPlayer::GetVolumeFor(Assets::SoundID soundID) const
{
	// Both factors are percentages of "normal" (100 = as authored / as set by
	// the player): `globalVolume` is the player's slider, already 0..100 (see
	// SettingsManager::Apply); `GetSoundBalance` is this one sound's authored
	// mix correction, typically close to 100 too. Multiplying two percentages
	// together scales by 100 twice, so dividing once here brings the result
	// back down to a single percentage before it reaches sf::Sound::setVolume
	// (e.g. a 50% slider times a 200%-boosted sound = 100%, not 10000%).
	return std::clamp(globalVolume * balance.GetSoundBalance(soundID) / 100.f, 0.f, MaxVolume);
}

void AudioPlayer::Play(Assets::SoundID soundID, float pitch)
{
	// Sounds that were never loaded (an asset file the project doesn't ship
	// yet) are silently skipped rather than crashing.
	if (!soundBuffers.Contains(soundID))
		return;

	if (activeSounds.size() >= MaxActiveSounds)
		RemoveStoppedSounds();

	if (activeSounds.size() >= MaxActiveSounds)
	{
		// Still full after reclaiming finished instances -- force out the
		// oldest one. New sounds are always appended (emplace_back below) and
		// erase_if in RemoveStoppedSounds() preserves relative order, so index
		// 0 is guaranteed to be whichever surviving instance started first.
		activeSounds.erase(activeSounds.begin());
	}

	auto& active = activeSounds.emplace_back(
		std::make_unique<ActiveSound>(soundID, soundBuffers.Get(soundID), pitch));
	active->sound.setVolume(GetVolumeFor(soundID));
	active->sound.setPitch(pitch);
	active->sound.play();
}

void AudioPlayer::Restart(Assets::SoundID soundID, float pitch)
{
	for (const std::unique_ptr<ActiveSound>& active : activeSounds)
	{
		if (active->id == soundID)
		{
			active->sound.stop();
			active->sound.setPlayingOffset(sf::Time::Zero);
			active->sound.setPitch(pitch);
			active->sound.play();
			active->age.restart();
			return;
		}
	}

	Play(soundID, pitch);
}

void AudioPlayer::RemoveStoppedSounds()
{
	std::erase_if(
		activeSounds,
		[](const std::unique_ptr<ActiveSound>& active)
		{
			// A looping instance never "finishes" -- keep it until it is
			// stopped explicitly (nothing here loops today, but be safe).
			return !active->sound.isLooping() && active->age.getElapsedTime() >= active->lifespan + ReclaimMargin;
		});
}

void AudioPlayer::SetGlobalVolume(float volume)
{
	globalVolume = volume;

	for (const std::unique_ptr<ActiveSound>& active : activeSounds)
	{
		active->sound.setVolume(GetVolumeFor(active->id));
	}
}
