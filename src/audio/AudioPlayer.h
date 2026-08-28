#pragma once

#include <cstddef>
#include <vector>

#include <SFML/Audio/Sound.hpp>

#include "../resources/ResourceManager.h"

struct ActiveSound
{
	Assets::SoundID id;
	sf::Sound sound;

	ActiveSound(Assets::SoundID id, const sf::SoundBuffer& buffer)
		: id(id), sound(buffer)
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
	std::vector<ActiveSound> activeSounds;
	float globalVolume = 100.f;

public:
	AudioPlayer(SoundBufferManager& soundBuffers);

	void Play(Assets::SoundID soundID);
	void Restart(Assets::SoundID soundID);
	void RemoveStoppedSounds();
	void SetGlobalVolume(float volume);
};