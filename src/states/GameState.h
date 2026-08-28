#pragma once

#include <SFML/Graphics/Sprite.hpp>

#include "../core/State.h"
#include "../core/Context.h"
#include "../game/Board.h"
#include "../game/Tetromino.h"
#include "../game/TetrominoBag.h"
#include "../ui/Label.h"
#include "../ui/Layout.h"
#include "../ui/Panel.h"

class GameState : public State
{
private:
	struct ClearRowEffect
	{
		int row = 0;
		float timer = 0.f;
	};

	enum class Phase
	{
		Falling,       // a piece is in play and responds to input
		ClearingRows   // full rows found; the clear animation is running, input is ignored
	};

	static constexpr float BLOCK_SIZE = 48.f;
	static constexpr int WALL_TEXTURE_INDEX = 10;
	static constexpr sf::Vector2f BOARD_POSITION{ 720.f, 60.f };
	static constexpr int SCORE_PER_LEVEL = 50;
	static constexpr float LANDING_EFFECT_DURATION = 0.15f;
	static constexpr float CLEAR_ROW_EFFECT_DURATION = 0.45f;

	Context& context;

	Board board;
	TetrominoBag tetrominoBag;
	Tetromino currentTetromino;

	Tetromino nextTetromino;
	sf::Vector2f nextTetrominoPreviewPosition;

	Phase phase = Phase::Falling;

	float fallTimer = 0.f;
	float fallDelay = 0.5f;

	int score = 0;

	std::unique_ptr<UI::Layout> rightHudLayout;
	std::unique_ptr<UI::Panel> controlsPanel;

	UI::Label* nextTetrominoLabel = nullptr;
	UI::Label* scoreLabel = nullptr;
	UI::Label* levelLabel = nullptr;

	int level = 1;

	sf::Sprite backgroundSprite;

	float shakeTimer = 0.f;
	float shakeDuration = 0.f;
	float shakeIntensity = 0.f;
	sf::Vector2f shakeOffset{ 0.f, 0.f };

	std::array<sf::Vector2i, 4> landingEffectBlocks;
	float landingEffectTimer = 0.f;

	std::vector<ClearRowEffect> clearRowEffects;

	void StartScreenShake(float duration, float intensity);

	bool SpawnTetromino();
	void TryMoveTetromino(int offsetX, int offsetY);
	void TryRotateTetromino();
	void TryDropTetromino();
	void HandleTetrominoLanding();

	[[nodiscard]] Tetromino GetGhostTetromino() const;
	[[nodiscard]] Assets::TextureID GetBlockTextureID() const;

public:
	explicit GameState(Context& context);

	[[nodiscard]] StateId GetId() const override { return StateId::Game; }

	void ProcessEvents(sf::RenderWindow& window) override;
	void Update(float deltaTime) override;
	void Render(sf::RenderTarget& target) override;
};