#pragma once

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Shader.hpp>
#include <SFML/Graphics/Sprite.hpp>

#include "../audio/AudioPlayer.h"
#include "../resources/ResourceManager.h"
#include "../settings/SettingsManager.h"
#include "../statistics/HighScoreManager.h"
#include "../resources/ShaderManager.h"
#include "Context.h"
#include "StateMachine.h"

class Game
{
private:
	static constexpr sf::Vector2f VIRTUAL_RESOLUTION{ 1920.f, 1080.f };

	// Upper bound on the delta time handed to a single Update(). A stall
	// (window drag, minimize, debugger breakpoint, OS hiccup) would otherwise
	// deliver one huge delta and make the piece "teleport" several rows or a
	// timer expire instantly. Capping turns that frame into a brief hitch.
	static constexpr float MaxFrameTime = 0.1f;

	sf::RenderWindow window;

	sf::RenderTexture renderTexture;
	sf::RenderTexture gameplayTexture;
	sf::RenderTexture finalTexture;
	ShaderManager shaders;

	sf::View gameView;

	StateMachine stateMachine;

	FontManager fonts;
	MusicManager music;
	SoundBufferManager soundBuffers;
	TextureManager textures;
	SettingsManager settings;
	HighScoreManager highScores;

	AudioPlayer audioPlayer;

	Context context;

	bool IsWindowOpen() const;
	void ProcessEvents();
	void Update(float deltaTime);
	void Render();

public:
	Game();
	void Run();
};