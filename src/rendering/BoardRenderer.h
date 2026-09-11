#pragma once

#include <cstdint>
#include <optional>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Vector2.hpp>

#include "../resources/Assets.h"

namespace sf
{
	class RenderTarget;
}

struct Context;
class GameplaySession;
class EffectsController;
class NeonGlow;
class Tetromino;

// Draws the play area for GameplayState: board gradient, walls, locked cells,
// the ghost, the active piece (glow + normal passes), the next-piece preview,
// and the visual half of the gameplay effects. It reads the session and the
// effects controller and never changes them; the only state it keeps for
// itself is the next-queue slide animation, advanced by Update().
class BoardRenderer
{
public:
	static constexpr float BlockSize = 42.f;
	static constexpr sf::Vector2f BoardPosition{ 720.f, 84.f };

	explicit BoardRenderer(Context& context);

	// Advances the next-queue slide: call once per frame before Render().
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
	// nothing while no piece has been held yet.
	void RenderHoldPreview(sf::RenderTarget& target, const GameplaySession& session, sf::FloatRect area) const;

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

	void DrawPiecePreview(sf::RenderTarget& target, const Tetromino& piece, float blockSize,
		sf::Vector2f centre, sf::Color tint = sf::Color::White) const;

	[[nodiscard]] static float NextSlotCentreY(sf::FloatRect area, int slot);
	[[nodiscard]] static float NextSlotBlockSize(int slot);
	[[nodiscard]] static sf::Color NextSlotTint(int slot, int count);

	[[nodiscard]] Assets::TextureID ResolveBlockTexture() const;

	Context& context;

	// Next-queue slide animation: 1 = settled, 0 = just advanced (every piece
	// still drawn one slot behind where it's headed).
	std::optional<int> previousSpawnCount;
	float nextSlideProgress = 1.f;
};
