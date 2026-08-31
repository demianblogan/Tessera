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

	namespace Settings
	{
		inline constexpr std::string_view Title             = "settings.title";
		inline constexpr std::string_view SectionGraphics   = "settings.section_graphics";
		inline constexpr std::string_view SectionAudio      = "settings.section_audio";
		inline constexpr std::string_view FooterReturn      = "settings.footer_return";
		inline constexpr std::string_view VsyncOn           = "settings.vsync_on";
		inline constexpr std::string_view VsyncOff          = "settings.vsync_off";
		inline constexpr std::string_view ShowFpsOn         = "settings.show_fps_on";
		inline constexpr std::string_view ShowFpsOff        = "settings.show_fps_off";
		inline constexpr std::string_view FpsLimit          = "settings.fps_limit";
		inline constexpr std::string_view FpsUnlimited      = "settings.fps_unlimited";
		inline constexpr std::string_view BlockStyleOutline = "settings.block_style_outline";
		inline constexpr std::string_view BlockStyleNoOutline = "settings.block_style_no_outline";
		inline constexpr std::string_view Sounds            = "settings.sounds";
		inline constexpr std::string_view Music             = "settings.music";
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
