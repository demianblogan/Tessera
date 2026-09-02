#pragma once

#include <string_view>

// Every localization key the game asks for, in one place. Call sites use these
// instead of bare strings so a typo is a compile error and the full set of
// on-screen text is discoverable from here.
namespace TextKey
{
	namespace MainMenu
	{
		inline constexpr std::string_view Title     = "main_menu.title";
		inline constexpr std::string_view StartGame = "main_menu.start_game";
		inline constexpr std::string_view Options      = "main_menu.options";
		inline constexpr std::string_view Records      = "main_menu.records";
		inline constexpr std::string_view Achievements = "main_menu.achievements";
		inline constexpr std::string_view Credits      = "main_menu.credits";
		inline constexpr std::string_view Quit         = "main_menu.quit";
	}

	namespace Pause
	{
		inline constexpr std::string_view Title    = "pause.title";
		inline constexpr std::string_view Resume   = "pause.resume";
		inline constexpr std::string_view Restart  = "pause.restart";
		inline constexpr std::string_view MainMenu = "pause.main_menu";
	}

	namespace GameOver
	{
		inline constexpr std::string_view Title     = "game_over.title";
		inline constexpr std::string_view Score     = "game_over.score";      // {score}
		inline constexpr std::string_view NewRecord = "game_over.new_record";
		inline constexpr std::string_view EnterName = "game_over.enter_name";
		inline constexpr std::string_view Save      = "game_over.save";
		inline constexpr std::string_view Restart   = "game_over.restart";
		inline constexpr std::string_view MainMenu  = "game_over.main_menu";
	}

	namespace Options
	{
		inline constexpr std::string_view Title    = "options.title";
		inline constexpr std::string_view Gameplay = "options.gameplay";
		inline constexpr std::string_view Graphics = "options.graphics";
		inline constexpr std::string_view Audio    = "options.audio";
		inline constexpr std::string_view Controls = "options.controls";
		inline constexpr std::string_view Language = "options.language";
		inline constexpr std::string_view Back     = "options.back";
		inline constexpr std::string_view ComingSoon = "options.coming_soon";

		inline constexpr std::string_view ControlsKeyboard = "options.controls_keyboard";
		inline constexpr std::string_view ControlsGamepad  = "options.controls_gamepad";
		inline constexpr std::string_view ControlsBack     = "options.controls_back";

		inline constexpr std::string_view Resolution     = "options.resolution";
		inline constexpr std::string_view WindowMode     = "options.window_mode";
		inline constexpr std::string_view Vsync          = "options.vsync";
		inline constexpr std::string_view ShowFps        = "options.show_fps";
		inline constexpr std::string_view CrtFilter      = "options.crt_filter";
		inline constexpr std::string_view ModeFullscreen = "options.mode_fullscreen";
		inline constexpr std::string_view ModeBorderless = "options.mode_borderless";
		inline constexpr std::string_view ModeWindow     = "options.mode_window";
		inline constexpr std::string_view Sound          = "options.sound";
		inline constexpr std::string_view Music          = "options.music";
		inline constexpr std::string_view Apply          = "options.apply";
		inline constexpr std::string_view Reset          = "options.reset";
		inline constexpr std::string_view BackButton     = "options.back_button";
		inline constexpr std::string_view Unsaved        = "options.unsaved";
		inline constexpr std::string_view BorderlessNote = "options.borderless_note";
	}

	namespace Common
	{
		inline constexpr std::string_view Yes = "common.yes";
		inline constexpr std::string_view No  = "common.no";
	}

	namespace Credits
	{
		inline constexpr std::string_view Title      = "credits.title";
		inline constexpr std::string_view Line1      = "credits.line_1";
		inline constexpr std::string_view Line2      = "credits.line_2";
		inline constexpr std::string_view Line3      = "credits.line_3";
		inline constexpr std::string_view Line4      = "credits.line_4";
		inline constexpr std::string_view Line5      = "credits.line_5";
		inline constexpr std::string_view Line6      = "credits.line_6";
		inline constexpr std::string_view Contact    = "credits.contact";
		inline constexpr std::string_view Email      = "credits.email";
		inline constexpr std::string_view YouTube    = "credits.youtube";
		inline constexpr std::string_view Source     = "credits.source";
		inline constexpr std::string_view Repository = "credits.repository";
		inline constexpr std::string_view Back       = "credits.back";
	}

	namespace Stats
	{
		inline constexpr std::string_view Title        = "stats.title";         // {count}
		inline constexpr std::string_view FooterReturn = "stats.footer_return";
		inline constexpr std::string_view FooterDelete = "stats.footer_delete";
		inline constexpr std::string_view Row          = "stats.row";           // {rank} {name} {score}
		inline constexpr std::string_view RowEmpty     = "stats.row_empty";     // {rank}
	}

	namespace Loading
	{
		inline constexpr std::string_view Audio     = "loading.audio";
		inline constexpr std::string_view Music     = "loading.music";
		inline constexpr std::string_view Interface = "loading.interface";
	}

	namespace Hud
	{
		inline constexpr std::string_view NextPiece = "hud.next_piece";
		inline constexpr std::string_view Score     = "hud.score";   // {score}
		inline constexpr std::string_view Level     = "hud.level";   // {level}
		inline constexpr std::string_view Controls  = "hud.controls";
	}
}
