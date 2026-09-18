#include "SettingsManager.h"

#include <fstream>
#include <system_error>
#include <type_traits>

#include <nlohmann/json.hpp>

#include "../audio/AudioPlayer.h"
#include "../core/Context.h"
#include "../input/gamepad/GamepadHaptics.h"
#include "../utils/SafeFileWrite.h"

namespace
{
	using Json = nlohmann::json;

	// Reads `data[key]` into `out` if present and of the right JSON type;
	// otherwise leaves `out` untouched and reports failure so the caller can
	// bail out to defaults instead of adopting a half-parsed settings file.
	template <typename T>
	[[nodiscard]] bool ReadField(const Json& data, const char* key, T& out)
	{
		const auto field = data.find(key);
		if (field == data.end())
			return false;

		if constexpr (std::is_same_v<T, bool>)
		{
			if (!field->is_boolean())
				return false;
		}
		else
		{
			if (!field->is_number_integer())
				return false;
		}

		out = field->get<T>();
		return true;
	}

	[[nodiscard]] bool ReadScancode(const Json& data, const char* key, sf::Keyboard::Scancode& out)
	{
		int value = 0;
		if (!ReadField(data, key, value) || value < 0 || value >= static_cast<int>(sf::Keyboard::ScancodeCount))
			return false;

		out = static_cast<sf::Keyboard::Scancode>(value);
		return true;
	}
}

SettingsManager::SettingsManager(const std::filesystem::path& filepath)
	: filepath(filepath)
{}

// File layout is JSON, grouped the same way GameSettings.h is:
//   {
//     "formatVersion": <int>,
//     "graphics": { "verticalSyncEnabled", "showFPS", "crtFilterEnabled", "windowMode", "resolutionWidth", "resolutionHeight" },
//     "audio": { "soundVolume", "musicVolume" },
//     "controls": { "moveTetrominoLeft", "moveTetrominoRight", "softDropTetromino", "hardDropTetromino",
//                   "rotateTetrominoClockwise", "rotateTetrominoCounterClockwise", "holdTetromino" },
//     "gameplay": { "gamepadVibrationEnabled", "gamepadLightbarEnabled", "screenShakeEnabled",
//                   "ghostPieceEnabled", "holdTetrominoEnabled", "nextQueueLength", "sevenBagEnabled" },
//     "hud": { "holdTetrominoPanelVisible", "nextTetrominoPanelVisible", "scorePanelVisible",
//              "linesPanelVisible", "levelPanelVisible", "timePanelVisible", "controlsLegendPanelVisible" },
//     "language": { "value", "chosen" }
//   }
// `pause` is fixed to Escape and is never part of "controls" -- see ControlSettings' own comment.
void SettingsManager::Load()
{
	std::ifstream file(filepath);

	if (!file.is_open())
	{
		// First run -- write the defaults `settings` already holds.
		Save();
		return;
	}

	Json data;

	try
	{
		data = Json::parse(file);
	}
	catch (const Json::exception&)
	{
		file.close();

		static_cast<void>(SafeFileWrite::PreserveCorruptFile(filepath));
		Save();

		return;
	}

	// Parse into a scratch copy. Only adopt it if the format version matches,
	// every field reads, and every value is in range; otherwise the file is
	// kept as .corrupt and replaced with defaults.
	const auto formatVersion = data.find("formatVersion");

	const Json& graphics = data.value("graphics", Json::object());
	const Json& audio = data.value("audio", Json::object());
	const Json& controls = data.value("controls", Json::object());
	const Json& gameplay = data.value("gameplay", Json::object());
	const Json& HUD = data.value("hud", Json::object());
	const Json& language = data.value("language", Json::object());

	GameSettings parsedSettings;
	int windowModeValue = 0;
	unsigned int resolutionWidth = 0;
	unsigned int resolutionHeight = 0;
	int languageValue = 0;

	const bool areFieldsRead =
		formatVersion != data.end() && formatVersion->is_number_integer() &&
		ReadField(graphics, "verticalSyncEnabled", parsedSettings.isVerticalSyncEnabled) &&
		ReadField(graphics, "showFPS", parsedSettings.needToShowFPS) &&
		ReadField(graphics, "crtFilterEnabled", parsedSettings.isCRTFilterEnabled) &&
		ReadField(graphics, "windowMode", windowModeValue) &&
		ReadField(graphics, "resolutionWidth", resolutionWidth) &&
		ReadField(graphics, "resolutionHeight", resolutionHeight) &&
		ReadField(audio, "soundVolume", parsedSettings.soundVolume) &&
		ReadField(audio, "musicVolume", parsedSettings.musicVolume) &&
		ReadScancode(controls, "moveTetrominoLeft", parsedSettings.controls.moveTetrominoLeft) &&
		ReadScancode(controls, "moveTetrominoRight", parsedSettings.controls.moveTetrominoRight) &&
		ReadScancode(controls, "softDropTetromino", parsedSettings.controls.softDropTetromino) &&
		ReadScancode(controls, "hardDropTetromino", parsedSettings.controls.hardDropTetromino) &&
		ReadScancode(controls, "rotateTetrominoClockwise", parsedSettings.controls.rotateTetrominoClockwise) &&
		ReadScancode(controls, "rotateTetrominoCounterClockwise", parsedSettings.controls.rotateTetrominoCounterClockwise) &&
		ReadScancode(controls, "holdTetromino", parsedSettings.controls.holdTetromino) &&
		ReadField(gameplay, "gamepadVibrationEnabled", parsedSettings.isGamepadVibrationEnabled) &&
		ReadField(gameplay, "gamepadLightbarEnabled", parsedSettings.isGamepadLightbarEnabled) &&
		ReadField(gameplay, "screenShakeEnabled", parsedSettings.isScreenShakeEnabled) &&
		ReadField(gameplay, "ghostPieceEnabled", parsedSettings.isGhostPieceEnabled) &&
		ReadField(gameplay, "holdTetrominoEnabled", parsedSettings.isHoldTetrominoEnabled) &&
		ReadField(gameplay, "nextQueueLength", parsedSettings.nextQueueLength) &&
		ReadField(gameplay, "sevenBagEnabled", parsedSettings.isSevenBagEnabled) &&
		ReadField(HUD, "holdTetrominoPanelVisible", parsedSettings.isHoldTetrominoPanelVisible) &&
		ReadField(HUD, "nextTetrominoPanelVisible", parsedSettings.isNextTetrominoPanelVisible) &&
		ReadField(HUD, "scorePanelVisible", parsedSettings.isScorePanelVisible) &&
		ReadField(HUD, "linesPanelVisible", parsedSettings.isLinesPanelVisible) &&
		ReadField(HUD, "levelPanelVisible", parsedSettings.isLevelPanelVisible) &&
		ReadField(HUD, "timePanelVisible", parsedSettings.isTimePanelVisible) &&
		ReadField(HUD, "controlsLegendPanelVisible", parsedSettings.isControlsLegendPanelVisible) &&
		ReadField(language, "value", languageValue) &&
		ReadField(language, "chosen", parsedSettings.isLanguageChosen);

	const bool isFileUsable =
		areFieldsRead &&
		formatVersion->get<int>() == GameSettings::FormatVersion &&
		windowModeValue >= 0 && windowModeValue < static_cast<int>(Display::WindowModeCount) &&
		parsedSettings.soundVolume <= MaxVolumeStep &&
		parsedSettings.musicVolume <= MaxVolumeStep &&
		parsedSettings.nextQueueLength >= MinNextQueueLength && parsedSettings.nextQueueLength <= MaxNextQueueLength &&
		languageValue >= 0 && languageValue < static_cast<int>(LanguageCount);

	if (!isFileUsable)
	{
		file.close();

		static_cast<void>(SafeFileWrite::PreserveCorruptFile(filepath));
		Save();

		return;
	}

	parsedSettings.display.windowMode = static_cast<Display::WindowMode>(windowModeValue);
	parsedSettings.display.resolution = { resolutionWidth, resolutionHeight };
	parsedSettings.language = static_cast<Language>(languageValue);
	settings = parsedSettings;
}

// Mirrors Load's layout -- see the comment above it.
void SettingsManager::Save() const
{
	std::error_code error;
	std::filesystem::create_directories(filepath.parent_path(), error);

	std::filesystem::path temporaryPath(filepath);
	temporaryPath += ".tmp";

	std::ofstream file(temporaryPath, std::ios::trunc);
	if (!file.is_open())
		return;

	Json data;

	data["formatVersion"] = GameSettings::FormatVersion;

	data["graphics"] =
	{
		{ "verticalSyncEnabled", settings.isVerticalSyncEnabled },
		{ "showFPS", settings.needToShowFPS },
		{ "crtFilterEnabled", settings.isCRTFilterEnabled },
		{ "windowMode", static_cast<int>(settings.display.windowMode) },
		{ "resolutionWidth", settings.display.resolution.x },
		{ "resolutionHeight", settings.display.resolution.y },
	};

	data["audio"] =
	{
		{ "soundVolume", settings.soundVolume },
		{ "musicVolume", settings.musicVolume },
	};

	data["controls"] =
	{
		{ "moveTetrominoLeft", static_cast<int>(settings.controls.moveTetrominoLeft) },
		{ "moveTetrominoRight", static_cast<int>(settings.controls.moveTetrominoRight) },
		{ "softDropTetromino", static_cast<int>(settings.controls.softDropTetromino) },
		{ "hardDropTetromino", static_cast<int>(settings.controls.hardDropTetromino) },
		{ "rotateTetrominoClockwise", static_cast<int>(settings.controls.rotateTetrominoClockwise) },
		{ "rotateTetrominoCounterClockwise", static_cast<int>(settings.controls.rotateTetrominoCounterClockwise) },
		{ "holdTetromino", static_cast<int>(settings.controls.holdTetromino) },
	};

	data["gameplay"] =
	{
		{ "gamepadVibrationEnabled", settings.isGamepadVibrationEnabled },
		{ "gamepadLightbarEnabled", settings.isGamepadLightbarEnabled },
		{ "screenShakeEnabled", settings.isScreenShakeEnabled },
		{ "ghostPieceEnabled", settings.isGhostPieceEnabled },
		{ "holdTetrominoEnabled", settings.isHoldTetrominoEnabled },
		{ "nextQueueLength", settings.nextQueueLength },
		{ "sevenBagEnabled", settings.isSevenBagEnabled },
	};

	data["hud"] =
	{
		{ "holdTetrominoPanelVisible", settings.isHoldTetrominoPanelVisible },
		{ "nextTetrominoPanelVisible", settings.isNextTetrominoPanelVisible },
		{ "scorePanelVisible", settings.isScorePanelVisible },
		{ "linesPanelVisible", settings.isLinesPanelVisible },
		{ "levelPanelVisible", settings.isLevelPanelVisible },
		{ "timePanelVisible", settings.isTimePanelVisible },
		{ "controlsLegendPanelVisible", settings.isControlsLegendPanelVisible },
	};

	data["language"] =
	{
		{ "value", static_cast<int>(settings.language) },
		{ "chosen", settings.isLanguageChosen },
	};

	file << data.dump(1, '\t');
	file.close();

	static_cast<void>(SafeFileWrite::ReplaceFileAtomically(temporaryPath, filepath));
}

void SettingsManager::Apply(Context& context) const
{
	// --- Graphics settings ---
	// The window mode / resolution are applied by Application (they recreate the
	// window); this only touches per-window toggles.
	context.window.setVerticalSyncEnabled(settings.isVerticalSyncEnabled);

	// --- Audio settings ---
	// Music volume is applied every frame by MusicPlayer::Update() instead
	// (Application::Update ticks it unconditionally), so it always reflects
	// the current slider without needing an explicit Apply() here.

	// The sound slider is stored on the AudioPlayer; per-sound balance is
	// applied there per instance.
	context.audioPlayer.SetGlobalVolume(settings.soundVolume * 10.f);

	// --- Gameplay settings ---
	// The gamepad feedback toggles take effect at once (even in the menus);
	// screen shake is read by GameplayState when a game starts.
	context.gamepadHaptics.SetVibrationEnabled(settings.isGamepadVibrationEnabled);
	context.gamepadHaptics.SetLightbarEnabled(settings.isGamepadLightbarEnabled);
}

GameSettings& SettingsManager::GetSettings()
{
	return settings;
}

const GameSettings& SettingsManager::GetSettings() const
{
	return settings;
}
