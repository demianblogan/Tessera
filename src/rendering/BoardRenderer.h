#pragma once

#include <cstdint>
#include <optional>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Vector2.hpp>

#include "../gameplay/Tetromino.h"
#include "../resources/Assets.h"
#include "NeonGlow.h"

namespace sf
{
	class RenderTarget;
}

struct Context;
class GameplaySession;
class EffectsController;

// Draws the play area for GameplayState: board gradient, walls, locked cells,
// the ghost, the active piece (glow + normal passes), the next-piece preview,
// and the visual half of the gameplay effects. It reads the session and the
// effects controller and never changes them; the only state it keeps for
// itself is presentation animation (the next-queue slide, the hold-swap
// flight), advanced by Update().
class BoardRenderer
{
public:
	static constexpr float BlockSize = 42.f;
	static constexpr sf::Vector2f BoardPosition{ 720.f, 84.f };

	explicit BoardRenderer(Context& context);

	// Advances the next-queue slide and any hold-swap flight: call once per
	// frame before Render().
	void Update(float deltaTime, const GameplaySession& session);

	// `deathProgress` (0..1) crumbles the locked cells downward and greys them
	// out for the game-over sequence.
	void Render(sf::RenderTarget& target, const GameplaySession& session, const EffectsController& effects,
		NeonGlow& glow, float deathProgress = 0.f) const;
	// Draws the upcoming pieces stacked inside `area` (the NEXT HUD cell). The
	// piece that spawns next is drawn larger and brighter than the rest; when
	// the queue advances, everything slides smoothly into its new slot.
	void RenderNextPreview(sf::RenderTarget& target, const GameplaySession& session, sf::FloatRect area) const;
	// Draws the held piece centred inside `area` (the HOLD HUD cell), dimmed
	// once hold has already been used on the piece currently in play. Draws
	// nothing while no piece has been held yet, or while a hold-swap flight
	// (see TriggerHoldSwap) is covering the same ground.
	void RenderHoldPreview(sf::RenderTarget& target, const GameplaySession& session, sf::FloatRect area) const;
	// Draws whatever hold-swap flight is in progress: the piece just sent to
	// hold, and (if one was already held) the piece coming back out.
	void RenderHoldFlight(sf::RenderTarget& target) const;

	// Kicks off the hold-swap flight: `outgoingPiece` (the piece that was just
	// sent to hold, at its board position right before the swap) flies to the
	// HOLD box; `incomingPiece`, if set, is the piece that was already held --
	// it flies from the HOLD box out to its own (already-assigned) board
	// position. Purely cosmetic: GameplaySession has already applied the swap.
	void TriggerHoldSwap(const Tetromino& outgoingPiece, std::optional<Tetromino> incomingPiece,
		sf::FloatRect holdBoxArea);

	[[nodiscard]] bool IsHoldFlightActive() const { return outgoingFlight || incomingFlight; }

	// Whether the landing-preview ghost piece is drawn. A player preference,
	// read from settings.
	void SetGhostEnabled(bool enabled) { ghostEnabled = enabled; }

private:
	static constexpr int SpriteSize = 16;
	static constexpr int WallTextureIndex = 10;

	// Next-queue preview sizing: the piece that spawns next is the "hero" slot,
	// the rest are smaller and progressively darkened.
	static constexpr float NextHeroBlockSize = 30.f;
	static constexpr float NextRestBlockSize = 20.f;
	static constexpr float NextHeroSlotHeight = 78.f;
	static constexpr float NextRestSlotHeight = 52.f;
	static constexpr std::uint8_t NextMinBrightness = 110;

	// How long a slide from one slot to the next takes once the queue advances.
	static constexpr float NextSlideDuration = 0.16f;

	// How long a hold-swap flight takes to cross from board to HOLD box (or back).
	static constexpr float HoldFlightDuration = 0.22f;

	// One piece animating between two points/sizes -- either board <-> HOLD box.
	struct PieceFlight
	{
		Tetromino::Type type;
		sf::Vector2f fromCentre;
		float fromBlockSize;
		sf::Vector2f toCentre;
		float toBlockSize;
		float timer = 0.f;
	};

	void DrawPiecePreview(sf::RenderTarget& target, const Tetromino& piece, float blockSize,
		sf::Vector2f centre, sf::Color tint = sf::Color::White) const;

	[[nodiscard]] static float NextSlotCentreY(sf::FloatRect area, int slot);
	[[nodiscard]] static float NextSlotBlockSize(int slot);
	[[nodiscard]] static sf::Color NextSlotTint(int slot, int count);

	// Centre of a piece's bounding box, in board screen space -- the same point
	// its normal on-board rendering is centred on.
	[[nodiscard]] static sf::Vector2f BoardSpaceCentre(const Tetromino& piece);

	[[nodiscard]] Assets::TextureID ResolveBlockTexture() const;

	Context& context;

	// Next-queue slide animation: 1 = settled, 0 = just advanced (every piece
	// still drawn one slot behind where it's headed).
	std::optional<int> previousSpawnCount;
	float nextSlideProgress = 1.f;

	std::optional<PieceFlight> outgoingFlight;   // board -> HOLD box
	std::optional<PieceFlight> incomingFlight;   // HOLD box -> board

	bool ghostEnabled = true;

	// A standing pulsing halo around every golden lock (see EscalationDirector),
	// so a bonus piece stays visible after it lands, not just while falling.
	// Mutable: Render() is const (it only reads game state), but NeonGlow keeps
	// GPU-side scratch buffers it has to mutate to draw.
	mutable NeonGlow goldenGlow;
};
