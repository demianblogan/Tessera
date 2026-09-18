#pragma once

namespace Assets
{
	enum class FontID
	{
		Main,
		Loading,
		Menu,
		MenuList
	};

	enum class MusicID
	{
		MainMenu,
		Gameplay1,
		Gameplay2,
		Gameplay3,

		Count
	};

	inline constexpr int MusicIDCount = static_cast<int>(MusicID::Count);

	enum class SoundID
	{
		TitleButtonDrop,
		MenuItemAppeared,

		MenuItemSelected,
		MenuItemPressed,

		DropPiece,
		MovePiece,
		NextLevel,
		PieceHitWall,
		RotatePiece,
		RowCleared,

		GameOver,

		Count
	};

	inline constexpr int SoundIDCount = static_cast<int>(SoundID::Count);

	enum class TextureID
	{
		BlockSpritesheetWithOutline,
		MenuBackground,
		GameplayBackground,
		CompanyLogo,
		Cursor,
		UiArrow,
		UiFrameCyan,
		UiFrameBlue,
		UiFrameGreen,
		UiFramePurple,
		UiFrameBrown,
		UiFrameRed,
		UiFrameWhiteRed,
		UiFrameWarning,
		CarouselArrow,
		Checkbox,
		XboxGamepadLayout,
		PlayStationGamepadLayout
	};

	enum class ShaderID
	{
		CRT,
		GhostTetromino,
		NeonDilate,
		NeonBlur,
		MenuAurora,
		Mosaic
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

			// Vertical menu lists (Options categories, later the in-game menus).
			inline constexpr const char* MenuList = "assets/fonts/chispix-bold.ttf";
		}

		namespace Music
		{
			inline constexpr const char* MainMenu = "assets/audio/music/main_menu_music.ogg";

			// Shuffled and looped by MusicPlayer while a game is in progress.
			inline constexpr const char* Gameplay1 = "assets/audio/music/gameplay_music_1.mp3";
			inline constexpr const char* Gameplay2 = "assets/audio/music/gameplay_music_2.mp3";
			inline constexpr const char* Gameplay3 = "assets/audio/music/gameplay_music_3.mp3";
		}

		namespace Sounds
		{
			inline constexpr const char* TitleButtonDrop = "assets/audio/sounds/title_button_drop.ogg";
			inline constexpr const char* MenuItemAppeared = "assets/audio/sounds/menu_item_appeared.mp3";

			inline constexpr const char* MenuItemSelected = "assets/audio/sounds/menu_item_selected.ogg";
			inline constexpr const char* MenuItemPressed = "assets/audio/sounds/menu_item_pressed.ogg";

			inline constexpr const char* DropPiece = "assets/audio/sounds/drop_piece.ogg";
			inline constexpr const char* MovePiece = "assets/audio/sounds/move_piece.ogg";
			inline constexpr const char* NextLevel = "assets/audio/sounds/next_level.ogg";
			inline constexpr const char* PieceHitWall = "assets/audio/sounds/piece_hit_wall.ogg";
			inline constexpr const char* RotatePiece = "assets/audio/sounds/rotate_piece.ogg";
			inline constexpr const char* RowCleared = "assets/audio/sounds/row_cleared.ogg";

			// Plays once, on top of the still-playing (ducked) gameplay music.
			inline constexpr const char* GameOver = "assets/audio/sounds/game_over.mp3";
		}

		namespace Textures
		{
			inline constexpr const char* BlockSpritesheetWithOutline = "assets/textures/block_spritesheet_with_outline.png";
			inline constexpr const char* MenuBackground = "assets/textures/menu_background.png";
			inline constexpr const char* GameplayBackground = "assets/textures/gameplay_background.jpg";
			inline constexpr const char* CompanyLogo = "assets/other/alone_bull_splash_logo.jpg";
			inline constexpr const char* Cursor = "assets/textures/cursor.png";
			inline constexpr const char* UiArrow = "assets/textures/ui/arrow.png";

			// Per-menu 9-slice frames (62x62 source, decorative border
			// ~MenuFrameSourceBorder px) -- one hue per Options category /
			// screen, matching that screen's accent. Gold is the warning dialog.
			inline constexpr const char* UiFrameCyan = "assets/textures/ui/menu_background_cyan_frame.png";
			inline constexpr const char* UiFrameBlue = "assets/textures/ui/menu_background_blue_frame.png";
			inline constexpr const char* UiFrameGreen = "assets/textures/ui/menu_background_green_frame.png";
			inline constexpr const char* UiFramePurple = "assets/textures/ui/menu_background_purple_frame.png";
			inline constexpr const char* UiFrameBrown = "assets/textures/ui/menu_background_brown_frame.png";
			inline constexpr const char* UiFrameRed = "assets/textures/ui/menu_background_red_frame.png";
			inline constexpr const char* UiFrameWhiteRed = "assets/textures/ui/menu_background_white_red_frame.png";
			inline constexpr const char* UiFrameWarning = "assets/textures/ui/menu_background_gold_frame.png";

			// Settings widgets.
			inline constexpr const char* CarouselArrow = "assets/textures/ui/carusel_arrow.png";
			inline constexpr const char* Checkbox = "assets/textures/ui/checkbox.png";   // two 26x26 sprites at x=0 and x=28

			// Button-prompt atlases for the read-only Controls > Gamepad table.
			inline constexpr const char* XboxGamepadLayout = "assets/textures/ui/xbox_gamepad_layout.png";
			inline constexpr const char* PlayStationGamepadLayout = "assets/textures/ui/ps_gamepad_layout.png";
		}

		namespace Data
		{
			inline constexpr const char* LocalizationDir = "assets/data/localization";

			// Shown by the first-run language picker, before any language is
			// chosen -- one line per language, so it has to live outside the
			// per-language catalogs. Plain UTF-8 text file (no key=value), read
			// and decoded the same way LocalizationManager reads its catalogs.
			inline constexpr const char* LanguagePickerPrompt = "assets/data/localization/language_picker_prompt.txt";

			// Authored content, loaded once at startup -- see Application::Application.
			inline constexpr const char* Pieces = "assets/data/pieces.json";
			inline constexpr const char* SrsKicks = "assets/data/srs_kicks.json";
			inline constexpr const char* AudioBalance = "assets/data/audio_balance.json";
			inline constexpr const char* Haptics = "assets/data/haptics.json";
		}

		namespace Shaders
		{
			inline constexpr const char* CRT = "assets/shaders/crt.frag";
			inline constexpr const char* GhostTetromino = "assets/shaders/ghost_tetromino.frag";
			inline constexpr const char* NeonDilate = "assets/shaders/neon_dilate.frag";
			inline constexpr const char* NeonBlur = "assets/shaders/neon_blur.frag";
			inline constexpr const char* MenuAurora = "assets/shaders/menu_aurora.frag";
			inline constexpr const char* Mosaic = "assets/shaders/mosaic.frag";
		}
	}
}

// Bare file names for the per-player save files. AppDataPath::Resolve() turns
// each into a full path under %LOCALAPPDATA%.
namespace SaveFile
{
	inline constexpr const char* Settings = "settings.json";
	inline constexpr const char* Scores = "scores.json";
}
