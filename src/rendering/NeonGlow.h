#pragma once

#include <functional>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/System/Vector2.hpp>

namespace sf
{
	class RenderTarget;
	class Shader;
	class Texture;
}

// An even neon edge-glow for a small piece of content — the active tetromino,
// or the selected menu button. The caller draws the source (in target
// coordinates) through a callback; NeonGlow renders it to an off-screen buffer,
// grows the silhouette by a fixed margin on every side and corner (a square
// dilation), softens it with a tight inner blur and a wide outer blur, and
// composites both back additively, tinted and gently pulsing.
//
// The crisp content is expected to be drawn on top afterwards, leaving an even
// glowing band around its whole outline.
class NeonGlow
{
public:
	// Draws the bright source into `buffer` with the given render states, which
	// carry the offset that maps target space into the buffer.
	using DrawSource = std::function<void(sf::RenderTarget& buffer, const sf::RenderStates& states)>;

	NeonGlow(sf::Shader& dilateShader, sf::Shader& blurShader);

	void Update(float deltaTime);

	// `area` is the source's bounding box in `target` coordinates. It should
	// already include a few pixels of slack for the glow band.
	void Draw(sf::RenderTarget& target, sf::FloatRect area, const DrawSource& drawSource,
		sf::Color tint, bool pulsing = true);

private:
	static constexpr float Padding = 32.f;
	static constexpr float BloomScale = 0.5f;
	static constexpr float OutlineRadius = 8.f;   // full-resolution pixels the silhouette grows by
	static constexpr float InnerBlurRadius = 1.0f;
	static constexpr float OuterBlurRadius = 1.7f;
	static constexpr unsigned int InnerIterations = 2u;
	static constexpr unsigned int OuterIterations = 3u;
	static constexpr float PulseSpeed = 4.0f;

	[[nodiscard]] bool Resize(sf::Vector2f contentSize);
	void Dilate(sf::RenderTexture& buffer, float radiusPixels);
	void Blur(const sf::Texture& input, sf::RenderTexture& output, float radius, unsigned int iterations);
	[[nodiscard]] float Pulse() const;

	sf::Shader& dilateShader;
	sf::Shader& blurShader;

	sf::RenderTexture source;      // full resolution, the raw content on transparent
	sf::RenderTexture seed;        // half resolution, downscaled then dilated silhouette
	sf::RenderTexture scratch;     // half resolution, single-axis-pass intermediate
	sf::RenderTexture innerBlur;   // half resolution, tight blur
	sf::RenderTexture outerBlur;   // half resolution, wide blur

	sf::Vector2f cachedContentSize{ 0.f, 0.f };
	float elapsedTime = 0.f;
};
