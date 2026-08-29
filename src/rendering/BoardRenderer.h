#pragma once

#include <SFML/System/Vector2.hpp>

#include "../resources/Assets.h"

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
// effects controller; it never changes them.
class BoardRenderer
{
public:
	static constexpr float BlockSize = 48.f;
	static constexpr sf::Vector2f BoardPosition{ 720.f, 60.f };

	explicit BoardRenderer(Context& context);

	void Render(sf::RenderTarget& target, const GameplaySession& session, const EffectsController& effects) const;
	void RenderNextPreview(sf::RenderTarget& target, const GameplaySession& session, sf::Vector2f centre) const;

private:
	static constexpr int SpriteSize = 16;
	static constexpr int WallTextureIndex = 10;
	static constexpr float GlowScale = 1.18f;
	static constexpr float PreviewBlockSize = 36.f;

	[[nodiscard]] Assets::TextureID ResolveBlockTexture() const;

	Context& context;
};
