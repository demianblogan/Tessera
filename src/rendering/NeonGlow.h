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

// Additive neon bloom for a small piece of content — currently the active
// tetromino. The caller draws the source (in target coordinates) through a
// callback; NeonGlow renders it to an off-screen buffer, blurs it at half
// resolution in a tight inner pass and a wide outer pass, and composites both
// back over the target with additive blending, tinted and gently pulsing.
//
// Reusable: it holds only render buffers, a blur shader reference and a clock.
class NeonGlow
{
public:
	// Draws the bright source into `buffer` using the given render states
	// (which carry the offset that maps target space into the buffer).
	using DrawSource = std::function<void(sf::RenderTarget& buffer, const sf::RenderStates& states)>;

	explicit NeonGlow(sf::Shader& blurShader);

	void Update(float deltaTime);

	// `area` is the source's bounding box in `target` coordinates.
	void Draw(sf::RenderTarget& target, sf::FloatRect area, const DrawSource& drawSource,
		sf::Color tint, bool pulsing = true);

private:
	static constexpr float Padding = 64.f;
	static constexpr float BloomScale = 0.5f;
	static constexpr float InnerBlurRadius = 1.0f;
	static constexpr float OuterBlurRadius = 2.4f;
	static constexpr unsigned int InnerIterations = 2u;
	static constexpr unsigned int OuterIterations = 5u;
	static constexpr float PulseSpeed = 4.0f;

	[[nodiscard]] bool Resize(sf::Vector2f contentSize);
	void Blur(const sf::Texture& input, sf::RenderTexture& output, float radius, unsigned int iterations);
	[[nodiscard]] float Pulse() const;

	sf::Shader& blurShader;

	sf::RenderTexture source;      // full resolution, the raw piece on transparent
	sf::RenderTexture seed;        // half resolution, downscaled source
	sf::RenderTexture scratch;     // half resolution, horizontal-pass intermediate
	sf::RenderTexture innerBlur;   // half resolution, tight blur
	sf::RenderTexture outerBlur;   // half resolution, wide blur

	sf::Vector2f cachedContentSize{ 0.f, 0.f };
	float elapsedTime = 0.f;
};
