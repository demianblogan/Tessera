#pragma once

#include <stop_token>

#include "../resources/ResourceManager.h"

namespace Loading
{
	class Progress;

	// Loads the non-GPU game assets (sounds, music, fonts) on a background
	// thread while the loading screen animates on the main one. Textures and
	// shaders are deliberately NOT here -- creating GPU objects off the main
	// thread deadlocks some drivers -- Application loads those synchronously.
	//
	// Holds references to the resource managers it fills; they must outlive it.
	class AssetLoadJob
	{
	public:
		AssetLoadJob(SoundBufferManager& soundBuffers, MusicManager& music, FontManager& fonts) noexcept;

		// Runs all stages, updating `progress` and calling progress.MarkDone()
		// at the end. Bails early if `stopToken` fires (window closed mid-load).
		void Run(std::stop_token stopToken, Progress& progress) const;

	private:
		SoundBufferManager& soundBuffers;
		MusicManager& music;
		FontManager& fonts;
	};
}
