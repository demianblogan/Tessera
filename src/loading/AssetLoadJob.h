#pragma once

#include <functional>
#include <stop_token>

#include "../resources/ResourceManager.h"

namespace Loading
{
	enum class Stage;
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
		// Runs one stage of Run(): reports `stage` to `progress`, then calls
		// `loadResources` -- unless a stop was already requested, in which case
		// it skips straight to returning false without starting the stage.
		// Returns whether the caller should move on to the next stage (false
		// once `stopToken` fires, either just before or during this one).
		[[nodiscard]] bool RunStage(Stage stage, const std::stop_token& stopToken, Progress& progress,
			const std::function<void()>& loadResources) const;

		void LoadAudioAssets() const;
		void LoadMusicAssets() const;
		void LoadFontAssets() const;

		SoundBufferManager& soundBuffers;
		MusicManager& music;
		FontManager& fonts;
	};
}
