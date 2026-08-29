#pragma once

#include <memory>

#include <SFML/Graphics/Sprite.hpp>

#include "../core/State.h"
#include "../core/Context.h"
#include "../gameplay/GameplaySession.h"
#include "../input/ActionMap.h"
#include "../input/DirectionalRepeater.h"
#include "../input/InputHandler.h"
#include "../rendering/BoardRenderer.h"
#include "../rendering/EffectsController.h"
#include "../rendering/NeonGlow.h"
#include "../ui/Label.h"
#include "../ui/Layout.h"
#include "../ui/Panel.h"

// The gameplay screen: owns the rules (GameplaySession), the input layer that
// feeds it, the HUD, and the two renderers. It translates the session's
// per-frame events into sound, HUD text and screen effects.
class GameplayState : public State
{
private:
	enum class GameplayAction
	{
		MoveLeft,
		MoveRight,
		SoftDrop,
		HardDrop,
		RotateClockwise,
		RotateCounterClockwise,
		Pause
	};

	Context& context;

	GameplaySession session;
	BoardRenderer boardRenderer;
	NeonGlow neonGlow;
	EffectsController effects;

	ActionMap<GameplayAction> gameplayActions;
	InputHandler<GameplayAction> gameplayInput;
	DirectionalRepeater horizontalRepeater;

	int heldHorizontal = 0;
	int previousHeldHorizontal = 0;
	bool horizontalWasBlocked = false;
	bool softDropHeld = false;
	float softDropTimer = 0.f;

	sf::Sprite backgroundSprite;

	std::unique_ptr<UI::Layout> rightHudLayout;
	std::unique_ptr<UI::Panel> controlsPanel;

	UI::Label* scoreLabel = nullptr;
	UI::Label* levelLabel = nullptr;

	sf::Vector2f nextTetrominoPreviewPosition;

	void SetUpInputBindings();
	void BuildHud();

	void PollHeldInput();
	void ApplyGamepadActions();
	void ApplyHorizontalRepeat(float deltaTime);
	void ApplySoftDrop(float deltaTime);

	void TryRotate(bool clockwise);
	void PerformHardDrop();
	void ReactToEvents(const GameplaySession::Events& events);

public:
	explicit GameplayState(Context& context);

	[[nodiscard]] StateId GetId() const override { return StateId::Game; }

	void HandleEvent(const sf::Event& event) override;
	void Update(float deltaTime) override;
	void Render(sf::RenderTarget& target) override;
};
