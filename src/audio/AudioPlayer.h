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

struct ActiveSound
{
	Assets::SoundID id;
	sf::Sound sound;
	sf::Clock age;
	sf::Time lifespan;   // how long this instance will play for (buffer / pitch)

	ActiveSound(Assets::SoundID id, const sf::SoundBuffer& buffer, float pitch)
		: id(id)
		, sound(buffer)
		, lifespan(buffer.getDuration() / std::max(pitch, 0.01f))
	{
		// No code
	}
};

class AudioPlayer
{
private:
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

public:
	AudioPlayer(SoundBufferManager& soundBuffers, const AudioBalance& balance);

	void Play(Assets::SoundID soundID, float pitch = 1.f);
	void Restart(Assets::SoundID soundID);
	void RemoveStoppedSounds();
	void SetGlobalVolume(float volume);

private:
	[[nodiscard]] float VolumeFor(Assets::SoundID soundID) const;
};