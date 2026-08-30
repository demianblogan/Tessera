#pragma once

namespace Assets
{
	enum class FontID
	{
		Main,
		Loading,
		Menu
	};

	enum class MusicID
	{
		MainMenu,
		Gameplay,
		GameOver
	};

	enum class SoundID
	{
		MenuItemSelected,
		MenuItemPressed,

		DropPiece,
		MovePiece,
		NextLevel,
		PieceHitWall,
		RotatePiece,
		RowCleared
	};

	enum class TextureID
	{
		BlockSpritesheetWithOutline,
		BlockSpritesheetWithoutOutline,
		ButtonBackground,
		MenuBackground,
		PanelBackground,
		GameBackground,
		CompanyLogo,
		Cursor,
		UiArrow
	};

	enum class ShaderID
	{
		CRT,
		Blur,
		GhostTetromino,
		NeonDilate,
		NeonBlur,
		MenuAurora
	};

	namespace Paths
	{
		namespace Fonts
		{
			inline constexpr const char* Main = "assets/fonts/main.ttf";

			// Used only by the loading screen for now; broad language coverage
			// (Latin, Cyrillic, ...). Falls back to Main if the file is missing.
			inline constexpr const char* Loading = "assets/fonts/chis-pix.ttf";

			// Main-menu carousel entries. Pixel face with localisation coverage.
			inline constexpr const char* Menu = "assets/fonts/pixel.ttf";
		}

		namespace Music
		{
			inline constexpr const char* MainMenu = "assets/music/main_menu_music.ogg";
			inline constexpr const char* Gameplay = "assets/music/gameplay_music.ogg";
			inline constexpr const char* GameOver = "assets/music/game_over_music.ogg";
		}

		namespace Sounds
		{
			inline constexpr const char* MenuItemSelected = "assets/sounds/menu_item_selected.ogg";
			inline constexpr const char* MenuItemPressed = "assets/sounds/menu_item_pressed.ogg";

			inline constexpr const char* DropPiece = "assets/sounds/drop_piece.ogg";
			inline constexpr const char* MovePiece = "assets/sounds/move_piece.ogg";
			inline constexpr const char* NextLevel = "assets/sounds/next_level.ogg";
			inline constexpr const char* PieceHitWall = "assets/sounds/piece_hit_wall.ogg";
			inline constexpr const char* RotatePiece = "assets/sounds/rotate_piece.ogg";
			inline constexpr const char* RowCleared = "assets/sounds/row_cleared.ogg";
		}

		namespace Textures
		{
			inline constexpr const char* BlockSpritesheetWithOutline = "assets/textures/block_spritesheet_with_outline.png";
			inline constexpr const char* BlockSpritesheetWithoutOutline = "assets/textures/block_spritesheet_without_outline.png";
			inline constexpr const char* ButtonBackground = "assets/textures/button_background.png";
			inline constexpr const char* MenuBackground = "assets/textures/menu_background.png";
			inline constexpr const char* PanelBackground = "assets/textures/panel_background.png";
			inline constexpr const char* GameBackground = "assets/textures/game_background.png";
			inline constexpr const char* CompanyLogo = "assets/other/alone_bull_splash_logo.jpg";
			inline constexpr const char* Cursor = "assets/textures/cursor.png";
			inline constexpr const char* UiArrow = "assets/textures/ui/arrow.png";
		}

		namespace Data
		{
			inline constexpr const char* LocalizationDir = "assets/data/localization";
		}

		namespace Shaders
		{
			inline constexpr const char* CRT = "assets/shaders/crt.frag";
			inline constexpr const char* Blur = "assets/shaders/blur.frag";
			inline constexpr const char* GhostTetromino = "assets/shaders/ghost_tetromino.frag";
			inline constexpr const char* NeonDilate = "assets/shaders/neon_dilate.frag";
			inline constexpr const char* NeonBlur = "assets/shaders/neon_blur.frag";
			inline constexpr const char* MenuAurora = "assets/shaders/menu_aurora.frag";
		}
	}
}

// Bare file names for the per-player save files. AppDataPath::Resolve() turns
// each into a full path under %LOCALAPPDATA%.
namespace SaveFile
{
	inline constexpr const char* Settings = "settings.txt";
	inline constexpr const char* Scores = "scores.txt";
}