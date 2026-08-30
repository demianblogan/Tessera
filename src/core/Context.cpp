#include "Context.h"

#include "../audio/AudioPlayer.h"
#include "../settings/SettingsManager.h"

Context::Context(
    StateMachine& stateMachine,
    sf::RenderWindow& window,
    FontManager& fonts,
    MusicManager& music,
    SoundBufferManager& soundBuffers,
    TextureManager& textures,
    ShaderManager& shaders,
    AudioPlayer& audioPlayer,
    AudioBalance& audioBalance,
    SettingsManager& settings,
    HighScoreManager& highScores,
    GamepadManager& gamepad,
    Haptics::GamepadHaptics& gamepadHaptics,
    LocalizationManager& localization
)
    : stateMachine(stateMachine)
    , window(window)
    , fonts(fonts)
    , music(music)
    , soundBuffers(soundBuffers)
    , textures(textures)
    , shaders(shaders)
    , audioPlayer(audioPlayer)
    , audioBalance(audioBalance)
    , settings(settings)
    , highScores(highScores)
    , gamepad(gamepad)
    , gamepadHaptics(gamepadHaptics)
    , localization(localization)
{
	// No code
}