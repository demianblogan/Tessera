#include "AssetLoadJob.h"

#include "LoadingProgress.h"
#include "../resources/Assets.h"

namespace Loading
{
	AssetLoadJob::AssetLoadJob(SoundBufferManager& soundBuffers, MusicManager& music, FontManager& fonts) noexcept
		: soundBuffers(soundBuffers)
		, music(music)
		, fonts(fonts)
	{}

	bool AssetLoadJob::RunStage(Stage stage, const std::stop_token& stopToken, Progress& progress,
		const std::function<void()>& loadResources) const
	{
		if (stopToken.stop_requested())
			return false;

		progress.SetStage(stage);
		loadResources();

		return !stopToken.stop_requested();
	}

	void AssetLoadJob::LoadAudioAssets() const
	{
		namespace Paths = Assets::Paths;

		soundBuffers.Load(Assets::SoundID::TitleButtonDrop, Paths::Sounds::TitleButtonDrop);
		soundBuffers.Load(Assets::SoundID::MenuItemAppeared, Paths::Sounds::MenuItemAppeared);
		soundBuffers.Load(Assets::SoundID::MenuItemSelected, Paths::Sounds::MenuItemSelected);
		soundBuffers.Load(Assets::SoundID::MenuItemPressed, Paths::Sounds::MenuItemPressed);
		soundBuffers.Load(Assets::SoundID::DropPiece, Paths::Sounds::DropPiece);
		soundBuffers.Load(Assets::SoundID::MovePiece, Paths::Sounds::MovePiece);
		soundBuffers.Load(Assets::SoundID::RotatePiece, Paths::Sounds::RotatePiece);
		soundBuffers.Load(Assets::SoundID::PieceHitWall, Paths::Sounds::PieceHitWall);
		soundBuffers.Load(Assets::SoundID::NextLevel, Paths::Sounds::NextLevel);
		soundBuffers.Load(Assets::SoundID::RowCleared, Paths::Sounds::RowCleared);
		soundBuffers.Load(Assets::SoundID::GameOver, Paths::Sounds::GameOver);
	}

	void AssetLoadJob::LoadMusicAssets() const
	{
		namespace Paths = Assets::Paths;

		// MainMenu (the shell track) is loaded synchronously by Application so
		// the loading screen can start it immediately.
		music.Load(Assets::MusicID::Gameplay1, Paths::Music::Gameplay1);
		music.Load(Assets::MusicID::Gameplay2, Paths::Music::Gameplay2);
		music.Load(Assets::MusicID::Gameplay3, Paths::Music::Gameplay3);
	}

	void AssetLoadJob::LoadFontAssets() const
	{
		namespace Paths = Assets::Paths;

		fonts.Load(Assets::FontID::Main, Paths::Fonts::Main);
		fonts.Load(Assets::FontID::Menu, Paths::Fonts::Menu);
		fonts.Load(Assets::FontID::MenuList, Paths::Fonts::MenuList);
	}

	void AssetLoadJob::Run(std::stop_token stopToken, Progress& progress) const
	{
		bool areAudioAssetsLoaded = RunStage(Stage::Audio, stopToken, progress, [this] { LoadAudioAssets(); });
		bool areMusicAssetsLoaded = RunStage(Stage::Music, stopToken, progress, [this] { LoadMusicAssets(); });
		bool areFontAssetsLoaded = RunStage(Stage::Interface, stopToken, progress, [this] { LoadFontAssets(); });

		bool isLoadingCompleted = areAudioAssetsLoaded && areMusicAssetsLoaded && areFontAssetsLoaded;

		if (isLoadingCompleted)
			progress.MarkDone();
	}
}
