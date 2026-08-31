#pragma once

#include <SFML/Graphics/RenderWindow.hpp>

#include "../resources/ResourceManager.h"
#include "../resources/ShaderManager.h"

class AudioPlayer;
class AudioBalance;
class HapticSettings;
class StateMachine;
class SettingsManager;
class HighScoreManager;
class GamepadManager;
class LocalizationManager;

namespace Haptics { class GamepadHaptics; }

struct Context
{
    StateMachine& stateMachine;

    sf::RenderWindow& window;

    FontManager& fonts;
    MusicManager& music;
    SoundBufferManager& soundBuffers;
    TextureManager& textures;
    SettingsManager& settings;
    HighScoreManager& highScores;
    AudioPlayer& audioPlayer;
    AudioBalance& audioBalance;
    HapticSettings& hapticSettings;
    ShaderManager& shaders;
    GamepadManager& gamepad;
    Haptics::GamepadHaptics& gamepadHaptics;
    LocalizationManager& localization;
    float totalTime = 0.f;

    Context(
        StateMachine& stateMachine,
        sf::RenderWindow& window,
        FontManager& fonts,
        MusicManager& music,
        SoundBufferManager& soundBuffers,
        TextureManager& textures,
        ShaderManager& shaders,
        AudioPlayer& audioPlayer,
        AudioBalance& audioBalance,
        HapticSettings& hapticSettings,
        SettingsManager& settings,
        HighScoreManager& highScores,
        GamepadManager& gamepad,
        Haptics::GamepadHaptics& gamepadHaptics,
        LocalizationManager& localization
    );
};