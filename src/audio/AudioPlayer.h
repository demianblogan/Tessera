#pragma once

#include <algorithm>
#include <cstddef>
#include <memory>
#include <vector>

#include <SFML/Audio/Sound.hpp>
#include <SFML/Audio/SoundBuffer.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/System/Time.hpp>

#include "../resources/ResourceManager.h"

class AudioBalance;

// The lowest pitch a sound is treated as playing at when working out how long
// it will take to finish (see `lifespan` below) -- guards against a division
// by (near) zero if something ever requests an absurdly low pitch.
inline constexpr float MinPitchForLifespan = 0.01f;

struct ActiveSound
{
	Assets::SoundID id;
	sf::Sound sound;

	// Time since this instance started playing (or was last restarted, see
	// AudioPlayer::Restart) -- not a property of the sound itself, just a
	// stopwatch RemoveStoppedSounds() reads to tell whether `lifespan` has
	// elapsed yet, so the pool can reclaim this slot.
	sf::Clock age;

	// How long this instance takes to finish playing: the buffer's own
	// duration, shortened by `pitch` (SFML plays a higher pitch faster).
	sf::Time lifespan;

	ActiveSound(Assets::SoundID id, const sf::SoundBuffer& buffer, float pitch)
		: id(id)
		, sound(buffer)
		, lifespan(buffer.getDuration() / std::max(pitch, MinPitchForLifespan))
	{}
};

class AudioPlayer
{
public:
	AudioPlayer(SoundBufferManager& soundBuffers, const AudioBalance& balance);

	void Play(Assets::SoundID soundID, float pitch = 1.f);
	void Restart(Assets::SoundID soundID, float pitch = 1.f);
	void RemoveStoppedSounds();
	void SetGlobalVolume(float volume);

private:
	[[nodiscard]] float GetVolumeFor(Assets::SoundID soundID) const;

	// SFML has a hard ceiling on simultaneous sf::Sound voices, and rapid
	// events (holding a movement key, a burst of hits) can pile up a lot of
	// short overlapping sounds. Cap the pool well below SFML's limit and drop
	// the oldest voice when full.
	static constexpr std::size_t MaxActiveSounds = 32;

	SoundBufferManager& soundBuffers;
	const AudioBalance& balance;

	// Heap-owned so the vector's own housekeeping (erase / remove_if, called
	// every frame) only ever moves pointers -- never a live sf::Sound, which
	// glitches or silences it under SFML 3's miniaudio backend.
	std::vector<std::unique_ptr<ActiveSound>> activeSounds;
	float globalVolume = 100.f;
};
