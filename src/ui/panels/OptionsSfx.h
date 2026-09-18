#pragma once

#include "../../audio/AudioPlayer.h"
#include "../../resources/Assets.h"

// Shared audio cues for the Options category panels -- the same menu-navigation
// and menu-press sounds as the main menu, at tweaked pitch. No gameplay sounds.
namespace OptionsSfx
{
	constexpr float NavPitchHigh = 1.14f;
	constexpr float NavPitchLow = 0.9f;
	constexpr float ResetPitch = 0.9f;
	constexpr float DialogOpenPitch = 0.85f;

	inline void Nav(AudioPlayer& audio, int direction)
	{
		audio.Restart(Assets::SoundID::MenuItemSelected, direction >= 0 ? NavPitchHigh : NavPitchLow);
	}

	inline void Step(AudioPlayer& audio, int direction) { Nav(audio, direction); }
	// On uses the same pitch as a right/down nav step, off the same as left/up.
	inline void Toggle(AudioPlayer& audio, bool isOn) { Nav(audio, isOn ? 1 : -1); }
	inline void Apply(AudioPlayer& audio) { audio.Play(Assets::SoundID::MenuItemPressed); }
	inline void Reset(AudioPlayer& audio) { audio.Play(Assets::SoundID::MenuItemPressed, ResetPitch); }
	inline void DialogOpen(AudioPlayer& audio) { audio.Play(Assets::SoundID::MenuItemPressed, DialogOpenPitch); }
}
