#pragma once

#include <memory>

#include <SFML/Graphics/Sprite.hpp>

#include "../core/State.h"
#include "../core/Context.h"
#include "../gameplay/GameplaySession.h"
#include "GameplayInputController.h"
#include "../rendering/BoardCallouts.h"
#include "../rendering/BoardRenderer.h"
#include "../rendering/EffectsController.h"
#include "../rendering/GameplayHUD.h"
#include "../primitives/NeonGlow.h"
#include "../rendering/SceneMotion.h"

// The gameplay screen: owns the rules (GameplaySession), the HUD, and the two
// renderers. GameplayInputController feeds player input to the session; this
// class reacts to what the session reports happened, turning it into sound,
// HUD text and screen effects.
class GameplayState : public State
{
public:
	// `isIntroPlayed` false starts the session immediately (no frozen hold).
	explicit GameplayState(Context& context, bool isIntroPlayed = true);

	void HandleEvent(const sf::Event& event) override;
	void Update(float deltaTime) override;
	void Render(sf::RenderTarget& target) override;

	// The mouse plays no part in gameplay -- pausing pushes PauseState on top,
	// which shows its own cursor via ScreenHost/MenuScreen.
	[[nodiscard]] bool IsCursorVisible() const override;

private:
	// Push the current HUD / effects settings into the live HUD and effects
	// objects. Run at construction and every Update() after, so a change made
	// from the pause screen's Options takes effect the instant play resumes,
	// with no dependence on the state stack's resume timing.
	void ApplyGameplaySettings();

	void ReactToEvents(const GameplaySession::Events& events);
	void ShowClearCallout(const GameplaySession::Events& events);
	void FireClearHaptics(const GameplaySession::Events& events);

	// Snapshot the current frame and hand it to a new PauseState, so the pause
	// screen can "solidify" the frozen picture behind its menu.
	void OpenPause();

	Context& context;

	GameplaySession session;
	BoardRenderer boardRenderer;
	NeonGlow neonGlow;
	EffectsController effects;
	GameplayHUD HUD;
	SceneMotion sceneMotion;
	BoardCallouts boardCallouts;

	// Translates keyboard/gamepad input into session actions; see its header
	// for why that's a separate collaborator from this class's own event
	// reactions below.
	GameplayInputController inputController;

	// A short hold at the start: the scene is up but the session and input are
	// frozen, so the menu -> gameplay transition can settle before the first
	// piece begins to fall.
	static constexpr float IntroDuration = 0.50f;
	bool isIntroActive = true;
	float introTimer = 0.f;

	// The death beat between top-out and the game-over screen: the stack
	// crumbles, a red flash and shake fire, and the frame darkens.
	static constexpr float DeathDuration = 0.80f;
	bool isDying = false;
	float deathTimer = 0.f;

	sf::Sprite backgroundSprite;

	// The language ApplyGameplaySettings() last refreshed the HUD's cached
	// captions for.
	unsigned int seenLocalizationRevision;
};
