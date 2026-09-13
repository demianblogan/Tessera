#include "MusicPlayer.h"

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

MusicPlayer::MusicPlayer(MusicManager& music, const AudioBalance& balance)
	: music(music)
	, balance(balance)
{
}

void MusicPlayer::StopCurrent()
{
	switch (mode)
	{
	case Mode::MainMenu:
		music.Get(Assets::MusicID::MainMenu).stop();
		break;
	case Mode::GameOver:
		music.Get(Assets::MusicID::GameOver).stop();
		break;
	case Mode::Gameplay:
		if (gameplayIndex < gameplayOrder.size())
		{
			music.Get(gameplayOrder[gameplayIndex]).stop();
		}
		break;
	case Mode::None:
		break;
	}

	mode = Mode::None;
}

void MusicPlayer::ReshuffleGameplay()
{
	const Assets::MusicID previousLast = gameplayOrder.empty() ? GameplayTracks.front() : gameplayOrder.back();

	gameplayOrder.assign(GameplayTracks.begin(), GameplayTracks.end());
	std::shuffle(gameplayOrder.begin(), gameplayOrder.end(), Random::Engine());

	// Avoid the same track playing twice back to back across a reshuffle.
	if (gameplayOrder.size() > 1 && gameplayOrder.front() == previousLast)
	{
		std::swap(gameplayOrder[0], gameplayOrder[1]);
	}

	gameplayIndex = 0;
}

void MusicPlayer::PlayCurrentGameplayTrack()
{
	if (gameplayIndex >= gameplayOrder.size())
	{
		return;
	}

	sf::Music& track = music.Get(gameplayOrder[gameplayIndex]);
	track.setLooping(false);
	track.play();
}

void MusicPlayer::PlayMainMenu()
{
	StopCurrent();
	mode = Mode::MainMenu;

	sf::Music& track = music.Get(Assets::MusicID::MainMenu);
	track.setLooping(true);
	track.play();

	ApplyVolume();
}

void MusicPlayer::PlayGameOver()
{
	StopCurrent();
	mode = Mode::GameOver;

	sf::Music& track = music.Get(Assets::MusicID::GameOver);
	track.setLooping(false);
	track.play();

	ApplyVolume();
}

void MusicPlayer::PlayGameplay()
{
	StopCurrent();
	mode = Mode::Gameplay;
	duck = duckTarget = 0.f;

	ReshuffleGameplay();
	PlayCurrentGameplayTrack();
	ApplyVolume();
}

void MusicPlayer::SetDucked(bool ducked)
{
	duckTarget = ducked ? 1.f : 0.f;
}

void MusicPlayer::Update(float deltaTime, unsigned int newVolumeStep)
{
	volumeStep = newVolumeStep;

	duck += (duckTarget - duck) * std::min(1.f, deltaTime * DuckSpeed);

	if (mode == Mode::Gameplay && gameplayIndex < gameplayOrder.size()
		&& music.Get(gameplayOrder[gameplayIndex]).getStatus() == sf::Music::Status::Stopped)
	{
		++gameplayIndex;
		if (gameplayIndex >= gameplayOrder.size())
		{
			ReshuffleGameplay();
		}
		PlayCurrentGameplayTrack();
	}

	ApplyVolume();
}

void MusicPlayer::ApplyVolume()
{
	Assets::MusicID id{};
	switch (mode)
	{
	case Mode::MainMenu:
		id = Assets::MusicID::MainMenu;
		break;
	case Mode::GameOver:
		id = Assets::MusicID::GameOver;
		break;
	case Mode::Gameplay:
		if (gameplayIndex >= gameplayOrder.size())
		{
			return;
		}
		id = gameplayOrder[gameplayIndex];
		break;
	case Mode::None:
		return;
	}

	const float duckMultiplier = mode == Mode::Gameplay ? (1.f - duck * (1.f - DuckedMultiplier)) : 1.f;
	const float sliderVolume = static_cast<float>(volumeStep) * 10.f;
	const float volume = std::clamp(sliderVolume * balance.ForMusic(id) / 100.f * duckMultiplier, 0.f, 400.f);
	music.Get(id).setVolume(volume);
}
