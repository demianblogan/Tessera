#pragma once

#include <array>
#include <cstddef>
#include <vector>

#include "../resources/Assets.h"
#include "../resources/ResourceManager.h"

class AudioBalance;

// Shuffles Tessera's three gameplay tracks into a fresh play order, moves to
// the next one whenever the current one finishes, and reshuffles once the
// order is exhausted -- an endless, non-repeating (per lap) playlist.
//
// Application::Update() ticks this every frame regardless of which state is
// on top, the same way it ticks the gamepad, so the playlist keeps advancing
// and the duck below keeps easing even while GameplayState itself is paused
// (covered by PauseState and no longer receiving its own Update() calls).
class GameplayMusicPlayer
{
public:
	GameplayMusicPlayer(MusicManager& music, const AudioBalance& balance);

	// Starts a freshly shuffled playlist from track one. Safe to call again
	// (e.g. a restarted game) -- it stops whatever was already playing first.
	void Start();
	// Stops whichever track is playing; idle until Start() is called again.
	void Stop();

	// A muffled, quieter dip while the pause menu covers the game -- as if
	// stepping into another room -- eased in and out rather than snapped.
	void SetDucked(bool ducked);

	// `volumeStep` is the player's music slider (0-10), the same input
	// SettingsManager::Apply() feeds the menu tracks. Called every frame so a
	// slider change (even from the pause menu's Options) takes effect at once.
	void Update(float deltaTime, unsigned int volumeStep);

private:
	static constexpr std::array<Assets::MusicID, 3> Tracks{
		Assets::MusicID::Gameplay1, Assets::MusicID::Gameplay2, Assets::MusicID::Gameplay3 };

	void Reshuffle();
	void PlayCurrent();
	void ApplyVolume();

	MusicManager& music;
	const AudioBalance& balance;

	std::vector<Assets::MusicID> order;
	std::size_t playIndex = 0;
	bool active = false;

	unsigned int volumeStep = 10;

	float duck = 0.f;         // 0 = full volume, 1 = fully ducked
	float duckTarget = 0.f;
};
