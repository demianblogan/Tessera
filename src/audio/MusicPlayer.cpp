#include "MusicPlayer.h"

#include <algorithm>
#include <utility>

#include <SFML/Audio/Music.hpp>

#include "../utils/Random.h"
#include "AudioBalance.h"

namespace
{
	constexpr float DuckSpeed = 4.f; // 1.f / seconds to reach the target

	// Volume multiplier at full duck, where 1.0 would be normal (unducked)
	// volume -- so this is how quiet the gameplay playlist gets while paused.
	constexpr float DuckedMultiplier = 0.35f;

	// The ceiling sf::Music::setVolume is clamped to -- see AudioPlayer's own
	// MaxVolume for why this isn't 100 (SFML allows amplifying past it, and
	// this matches AudioBalance::MaxVolume, the highest per-track balance
	// multiplier a track can be authored at).
	constexpr float MaxVolume = 400.f;
}

MusicPlayer::MusicPlayer(MusicManager& music, const AudioBalance& balance)
	: music(music)
	, balance(balance)
{}

void MusicPlayer::StopCurrentTrack()
{
	switch (mode)
	{
	case Mode::MainMenu:
		music.Get(Assets::MusicID::MainMenu).stop();
		break;

	case Mode::Gameplay:
		if (gameplayTrackIndex < gameplayTrackOrder.size())
			music.Get(gameplayTrackOrder[gameplayTrackIndex]).stop();
		break;

	case Mode::None:
		break;
	}

	mode = Mode::None;
}

void MusicPlayer::ReshuffleGameplayPlaylist()
{
	// The last track of whatever playlist was playing before this reshuffle --
	// from the previous lap if the 3-track loop just finished, or (since
	// gameplayTrackOrder is a member that outlives one play session) from the
	// *previous* game's playlist if this is a fresh PlayGameplay() after a
	// restart. Either way, comparing the new shuffle's first track against it
	// below stops that same track from playing twice in a row across the seam.
	const Assets::MusicID previousPlaylistLastTrack =
		gameplayTrackOrder.empty() ? GameplayTracks.front() : gameplayTrackOrder.back();

	gameplayTrackOrder.assign(GameplayTracks.begin(), GameplayTracks.end());
	std::shuffle(gameplayTrackOrder.begin(), gameplayTrackOrder.end(), Random::Engine());

	// Avoid the same track playing twice back to back across a reshuffle --
	// including a restart, so it doesn't feel like the music just picked up
	// where it left off.
	if (gameplayTrackOrder.size() > 1 && gameplayTrackOrder.front() == previousPlaylistLastTrack)
		std::swap(gameplayTrackOrder[0], gameplayTrackOrder[1]);

	gameplayTrackIndex = 0;
}

void MusicPlayer::PlayCurrentGameplayTrack()
{
	if (gameplayTrackIndex >= gameplayTrackOrder.size())
		return;

	sf::Music& track = music.Get(gameplayTrackOrder[gameplayTrackIndex]);
	track.setLooping(false);
	track.play();
}

void MusicPlayer::PlayMainMenu()
{
	// Already playing it -- most commonly the loading screen having started it
	// early (see the class comment) and the menu shell then asking for it again
	// once it takes over. Restarting here would cut the track and replay it
	// from the top instead of letting it carry on seamlessly.
	if (mode == Mode::MainMenu)
		return;

	StopCurrentTrack();
	mode = Mode::MainMenu;

	sf::Music& track = music.Get(Assets::MusicID::MainMenu);
	track.setLooping(true);
	track.play();

	ApplyVolume();
}

void MusicPlayer::PlayGameplay()
{
	StopCurrentTrack();
	mode = Mode::Gameplay;
	duckAmount = duckTargetAmount = 0.f;

	ReshuffleGameplayPlaylist();
	PlayCurrentGameplayTrack();
	ApplyVolume();
}

void MusicPlayer::SetDucked(bool isDucked)
{
	duckTargetAmount = isDucked ? 1.f : 0.f;
}

void MusicPlayer::Update(float deltaTime, unsigned int newVolumeStep)
{
	volumeStep = newVolumeStep;

	duckAmount += (duckTargetAmount - duckAmount) * std::min(1.f, deltaTime * DuckSpeed);

	if (mode == Mode::Gameplay &&
		gameplayTrackIndex < gameplayTrackOrder.size() &&
		music.Get(gameplayTrackOrder[gameplayTrackIndex]).getStatus() == sf::Music::Status::Stopped)
	{
		gameplayTrackIndex++;

		if (gameplayTrackIndex >= gameplayTrackOrder.size())
			ReshuffleGameplayPlaylist();

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
	case Mode::Gameplay:
		if (gameplayTrackIndex >= gameplayTrackOrder.size())
		{
			return;
		}
		id = gameplayTrackOrder[gameplayTrackIndex];
		break;
	case Mode::None:
		return;
	}

	const float duckMultiplier = mode == Mode::Gameplay ? (1.f - duckAmount * (1.f - DuckedMultiplier)) : 1.f;
	const float sliderVolume = static_cast<float>(volumeStep) * 10.f;

	// sliderVolume and GetMusicBalance are both percentages of "normal" (see
	// AudioPlayer::GetVolumeFor for the full explanation) -- dividing by 100
	// once brings their product back down to a single percentage before
	// duckMultiplier scales it and it reaches sf::Music::setVolume.
	const float volume =
		std::clamp(sliderVolume * balance.GetMusicBalance(id) / 100.f * duckMultiplier, 0.f, MaxVolume);

	music.Get(id).setVolume(volume);
}
