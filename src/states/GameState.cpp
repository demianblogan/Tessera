#include "GameState.h"

#include <algorithm>

#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/RectangleShape.hpp>

#include "../audio/AudioPlayer.h"
#include "../resources/Assets.h"
#include "../core/Context.h"
#include "../core/StateMachine.h"
#include "../settings/SettingsManager.h"
#include "../settings/GameSettings.h"
#include "../utils/Random.h"
#include "MainMenuState.h"
#include "PauseState.h"
#include "GameOverState.h"

GameState::GameState(Context& context)
	: State(context.stateMachine)
	, context(context)
	, currentTetromino(tetrominoBag.Next(), { Board::WIDTH / 2 - 2, 0 })
	, nextTetromino(tetrominoBag.Next(), { 0, 0 })
	, backgroundSprite(context.textures.Get(Assets::TextureID::GameBackground))
{
	backgroundSprite.setColor(sf::Color(150, 150, 150));

	rightHudLayout = std::make_unique<UI::Layout>(UI::Layout::Orientation::Vertical);
	rightHudLayout->SetGap(32.f);

	sf::Sprite panelSprite(context.textures.Get(Assets::TextureID::PanelBackground));

	// =====================================================
	// Next tetromino panel
	// =====================================================
	{
		auto panel = std::make_unique<UI::Panel>(panelSprite);

		auto layout = std::make_unique<UI::Layout>(UI::Layout::Orientation::Vertical);
		layout->SetGap(20.f);

		auto label = std::make_unique<UI::Label>(context.fonts.Get(Assets::FontID::Main), "Next Tetromino", 60);
		label->SetFillColor(sf::Color::White);
		nextTetrominoLabel = label.get();
		layout->Add(std::move(label));

		panel->SetChild(std::move(layout));
		panel->SetWidthPixels(450.f);
		panel->SetHeightPixels(260.f);
		panel->SetPadding({ 60.f, 50.f });

		rightHudLayout->Add(std::move(panel));
	}

	// =====================================================
	// Score panel
	// =====================================================
	{
		auto panel = std::make_unique<UI::Panel>(panelSprite);

		auto layout = std::make_unique<UI::Layout>(UI::Layout::Orientation::Vertical);
		layout->SetGap(30.f);

		// =================================================
		// Score label
		// =================================================
		{
			auto label = std::make_unique<UI::Label>(context.fonts.Get(Assets::FontID::Main), "Score: 0", 60);
			label->SetFillColor(sf::Color::White);
			scoreLabel = label.get();
			layout->Add(std::move(label));
		}

		// =================================================
		// Level label
		// =================================================
		{
			auto label = std::make_unique<UI::Label>(context.fonts.Get(Assets::FontID::Main), "Level: 1", 60);
			label->SetFillColor(sf::Color::White);
			levelLabel = label.get();
			layout->Add(std::move(label));
		}

		panel->SetChild(std::move(layout));
		panel->SetWidthPixels(340.f);
		panel->SetHeightPixels(200.f);
		panel->SetPadding({ 60.f, 50.f });

		rightHudLayout->Add(std::move(panel));
	}

	// =====================================================
	// Controls panel
	// =====================================================

	sf::Sprite controlsSprite(context.textures.Get(Assets::TextureID::PanelBackground));

	controlsPanel = std::make_unique<UI::Panel>(controlsSprite);

	auto controlsLayout = std::make_unique<UI::Layout>(UI::Layout::Orientation::Vertical);

	controlsLayout->SetPadding(
		{
			.left = 80.f,
			.top = 50.f,
		}
		);

	sf::String controlsText =
		L"[←] [↓] [→] - Move tetromino\n"
		L"[↑] - Rotate tetromino\n"
		L"[Space] - Drop tetromino\n"
		L"[ESC] - Pause";

	auto controlsLabel = std::make_unique<UI::Label>(context.fonts.Get(Assets::FontID::Main), controlsText, 45);
	controlsLabel->SetFillColor(sf::Color::White);
	controlsLayout->Add(std::move(controlsLabel));

	controlsPanel->SetChild(std::move(controlsLayout));
	controlsPanel->SetWidthPixels(620.f);
	controlsPanel->SetHeightPixels(300.f);

	const sf::Vector2f rightHudSize = rightHudLayout->Measure();

	rightHudLayout->Arrange(
		{
			BOARD_POSITION.x + Board::WIDTH * BLOCK_SIZE + 100.f,
			BOARD_POSITION.y
		},
		rightHudSize
	);

	controlsPanel->Arrange(
		{
			10,
			BOARD_POSITION.y
		},
		{ 620.f, 300.f }
	);

	// Centre of the "Next Tetromino" panel's preview area: the panel sits at
	// (board right edge + 100) and is 450 wide, so its centre is +325; the
	// preview goes below the panel's title.
	nextTetrominoPreviewPosition =
	{
		BOARD_POSITION.x + Board::WIDTH * BLOCK_SIZE + 325.f,
		BOARD_POSITION.y + 185.f
	};

	context.music.Get(Assets::MusicID::MainMenu).stop();

	sf::Music& music = context.music.Get(Assets::MusicID::Gameplay);
	music.setLooping(true);

	if (music.getStatus() != sf::Music::Status::Playing)
	{
		music.play();
	}
}

void GameState::HandleEvent(const sf::Event& event)
{
	const auto* keyPressed = event.getIf<sf::Event::KeyPressed>();
	if (keyPressed == nullptr)
	{
		return;
	}

	if (keyPressed->scancode == sf::Keyboard::Scancode::Escape)
	{
		RequestPush(std::make_unique<PauseState>(context));
		return;
	}

	// Gameplay keys are ignored while rows are clearing -- the piece they would
	// act on is already locked and about to be replaced.
	if (phase != Phase::Falling)
	{
		return;
	}

	switch (keyPressed->scancode)
	{
	case sf::Keyboard::Scancode::Left:
		TryMoveTetromino(-1, 0);
		break;

	case sf::Keyboard::Scancode::Right:
		TryMoveTetromino(1, 0);
		break;

	case sf::Keyboard::Scancode::Down:
		TryMoveTetromino(0, 1);
		break;

	case sf::Keyboard::Scancode::Up:
		TryRotateTetromino();
		break;

	case sf::Keyboard::Scancode::Space:
		TryDropTetromino();
		break;

	default:
		break;
	}
}

void GameState::Update(float deltaTime)
{
	// =====================================================
	// Landing effect (keeps fading regardless of phase)
	// =====================================================

	if (landingEffectTimer > 0.f)
	{
		landingEffectTimer -= deltaTime;
	}

	// =====================================================
	// Clear row effect
	// =====================================================

	if (phase == Phase::ClearingRows)
	{
		for (ClearRowEffect& effect : clearRowEffects)
		{
			effect.timer += deltaTime;
		}

		bool finished = true;

		for (const ClearRowEffect& effect : clearRowEffects)
		{
			if (effect.timer < CLEAR_ROW_EFFECT_DURATION)
			{
				finished = false;
				break;
			}
		}

		if (finished)
		{
			std::vector<int> clearedRowIndices;
			for (const ClearRowEffect& effect : clearRowEffects)
			{
				clearedRowIndices.push_back(effect.row);
			}

			const int clearedRows = static_cast<int>(clearedRowIndices.size());
			board.ClearRows(clearedRowIndices);
			score += clearedRows * 10;

			const int previousLevel = level;
			level = score / SCORE_PER_LEVEL + 1;
			if (level > previousLevel)
			{
				context.audioPlayer.Play(Assets::SoundID::NextLevel);
			}

			fallDelay = std::max(0.1f, 0.5f - (level - 1) * 0.05f);

			scoreLabel->SetString("Score: " + std::to_string(score));
			levelLabel->SetString("Level: " + std::to_string(level));

			clearRowEffects.clear();
			phase = Phase::Falling;

			if (!SpawnTetromino())
			{
				RequestChange(std::make_unique<GameOverState>(context, score));
			}
		}

		return;
	}

	// =====================================================
	// Screen shake
	//
	// The random shake offset is computed here, in Update, and only *applied*
	// in Render. Rendering must stay a pure function of state: pulling random
	// numbers from the shared engine inside Render made frame output depend on
	// how many times Render happened to run and perturbed every other consumer
	// of Random.
	// =====================================================

	if (shakeTimer > 0.f)
	{
		shakeTimer -= deltaTime;

		const float progress = shakeDuration > 0.f ? std::max(0.f, shakeTimer / shakeDuration) : 0.f;
		const float currentIntensity = shakeIntensity * progress;

		shakeOffset =
		{
			Random::Float(-currentIntensity, currentIntensity),
			Random::Float(-currentIntensity, currentIntensity)
		};
	}
	else
	{
		shakeOffset = { 0.f, 0.f };
	}

	// =====================================================
	// Normal gameplay update
	// =====================================================

	fallTimer += deltaTime;

	if (fallTimer >= fallDelay)
	{
		fallTimer -= fallDelay;

		Tetromino movedTetromino = currentTetromino;

		movedTetromino.Move(0, 1);

		if (board.CanPlace(movedTetromino))
		{
			currentTetromino = movedTetromino;
		}
		else
		{
			HandleTetrominoLanding();
		}
	}
}

void GameState::Render(sf::RenderTarget& target)
{
	sf::View shakenView = target.getView();
	shakenView.move(shakeOffset);
	target.setView(shakenView);

	// =====================================================
	// Render background
	// =====================================================

	target.draw(backgroundSprite);

	// =====================================================
	// Render board background tiles
	// =====================================================

	constexpr int SPRITE_SIZE = 16;

	sf::Sprite blockSprite(context.textures.Get(GetBlockTextureID()));

	blockSprite.setScale(
		{
			BLOCK_SIZE / 16.f,
			BLOCK_SIZE / 16.f
		}
	);

	blockSprite.setTextureRect(
		{
			{ WALL_TEXTURE_INDEX * SPRITE_SIZE, 0 },
			{ SPRITE_SIZE, SPRITE_SIZE }
		}
	);

	for (int y = 0; y < Board::HEIGHT; y++)
	{
		// =================================================
		// Gradient
		// =================================================

		const float t =
			static_cast<float>(y)
			/ (Board::HEIGHT - 1);

		const int  brightness = static_cast<int>(6 + t * 18);

		// Холодный sci-fi оттенок
		blockSprite.setColor(
			sf::Color(
				brightness / 2,
				brightness,
				brightness + 20
			)
		);

		for (int x = 0; x < Board::WIDTH; x++)
		{
			blockSprite.setPosition(
				{
					BOARD_POSITION.x + x * BLOCK_SIZE,
					BOARD_POSITION.y + y * BLOCK_SIZE
				}
			);

			target.draw(blockSprite);
		}
	}

	blockSprite.setColor(sf::Color::White);

	// =====================================================
	// Render walls
	// =====================================================

	blockSprite.setTextureRect(
		{
			{ WALL_TEXTURE_INDEX * SPRITE_SIZE, 0 },
			{ SPRITE_SIZE, SPRITE_SIZE }
		}
	);

	for (int y = 0; y < Board::HEIGHT; y++)
	{
		// Left wall
		blockSprite.setPosition(
			{
				BOARD_POSITION.x - BLOCK_SIZE,
				BOARD_POSITION.y + y * BLOCK_SIZE
			}
		);

		target.draw(blockSprite);

		// Right wall
		blockSprite.setPosition(
			{
				BOARD_POSITION.x + Board::WIDTH * BLOCK_SIZE,
				BOARD_POSITION.y + y * BLOCK_SIZE
			}
		);

		target.draw(blockSprite);
	}

	// Bottom wall
	for (int x = -1; x <= Board::WIDTH; x++)
	{
		blockSprite.setPosition(
			{
				BOARD_POSITION.x + x * BLOCK_SIZE,
				BOARD_POSITION.y + Board::HEIGHT * BLOCK_SIZE
			}
		);

		target.draw(blockSprite);
	}

	// =====================================================
	// Render board
	// =====================================================

	const Board::Grid& grid = board.GetGrid();

	for (int y = 0; y < Board::HEIGHT; ++y)
	{
		for (int x = 0; x < Board::WIDTH; x++)
		{
			const Cell& cell = grid[y][x];

			if (!cell.occupied)
			{
				continue;
			}

			const int textureX = static_cast<int>(cell.tetrominoType) * SPRITE_SIZE;

			blockSprite.setTextureRect(
				{
					{ textureX, 0 },
					{ SPRITE_SIZE, SPRITE_SIZE }
				}
			);

			blockSprite.setPosition(
				{
					BOARD_POSITION.x + x * BLOCK_SIZE,
					BOARD_POSITION.y + y * BLOCK_SIZE
				}
			);

			target.draw(blockSprite);
		}
	}

	// =====================================================
	// Render clear row effects
	// =====================================================

	for (const ClearRowEffect& effect : clearRowEffects)
	{
		// =====================================================
		// Render flash effect
		// =====================================================

		const float t = effect.timer / CLEAR_ROW_EFFECT_DURATION;
		const int alpha = static_cast<int>((1.f - t) * 255.f);

		sf::RectangleShape flash;
		flash.setPosition(
			{
				BOARD_POSITION.x,
				BOARD_POSITION.y + effect.row * BLOCK_SIZE
			}
		);
		flash.setSize(
			{
				Board::WIDTH * BLOCK_SIZE,
				BLOCK_SIZE
			}
		);
		flash.setFillColor(sf::Color(120, 220, 255, alpha));

		target.draw(flash);

		// =====================================================
		// Render sweep effect
		// =====================================================

		const float sweepWidth = 120.f;
		const float sweepX = -sweepWidth + t * (Board::WIDTH * BLOCK_SIZE + sweepWidth * 2.f);

		sf::RectangleShape sweep;
		sweep.setPosition(
			{
				BOARD_POSITION.x + sweepX,
				BOARD_POSITION.y + effect.row * BLOCK_SIZE
			}
		);
		sweep.setSize({ sweepWidth,	BLOCK_SIZE });
		sweep.setFillColor(sf::Color(180, 255, 255, alpha));

		target.draw(sweep);
	}

	// =====================================================
	// Render ghost tetromino  (skipped while rows are clearing -- the piece it
	// mirrors is already locked into the board and would draw on top of it)
	// =====================================================

	if (phase == Phase::Falling)
	{
		const Tetromino ghostTetromino = GetGhostTetromino();
		const auto ghostBlockPositions = ghostTetromino.GetBlockPositions();
		const int ghostTextureX = static_cast<int>(ghostTetromino.GetType()) * SPRITE_SIZE;

		blockSprite.setTextureRect(
			{
				{ ghostTextureX, 0 },
				{ SPRITE_SIZE, SPRITE_SIZE }
			}
		);

		sf::Shader& ghostShader = context.shaders.Get(Assets::ShaderID::GhostTetromino);
		ghostShader.setUniform("time", context.totalTime);

		for (const sf::Vector2i& blockPosition : ghostBlockPositions)
		{
			blockSprite.setPosition(
				{
					BOARD_POSITION.x + blockPosition.x * BLOCK_SIZE,
					BOARD_POSITION.y + blockPosition.y * BLOCK_SIZE
				}
			);

			target.draw(blockSprite, &ghostShader);
		}
	}

	// Adding landing effect:

	if (landingEffectTimer > 0.f)
	{
		const float alpha = landingEffectTimer / LANDING_EFFECT_DURATION;

		blockSprite.setColor(sf::Color(255, 255, 255, static_cast<std::uint8_t>(alpha * 120.f)));

		for (const sf::Vector2i& blockPosition : landingEffectBlocks)
		{
			blockSprite.setPosition(
				{
					BOARD_POSITION.x + blockPosition.x * BLOCK_SIZE,
					BOARD_POSITION.y + blockPosition.y * BLOCK_SIZE
				}
			);

			target.draw(blockSprite);
		}

		blockSprite.setColor(sf::Color::White);
	}

	// =====================================================
	// Render current tetromino  (skipped while rows are clearing -- it is
	// already locked into the board)
	// =====================================================

	if (phase == Phase::Falling)
	{
		const auto blockPositions = currentTetromino.GetBlockPositions();
		const int textureX = static_cast<int>(currentTetromino.GetType()) * SPRITE_SIZE;

		blockSprite.setTextureRect(
			{
				{ textureX, 0 },
				{ SPRITE_SIZE, SPRITE_SIZE }
			}
		);

		// -------- Glow pass --------

		const float glowScale = 1.18f;

		blockSprite.setScale(
			{
				(BLOCK_SIZE / 16.f) * glowScale,
				(BLOCK_SIZE / 16.f) * glowScale
			}
		);

		blockSprite.setColor(sf::Color(255, 255, 255, 50));

		const float glowOffset = (BLOCK_SIZE * glowScale - BLOCK_SIZE) / 2.f;
		sf::Shader& glowShader = context.shaders.Get(Assets::ShaderID::Glow);

		sf::RenderStates glowStates;
		glowStates.blendMode = sf::BlendAdd;
		glowStates.shader = &glowShader;

		for (const sf::Vector2i& blockPosition : blockPositions)
		{
			blockSprite.setPosition(
				{
					BOARD_POSITION.x + blockPosition.x * BLOCK_SIZE - glowOffset,
					BOARD_POSITION.y + blockPosition.y * BLOCK_SIZE - glowOffset
				}
			);

			target.draw(blockSprite, glowStates);
		}

		// -------- Normal pass --------

		blockSprite.setScale(
			{
				BLOCK_SIZE / 16.f,
				BLOCK_SIZE / 16.f
			}
		);

		blockSprite.setColor(sf::Color::White);

		for (const sf::Vector2i& blockPosition : blockPositions)
		{
			blockSprite.setPosition(
				{
					BOARD_POSITION.x + blockPosition.x * BLOCK_SIZE,
					BOARD_POSITION.y + blockPosition.y * BLOCK_SIZE
				}
			);

			target.draw(blockSprite);
		}
	}

	// =====================================================
	// Render UI
	// =====================================================

	rightHudLayout->Render(target);
	controlsPanel->Render(target);

	// =====================================================
	// Render next tetromino preview
	// =====================================================

	const auto previewBlockPositions = nextTetromino.GetBlockPositions();
	const int previewTextureX = static_cast<int>(nextTetromino.GetType()) * SPRITE_SIZE;

	blockSprite.setTextureRect(
		{
			{ previewTextureX, 0 },
			{ SPRITE_SIZE, SPRITE_SIZE }
		}
	);

	constexpr float PREVIEW_BLOCK_SIZE = 36.f;

	blockSprite.setScale(
		{
			PREVIEW_BLOCK_SIZE / 16.f,
			PREVIEW_BLOCK_SIZE / 16.f
		}
	);

	// Different pieces occupy different cells of the 4x4 shape matrix, so centre
	// the piece's own bounding box on nextTetrominoPreviewPosition instead of
	// pinning its top-left corner there.
	int minBlockX = TetrominoShapes::MATRIX_SIZE;
	int maxBlockX = -1;
	int minBlockY = TetrominoShapes::MATRIX_SIZE;
	int maxBlockY = -1;

	for (const sf::Vector2i& blockPosition : previewBlockPositions)
	{
		minBlockX = std::min(minBlockX, blockPosition.x);
		maxBlockX = std::max(maxBlockX, blockPosition.x);
		minBlockY = std::min(minBlockY, blockPosition.y);
		maxBlockY = std::max(maxBlockY, blockPosition.y);
	}

	const sf::Vector2f previewOrigin =
	{
		nextTetrominoPreviewPosition.x - (minBlockX + maxBlockX + 1) * 0.5f * PREVIEW_BLOCK_SIZE,
		nextTetrominoPreviewPosition.y - (minBlockY + maxBlockY + 1) * 0.5f * PREVIEW_BLOCK_SIZE
	};

	for (const sf::Vector2i& blockPosition : previewBlockPositions)
	{
		blockSprite.setPosition(
			{
				previewOrigin.x + blockPosition.x * PREVIEW_BLOCK_SIZE,
				previewOrigin.y + blockPosition.y * PREVIEW_BLOCK_SIZE
			}
		);

		target.draw(blockSprite);
	}
}

void GameState::StartScreenShake(float duration, float intensity)
{
	shakeDuration = duration;
	shakeTimer = duration;
	shakeIntensity = intensity;
}

bool GameState::SpawnTetromino()
{
	currentTetromino = { nextTetromino.GetType(), { Board::WIDTH / 2 - 2, 0 } };
	nextTetromino = { tetrominoBag.Next(), { 0, 0 } };

	return board.CanPlace(currentTetromino);
}

void GameState::TryMoveTetromino(int offsetX, int offsetY)
{
	Tetromino movedTetromino = currentTetromino;

	movedTetromino.Move(offsetX, offsetY);

	if (!board.CanPlace(movedTetromino))
	{
		context.audioPlayer.Play(Assets::SoundID::PieceHitWall);
		return;
	}

	currentTetromino = movedTetromino;

	context.audioPlayer.Play(Assets::SoundID::MovePiece);
}

void GameState::TryRotateTetromino()
{
	Tetromino rotatedTetromino = currentTetromino;

	rotatedTetromino.RotateClockwise();

	if (!board.CanPlace(rotatedTetromino))
	{
		context.audioPlayer.Play(Assets::SoundID::PieceHitWall);
		return;
	}

	currentTetromino = rotatedTetromino;

	context.audioPlayer.Play(Assets::SoundID::RotatePiece);
}

void GameState::TryDropTetromino()
{
	while (true)
	{
		Tetromino movedTetromino = currentTetromino;

		movedTetromino.Move(0, 1);

		if (!board.CanPlace(movedTetromino))
		{
			break;
		}

		currentTetromino = movedTetromino;
	}

	context.audioPlayer.Play(Assets::SoundID::DropPiece);
	StartScreenShake(0.12f, 12.f);

	// HandleTetrominoLanding() locks the piece into the board itself; locking
	// here as well double-wrote the same cells.
	HandleTetrominoLanding();

	fallTimer = 0.f;
}

void GameState::HandleTetrominoLanding()
{
	landingEffectBlocks = currentTetromino.GetBlockPositions();
	landingEffectTimer = LANDING_EFFECT_DURATION;

	board.LockTetromino(currentTetromino);

	const std::vector<int> fullRows = board.FindFullRows();

	// =====================================================
	// Clear row effect
	// =====================================================

	if (!fullRows.empty())
	{
		context.audioPlayer.Play(Assets::SoundID::RowCleared);

		for (int row : fullRows)
		{
			clearRowEffects.push_back({ .row = row, .timer = 0.f });
		}

		phase = Phase::ClearingRows;
		return;
	}

	// =====================================================
	// No cleared rows
	// =====================================================

	if (!SpawnTetromino())
	{
		RequestChange(std::make_unique<GameOverState>(context, score));
	}
}

Tetromino GameState::GetGhostTetromino() const
{
	Tetromino ghostTetromino = currentTetromino;

	while (true)
	{
		Tetromino movedTetromino = ghostTetromino;

		movedTetromino.Move(0, 1);

		if (!board.CanPlace(movedTetromino))
		{
			break;
		}

		ghostTetromino = movedTetromino;
	}

	return ghostTetromino;
}

Assets::TextureID GameState::GetBlockTextureID() const
{
	switch (context.settings.GetSettings().blockRenderStyle)
	{
	case BlockRenderStyle::WithOutline:
		return Assets::TextureID::BlockSpritesheetWithOutline;

	case BlockRenderStyle::WithoutOutline:
		return Assets::TextureID::BlockSpritesheetWithoutOutline;
	}

	std::unreachable();
}