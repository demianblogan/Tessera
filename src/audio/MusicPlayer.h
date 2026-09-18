#pragma once

#include <array>
#include <cstddef>
#include <vector>

#include "../resources/Assets.h"
#include "../resources/ResourceManager.h"

class AudioBalance;

// The single owner of every music track: the menu shell loop and the
// shuffled gameplay playlist (the game-over sting is a one-shot sound effect,
// not music -- see Assets::SoundID::GameOver, played through AudioPlayer over
// the still-playing, ducked gameplay music). Each PlayXxx() switches the
// active mode outright -- stopping whatever was playing before, whatever
// that was -- so a state's constructor only ever has to say what it wants
// playing, never worry about what came before it or who tears the previous
// state down and when. The one exception is PlayMainMenu() called while the
// menu track is already playing: a no-op, so the loading screen starting it
// early and the menu shell asking for it again once it takes over doesn't
// cut and restart the same track. That used to matter: a restart (destroy this
// GameplayState, construct a new one) built the new state, which started its
// own music, before the old state's destructor ran and stopped it again --
// silencing the replacement. Routing every transition through one mode
// switch here removes the destructor from the picture entirely.
//
// Application::Update() ticks this every frame regardless of which state is
// on top, the same way it ticks the gamepad, so the gameplay playlist keeps
// advancing and the pause/game-over duck keeps easing even while
// GameplayState itself is covered by PauseState or GameOverState and no
// longer receiving its own Update() calls.
class MusicPlayer
{
public:
	MusicPlayer(MusicManager& music, const AudioBalance& balance);

	// The menu shell's looping shell track (loading screen through the menus).
	void PlayMainMenu();

	// A freshly shuffled loop of the three gameplay tracks, played back to
	// back and reshuffled (never the same track twice across the seam) once
	// a lap finishes.
	void PlayGameplay();

	// A muffled, quieter dip on the gameplay playlist -- while the pause menu
	// covers the game, or the game-over screen is showing -- as if stepping
	// into another room, eased in and out rather than snapped. No effect on
	// the menu track. ("Ducking" is the standard audio-mixing term for
	// temporarily lowering one sound to make room for another -- here there's
	// no second sound competing for room, it's just reused for "quieter while
	// something else has focus".)
	void SetDucked(bool isDucked);

	// `volumeStep` is the player's music slider (0-10). Called every frame so
	// a slider change (even from the pause menu's Options) takes effect at
	// once, and so the gameplay playlist can advance and the duck can ease
	// independently of which state is currently receiving Update() calls.
	void Update(float deltaTime, unsigned int volumeStep);

private:
	enum class Mode
	{
		None,
		MainMenu,
		Gameplay
	};

	// Every MusicID except MainMenu -- the tracks PlayGameplay() shuffles
	// through. One less than Assets::MusicIDCount because MainMenu isn't a
	// gameplay track.
	static constexpr std::size_t GameplayTrackCount = Assets::MusicIDCount - 1;

	static constexpr std::array<Assets::MusicID, GameplayTrackCount> GameplayTracks =
	{
		Assets::MusicID::Gameplay1, Assets::MusicID::Gameplay2, Assets::MusicID::Gameplay3
	};

	void StopCurrentTrack();
	void ReshuffleGameplayPlaylist();
	void PlayCurrentGameplayTrack();
	void ApplyVolume();

	MusicManager& music;
	const AudioBalance& balance;

	Mode mode = Mode::None;

	std::vector<Assets::MusicID> gameplayTrackOrder;
	std::size_t gameplayTrackIndex = 0;

	unsigned int volumeStep = 10;

	// How muffled the gameplay playlist currently is: 0 = full volume,
	// 1 = fully ducked (see SetDucked). `duckAmount` eases toward
	// `duckTargetAmount` every Update() instead of snapping, so pausing /
	// unpausing fades rather than cuts.
	float duckAmount = 0.f;
	float duckTargetAmount = 0.f;
};
