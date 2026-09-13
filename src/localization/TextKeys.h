#pragma once

#include <string_view>

// Every localization key the game asks for, in one place. Call sites use these
// instead of bare strings so a typo is a compile error and the full set of
// on-screen text is discoverable from here.
namespace TextKey
{
	namespace MainMenu
	{
		inline constexpr std::string_view Title   = "main_menu.title";
		inline constexpr std::string_view Play    = "main_menu.play";
		inline constexpr std::string_view Options = "main_menu.options";
		inline constexpr std::string_view Records = "main_menu.records";
		inline constexpr std::string_view Credits = "main_menu.credits";
		inline constexpr std::string_view Quit    = "main_menu.quit";
	}

	namespace Pause
	{
		inline constexpr std::string_view Title    = "pause.title";
		inline constexpr std::string_view Resume   = "pause.resume";
		inline constexpr std::string_view Restart  = "pause.restart";
		inline constexpr std::string_view Options  = "pause.options";
		inline constexpr std::string_view MainMenu = "pause.main_menu";

		inline constexpr std::string_view ConfirmRestart = "pause.confirm_restart";
		inline constexpr std::string_view ConfirmQuit    = "pause.confirm_quit";
	}

	namespace GameOver
	{
		inline constexpr std::string_view Title     = "game_over.title";
		inline constexpr std::string_view Score     = "game_over.score";
		inline constexpr std::string_view Lines     = "game_over.lines";
		inline constexpr std::string_view Level     = "game_over.level";
		inline constexpr std::string_view Time      = "game_over.time";
		inline constexpr std::string_view NewRecord  = "game_over.new_record";   // + " #N" in code
		inline constexpr std::string_view EnterName  = "game_over.enter_name";
		inline constexpr std::string_view SaveRecord = "game_over.save_record";
		inline constexpr std::string_view Saved      = "game_over.saved";
		inline constexpr std::string_view UnsavedRecord = "game_over.unsaved_record";
		inline constexpr std::string_view PlayAgain  = "game_over.play_again";
		inline constexpr std::string_view MainMenu   = "game_over.main_menu";
	}

	namespace Options
	{
		inline constexpr std::string_view Title    = "options.title";
		inline constexpr std::string_view Gameplay = "options.gameplay";
		inline constexpr std::string_view Hud      = "options.hud";
		inline constexpr std::string_view Graphics = "options.graphics";
		inline constexpr std::string_view Audio    = "options.audio";
		inline constexpr std::string_view Controls = "options.controls";
		inline constexpr std::string_view Language = "options.language";
		inline constexpr std::string_view Back     = "options.back";

		inline constexpr std::string_view ControlsKeyboard = "options.controls_keyboard";
		inline constexpr std::string_view ControlsGamepad  = "options.controls_gamepad";
		inline constexpr std::string_view ControlsBack     = "options.controls_back";

		inline constexpr std::string_view GameplayVibration = "options.gameplay_vibration";
		inline constexpr std::string_view GameplayLightbar  = "options.gameplay_lightbar";
		inline constexpr std::string_view GameplayShake     = "options.gameplay_shake";
		inline constexpr std::string_view GameplayGhost      = "options.gameplay_ghost";
		inline constexpr std::string_view GameplayHold       = "options.gameplay_hold";
		inline constexpr std::string_view GameplayNextLength = "options.gameplay_next_length";
		inline constexpr std::string_view GameplayRandomiser = "options.gameplay_randomiser";
		inline constexpr std::string_view RandomiserSevenBag = "options.randomiser_seven_bag";
		inline constexpr std::string_view RandomiserRandom   = "options.randomiser_random";
		inline constexpr std::string_view RestartToApply     = "options.restart_to_apply";

		inline constexpr std::string_view HudHold           = "options.hud_hold";
		inline constexpr std::string_view HudNext           = "options.hud_next";
		inline constexpr std::string_view HudScore          = "options.hud_score";
		inline constexpr std::string_view HudLines          = "options.hud_lines";
		inline constexpr std::string_view HudLevel          = "options.hud_level";
		inline constexpr std::string_view HudTime           = "options.hud_time";
		inline constexpr std::string_view HudControlsLegend = "options.hud_controls_legend";

		inline constexpr std::string_view LanguageEnglish   = "options.language_english";
		inline constexpr std::string_view LanguageSpanish   = "options.language_spanish";
		inline constexpr std::string_view LanguageGerman    = "options.language_german";
		inline constexpr std::string_view LanguageRussian   = "options.language_russian";
		inline constexpr std::string_view LanguageUkrainian = "options.language_ukrainian";
		inline constexpr std::string_view LanguageSelected  = "options.language_selected";

		inline constexpr std::string_view KeyMoveLeft  = "options.key_move_left";
		inline constexpr std::string_view KeyMoveRight = "options.key_move_right";
		inline constexpr std::string_view KeySoftDrop  = "options.key_soft_drop";
		inline constexpr std::string_view KeyHardDrop  = "options.key_hard_drop";
		inline constexpr std::string_view KeyRotateCw  = "options.key_rotate_cw";
		inline constexpr std::string_view KeyRotateCcw = "options.key_rotate_ccw";
		inline constexpr std::string_view KeyHold      = "options.key_hold";

		inline constexpr std::string_view GamepadXbox        = "options.gamepad_xbox";
		inline constexpr std::string_view GamepadPlayStation = "options.gamepad_playstation";

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
		inline constexpr std::string_view Title       = "credits.title";
		inline constexpr std::string_view Intro       = "credits.intro";
		inline constexpr std::string_view Blurb       = "credits.blurb";   // one multi-line block
		inline constexpr std::string_view Email       = "credits.email";
		inline constexpr std::string_view LinkedIn    = "credits.linkedin";
		inline constexpr std::string_view Instagram   = "credits.instagram";
		inline constexpr std::string_view Code        = "credits.code";
		inline constexpr std::string_view Portfolio   = "credits.portfolio";
		inline constexpr std::string_view Programming = "credits.programming";
		inline constexpr std::string_view Gaming      = "credits.gaming";
		inline constexpr std::string_view Back        = "credits.back";
	}

	namespace Records
	{
		inline constexpr std::string_view Title        = "records.title";
		inline constexpr std::string_view HeaderName   = "records.header_name";
		inline constexpr std::string_view HeaderScore  = "records.header_score";
		inline constexpr std::string_view HeaderLines  = "records.header_lines";
		inline constexpr std::string_view HeaderLevel  = "records.header_level";
		inline constexpr std::string_view Reset        = "records.reset";
		inline constexpr std::string_view Back         = "records.back";
		inline constexpr std::string_view ConfirmReset = "records.confirm_reset";
	}

	namespace Loading
	{
		// The whole load finishes in about a second -- one label for all of it
		// reads far better than per-stage text nobody has time to actually read.
		inline constexpr std::string_view Label = "loading.label";
	}

	namespace Hud
	{
		inline constexpr std::string_view Hold  = "hud.hold";
		inline constexpr std::string_view Next  = "hud.next";
		inline constexpr std::string_view Score = "hud.score";
		inline constexpr std::string_view Lines = "hud.lines";
		inline constexpr std::string_view Level = "hud.level";
		inline constexpr std::string_view Time  = "hud.time";

		inline constexpr std::string_view Move     = "hud.move";
		inline constexpr std::string_view Rotate   = "hud.rotate";
		inline constexpr std::string_view SoftDrop = "hud.soft_drop";
		inline constexpr std::string_view HardDrop = "hud.hard_drop";
		inline constexpr std::string_view HoldKey  = "hud.hold_key";
		inline constexpr std::string_view Pause    = "hud.pause";
	}

	// On-board popups for scoring highlights; short fragments joined in code
	// (e.g. "T-SPIN" + "DOUBLE", "BACK-TO-BACK" + "TETRIS").
	namespace Callout
	{
		inline constexpr std::string_view Single       = "callout.single";
		inline constexpr std::string_view Double       = "callout.double";
		inline constexpr std::string_view Triple       = "callout.triple";
		inline constexpr std::string_view Tetris       = "callout.tetris";
		inline constexpr std::string_view TSpin        = "callout.t_spin";
		inline constexpr std::string_view TSpinMini    = "callout.t_spin_mini";
		inline constexpr std::string_view BackToBack   = "callout.back_to_back";
		inline constexpr std::string_view PerfectClear = "callout.perfect_clear";
		inline constexpr std::string_view Combo        = "callout.combo";   // + "xN " prefix in code

		// Escalation (see EscalationDirector).
		inline constexpr std::string_view SpeedSurge = "callout.speed_surge";
		inline constexpr std::string_view Golden     = "callout.golden";
	}
}
