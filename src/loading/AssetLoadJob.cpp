#include "AssetLoadJob.h"

#include <filesystem>

#include "LoadingProgress.h"
#include "../resources/Assets.h"

namespace Loading
{
	AssetLoadJob::AssetLoadJob(SoundBufferManager& soundBuffers, MusicManager& music, FontManager& fonts) noexcept
		: soundBuffers(soundBuffers)
		, music(music)
		, fonts(fonts)
	{
		// No code.
	}

	void AssetLoadJob::Run(std::stop_token stopToken, Progress& progress) const
	{
		namespace Paths = Assets::Paths;

		const auto stage = [&](Stage which, auto&& work) -> bool
		{
			if (stopToken.stop_requested())
			{
				return false;
			}

			progress.SetStage(which);
			work();
			return !stopToken.stop_requested();
		};

		const bool completed =
			stage(Stage::Audio, [&]
			{
				soundBuffers.Load(Assets::SoundID::TitleButtonDrop, Paths::Sounds::TitleButtonDrop);

				// Not shipped yet -- load only if present.
				if (std::filesystem::exists(Paths::Sounds::MenuEntrySwoosh))
				{
					soundBuffers.Load(Assets::SoundID::MenuEntrySwoosh, Paths::Sounds::MenuEntrySwoosh);
				}

				soundBuffers.Load(Assets::SoundID::MenuItemSelected, Paths::Sounds::MenuItemSelected);
				soundBuffers.Load(Assets::SoundID::MenuItemPressed, Paths::Sounds::MenuItemPressed);
				soundBuffers.Load(Assets::SoundID::DropPiece, Paths::Sounds::DropPiece);
				soundBuffers.Load(Assets::SoundID::MovePiece, Paths::Sounds::MovePiece);
				soundBuffers.Load(Assets::SoundID::RotatePiece, Paths::Sounds::RotatePiece);
				soundBuffers.Load(Assets::SoundID::PieceHitWall, Paths::Sounds::PieceHitWall);
				soundBuffers.Load(Assets::SoundID::NextLevel, Paths::Sounds::NextLevel);
				soundBuffers.Load(Assets::SoundID::RowCleared, Paths::Sounds::RowCleared);
			})
			&& stage(Stage::Music, [&]
			{
				// MainMenu (the shell track) is loaded synchronously by
				// Application so the loading screen can start it immediately.
				music.Load(Assets::MusicID::Gameplay, Paths::Music::Gameplay);
				music.Load(Assets::MusicID::GameOver, Paths::Music::GameOver);
			})
			&& stage(Stage::Interface, [&]
			{
				fonts.Load(Assets::FontID::Main, Paths::Fonts::Main);
				fonts.Load(Assets::FontID::Menu, Paths::Fonts::Menu);
			});

		if (completed)
		{
			progress.MarkDone();
		}
	}
}
