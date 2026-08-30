#include "AssetLoadJob.h"

#include <SFML/Graphics/Shader.hpp>
#include <SFML/Window/Context.hpp>

#include "LoadingProgress.h"
#include "../resources/Assets.h"
#include "../resources/ShaderManager.h"

namespace Loading
{
	AssetLoadJob::AssetLoadJob(
		TextureManager& textures,
		SoundBufferManager& soundBuffers,
		MusicManager& music,
		ShaderManager& shaders,
		FontManager& fonts) noexcept
		: textures(textures)
		, soundBuffers(soundBuffers)
		, music(music)
		, shaders(shaders)
		, fonts(fonts)
	{
		// No code.
	}

	void AssetLoadJob::Run(std::stop_token stopToken, Progress& progress) const
	{
		// SFML needs an active OpenGL context on this thread before any
		// texture, shader or font upload; it shares GPU objects with the
		// window's context automatically.
		[[maybe_unused]] const sf::Context loadingContext;

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
			stage(Stage::Textures, [&]
			{
				textures.Load(Assets::TextureID::BlockSpritesheetWithOutline, Paths::Textures::BlockSpritesheetWithOutline);
				textures.Load(Assets::TextureID::BlockSpritesheetWithoutOutline, Paths::Textures::BlockSpritesheetWithoutOutline);
				textures.Load(Assets::TextureID::ButtonBackground, Paths::Textures::ButtonBackground);
				textures.Load(Assets::TextureID::MenuBackground, Paths::Textures::MenuBackground);
				textures.Load(Assets::TextureID::PanelBackground, Paths::Textures::PanelBackground);
				textures.Load(Assets::TextureID::GameBackground, Paths::Textures::GameBackground);
				textures.Load(Assets::TextureID::CompanyLogo, Paths::Textures::CompanyLogo);
				textures.Load(Assets::TextureID::Cursor, Paths::Textures::Cursor);
			})
			&& stage(Stage::Audio, [&]
			{
				soundBuffers.Load(Assets::SoundID::CompanySplash, Paths::Sounds::CompanySplash);
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
				music.Load(Assets::MusicID::MainMenu, Paths::Music::MainMenu);
				music.Load(Assets::MusicID::Gameplay, Paths::Music::Gameplay);
				music.Load(Assets::MusicID::GameOver, Paths::Music::GameOver);
			})
			&& stage(Stage::Shaders, [&]
			{
				// CRT and Blur are loaded synchronously before this screen so
				// the very first frame can composite -- they are not repeated
				// here.
				shaders.Load(Assets::ShaderID::GhostTetromino, Paths::Shaders::GhostTetromino, sf::Shader::Type::Fragment);
				shaders.Load(Assets::ShaderID::NeonDilate, Paths::Shaders::NeonDilate, sf::Shader::Type::Fragment);
				shaders.Load(Assets::ShaderID::NeonBlur, Paths::Shaders::NeonBlur, sf::Shader::Type::Fragment);
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
