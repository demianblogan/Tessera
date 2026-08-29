#pragma once

#include <SFML/Graphics/RenderWindow.hpp>

#include "../resources/ResourceManager.h"
#include "../resources/ShaderManager.h"

class AudioPlayer;
class StateMachine;
class SettingsManager;
class HighScoreManager;
class GamepadManager;
class LocalizationManager;

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
    ShaderManager& shaders;
    GamepadManager& gamepad;
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
        SettingsManager& settings,
        HighScoreManager& highScores,
        GamepadManager& gamepad,
        LocalizationManager& localization
    );
};