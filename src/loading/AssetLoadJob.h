#pragma once

#include <stop_token>

#include "../resources/ResourceManager.h"

class ShaderManager;

namespace Loading
{
	class Progress;

	// Loads every heavyweight game asset (textures, sounds, music, shaders,
	// fonts) on a background thread while the loading screen animates on the
	// main one. Holds references to the resource managers it fills; those
	// managers must outlive the job.
	//
	// SFML's GPU resources need an active OpenGL context on whatever thread
	// touches them, so Run() spins up its own sf::Context for its lifetime.
	class AssetLoadJob
	{
	public:
		AssetLoadJob(
			TextureManager& textures,
			SoundBufferManager& soundBuffers,
			MusicManager& music,
			ShaderManager& shaders,
			FontManager& fonts) noexcept;

		// Runs all stages start to finish, updating `progress` as it goes and
		// calling progress.MarkDone() at the end. Bails early (leaving assets
		// half-loaded) if `stopToken` is triggered -- only happens when the
		// window is closed mid-load.
		void Run(std::stop_token stopToken, Progress& progress) const;

	private:
		TextureManager& textures;
		SoundBufferManager& soundBuffers;
		MusicManager& music;
		ShaderManager& shaders;
		FontManager& fonts;
	};
}
