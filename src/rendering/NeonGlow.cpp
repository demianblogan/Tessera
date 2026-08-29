#include "NeonGlow.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Shader.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>

namespace
{
	sf::Color Modulate(sf::Color color, float intensity)
	{
		const auto channel = [intensity](std::uint8_t value)
		{
			return static_cast<std::uint8_t>(std::clamp(static_cast<float>(value) * intensity, 0.f, 255.f));
		};

		return { channel(color.r), channel(color.g), channel(color.b), 255 };
	}

	sf::Vector2u CeilToTexture(float x, float y)
	{
		return
		{
			std::max(1u, static_cast<unsigned int>(std::ceil(x))),
			std::max(1u, static_cast<unsigned int>(std::ceil(y)))
		};
	}
}

NeonGlow::NeonGlow(sf::Shader& dilateShader, sf::Shader& blurShader)
	: dilateShader(dilateShader)
	, blurShader(blurShader)
{
	// No code
}

void NeonGlow::Update(float deltaTime)
{
	elapsedTime = std::fmod(elapsedTime + deltaTime, 1000.f);
}

float NeonGlow::Pulse() const
{
	return 0.78f + 0.22f * (std::sin(elapsedTime * PulseSpeed) * 0.5f + 0.5f);
}

bool NeonGlow::Resize(sf::Vector2f contentSize)
{
	const sf::Vector2u full = CeilToTexture(contentSize.x + Padding * 2.f, contentSize.y + Padding * 2.f);
	const sf::Vector2u half = CeilToTexture(full.x * BloomScale, full.y * BloomScale);

	if (!source.resize(full) ||
		!seed.resize(half) ||
		!scratch.resize(half) ||
		!innerBlur.resize(half) ||
		!outerBlur.resize(half))
	{
		return false;
	}

	source.setSmooth(true);
	seed.setSmooth(true);
	scratch.setSmooth(true);
	innerBlur.setSmooth(true);
	outerBlur.setSmooth(true);

	cachedContentSize = contentSize;
	return true;
}

void NeonGlow::Dilate(sf::RenderTexture& buffer, float radiusPixels)
{
	sf::RenderStates dilateStates;
	dilateStates.blendMode = sf::BlendNone;
	dilateStates.shader = &dilateShader;

	// Four samples per side, so the step is a quarter of the target radius.
	const float stepX = radiusPixels / 4.f / static_cast<float>(buffer.getSize().x);
	dilateShader.setUniform("offset", sf::Glsl::Vec2(stepX, 0.f));
	scratch.clear(sf::Color::Transparent);
	scratch.draw(sf::Sprite(buffer.getTexture()), dilateStates);
	scratch.display();

	const float stepY = radiusPixels / 4.f / static_cast<float>(buffer.getSize().y);
	dilateShader.setUniform("offset", sf::Glsl::Vec2(0.f, stepY));
	buffer.clear(sf::Color::Transparent);
	buffer.draw(sf::Sprite(scratch.getTexture()), dilateStates);
	buffer.display();
}

void NeonGlow::Blur(const sf::Texture& input, sf::RenderTexture& output, float radius, unsigned int iterations)
{
	const sf::Vector2f texelStep{ radius / static_cast<float>(input.getSize().x), radius / static_cast<float>(input.getSize().y) };

	sf::RenderStates blurStates;
	blurStates.blendMode = sf::BlendNone;
	blurStates.shader = &blurShader;

	const sf::Texture* current = &input;

	for (unsigned int iteration = 0; iteration < iterations; iteration++)
	{
		blurShader.setUniform("direction", sf::Glsl::Vec2(texelStep.x, 0.f));
		scratch.clear(sf::Color::Transparent);
		scratch.draw(sf::Sprite(*current), blurStates);
		scratch.display();

		blurShader.setUniform("direction", sf::Glsl::Vec2(0.f, texelStep.y));
		output.clear(sf::Color::Transparent);
		output.draw(sf::Sprite(scratch.getTexture()), blurStates);
		output.display();

		current = &output.getTexture();
	}
}

void NeonGlow::Draw(sf::RenderTarget& target, sf::FloatRect area, const DrawSource& drawSource,
	sf::Color tint, bool pulsing)
{
	if (cachedContentSize != area.size && !Resize(area.size))
	{
		return;
	}

	if (outerBlur.getSize().x == 0u || outerBlur.getSize().y == 0u)
	{
		return;
	}

	// Render the source with the top-left of `area` mapped to (Padding, Padding).
	source.clear(sf::Color::Transparent);

	sf::RenderStates sourceStates;
	sourceStates.transform.translate(sf::Vector2f{ Padding, Padding } - area.position);
	drawSource(source, sourceStates);
	source.display();

	// Downscale into the working buffer.
	seed.clear(sf::Color::Transparent);
	sf::Sprite downscaled(source.getTexture());
	downscaled.setScale({ BloomScale, BloomScale });
	seed.draw(downscaled);
	seed.display();

	// Grow the silhouette evenly on every side and corner, then soften it.
	Dilate(seed, OutlineRadius * BloomScale);

	Blur(seed.getTexture(), innerBlur, InnerBlurRadius, InnerIterations);
	Blur(seed.getTexture(), outerBlur, OuterBlurRadius, OuterIterations);

	const float pulse = pulsing ? Pulse() : 1.f;
	const sf::Vector2f position = area.position - sf::Vector2f{ Padding, Padding };

	sf::RenderStates additive;
	additive.blendMode = sf::BlendAdd;

	sf::Sprite outer(outerBlur.getTexture());
	outer.setPosition(position);
	outer.setScale({ 1.f / BloomScale, 1.f / BloomScale });
	outer.setColor(Modulate(tint, 0.85f * pulse));
	target.draw(outer, additive);
	target.draw(outer, additive);

	sf::Sprite inner(innerBlur.getTexture());
	inner.setPosition(position);
	inner.setScale({ 1.f / BloomScale, 1.f / BloomScale });
	inner.setColor(Modulate(tint, 0.95f * (0.72f + pulse * 0.28f)));
	target.draw(inner, additive);
}
