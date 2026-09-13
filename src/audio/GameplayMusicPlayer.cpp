#include "GameplayMusicPlayer.h"

#include <algorithm>
#include <utility>

#include <SFML/Audio/Music.hpp>

#include "../utils/Random.h"
#include "AudioBalance.h"

namespace
{
	constexpr float DuckSpeed = 4.f;            // 1 / seconds to reach the target
	constexpr float DuckedMultiplier = 0.35f;   // how quiet "paused" gets, vs 1.0 playing
}

GameplayMusicPlayer::GameplayMusicPlayer(MusicManager& music, const AudioBalance& balance)
	: music(music)
	, balance(balance)
{
}

void GameplayMusicPlayer::Reshuffle()
{
	const Assets::MusicID previousLast = order.empty() ? Tracks.front() : order.back();

	order.assign(Tracks.begin(), Tracks.end());
	std::shuffle(order.begin(), order.end(), Random::Engine());

	// Avoid the same track playing twice back to back across a reshuffle.
	if (order.size() > 1 && order.front() == previousLast)
	{
		std::swap(order[0], order[1]);
	}

	playIndex = 0;
}

void GameplayMusicPlayer::PlayCurrent()
{
	if (playIndex >= order.size())
	{
		return;
	}

	sf::Music& track = music.Get(order[playIndex]);
	track.setLooping(false);
	track.play();
}

void GameplayMusicPlayer::Start()
{
	Stop();
	Reshuffle();
	active = true;
	duck = duckTarget = 0.f;
	PlayCurrent();
	ApplyVolume();
}

void GameplayMusicPlayer::Stop()
{
	if (playIndex < order.size())
	{
		music.Get(order[playIndex]).stop();
	}
	active = false;
}

void GameplayMusicPlayer::SetDucked(bool ducked)
{
	duckTarget = ducked ? 1.f : 0.f;
}

void GameplayMusicPlayer::Update(float deltaTime, unsigned int newVolumeStep)
{
	volumeStep = newVolumeStep;

	if (!active)
	{
		return;
	}

	duck += (duckTarget - duck) * std::min(1.f, deltaTime * DuckSpeed);

	if (playIndex < order.size() && music.Get(order[playIndex]).getStatus() == sf::Music::Status::Stopped)
	{
		++playIndex;
		if (playIndex >= order.size())
		{
			Reshuffle();
		}
		PlayCurrent();
	}

	ApplyVolume();
}

void GameplayMusicPlayer::ApplyVolume()
{
	if (playIndex >= order.size())
	{
		return;
	}

	const Assets::MusicID id = order[playIndex];
	const float sliderVolume = static_cast<float>(volumeStep) * 10.f;
	const float duckMultiplier = 1.f - duck * (1.f - DuckedMultiplier);
	const float volume = std::clamp(sliderVolume * balance.ForMusic(id) / 100.f * duckMultiplier, 0.f, 400.f);
	music.Get(id).setVolume(volume);
}
