#pragma once

#include <memory>

#include <SFML/Graphics/Sprite.hpp>

#include "../core/State.h"
#include "../core/Context.h"
#include "../gameplay/GameplaySession.h"
#include "../input/ActionMap.h"
#include "../input/DirectionalRepeater.h"
#include "../input/InputHandler.h"
#include "../rendering/BoardCallouts.h"
#include "../rendering/BoardRenderer.h"
#include "../rendering/EffectsController.h"
#include "../rendering/GameplayHud.h"
#include "../rendering/NeonGlow.h"
#include "../rendering/SceneMotion.h"

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
		Hold,
		Pause
	};

	Context& context;

	GameplaySession session;
	BoardRenderer boardRenderer;
	NeonGlow neonGlow;
	EffectsController effects;
	GameplayHud hud;
	SceneMotion sceneMotion;
	BoardCallouts boardCallouts;

	ActionMap<GameplayAction> gameplayActions;
	InputHandler<GameplayAction> gameplayInput;
	DirectionalRepeater horizontalRepeater;

	int heldHorizontal = 0;
	int previousHeldHorizontal = 0;
	bool horizontalWasBlocked = false;
	bool softDropHeld = false;
	float softDropTimer = 0.f;

	// A short hold at the start: the scene is up but the session and input are
	// frozen, so the menu -> gameplay transition can settle before the first
	// piece begins to fall.
	static constexpr float IntroDuration = 0.50f;
	bool introActive = true;
	float introTimer = 0.f;

	// The death beat between top-out and the game-over screen: the stack
	// crumbles, a red flash and shake fire, and the frame darkens.
	static constexpr float DeathDuration = 0.80f;
	bool dying = false;
	float deathTimer = 0.f;

	sf::Sprite backgroundSprite;

	void SetUpInputBindings();

	// Push the current HUD / effects settings into the live HUD and effects
	// objects. Run at construction and every Update() after, so a change made
	// from the pause screen's Options takes effect the instant play resumes,
	// with no dependence on the state stack's resume timing.
	void ApplyGameplaySettings();

	void PollHeldInput();
	void ApplyGamepadActions();
	void ApplyHorizontalRepeat(float deltaTime);
	void ApplySoftDrop(float deltaTime);

	void TryRotate(bool clockwise);
	void PerformHardDrop();
	void TryHold();
	void ReactToEvents(const GameplaySession::Events& events);
	void ShowClearCallout(const GameplaySession::Events& events);
	void FireClearHaptics(const GameplaySession::Events& events);

	// Snapshot the current frame and hand it to a new PauseState, so the pause
	// screen can "solidify" the frozen picture behind its menu.
	void OpenPause();

public:
	// `playIntro` false starts the session immediately (no frozen hold).
	explicit GameplayState(Context& context, bool playIntro = true);

	void HandleEvent(const sf::Event& event) override;
	void Update(float deltaTime) override;
	void Render(sf::RenderTarget& target) override;

	// The mouse plays no part in gameplay -- pausing pushes PauseState on top,
	// which shows its own cursor via ScreenHost/MenuScreen.
	[[nodiscard]] bool ShowsCursor() const override { return false; }
};
