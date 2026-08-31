#include "MenuLabel.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <utility>

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Glyph.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Transform.hpp>
#include <SFML/Graphics/VertexArray.hpp>

#include "ColourUtils.h"
#include "GlyphQuad.h"
#include "../rendering/NeonGlow.h"

namespace
{
	// Same styling numbers the carousel entries use.
	constexpr float LetterSpacing = 0.09f;        // tracking, fraction of the char size
	constexpr float OutlineThickness = 4.f;
	constexpr sf::Vector2f ShadowOffset{ 5.f, 7.f };
	constexpr float ShadowAlpha = 0.5f;
	constexpr float GradientTopMix = 0.42f;
	constexpr float GradientBottom = 0.5f;
	constexpr float OutlineDarken = 0.18f;

	constexpr float WaveAmplitude = 4.f;
	constexpr float WaveSpeed = 2.0f;
	constexpr float WavePhaseStep = 0.6f;

	using UI::Darken;
	using UI::MixToWhite;
}

namespace UI
{
	MenuLabel::MenuLabel(const sf::Font& fontRef, unsigned int size)
		: font(&fontRef)
		, characterSize(size)
	{
	}

	void MenuLabel::SetText(const sf::String& text)
	{
		glyphs.clear();

		const float tracking = LetterSpacing * static_cast<float>(characterSize);

		std::vector<std::pair<char32_t, float>> raw;
		float penX = 0.f;
		char32_t previous = 0;
		float inkTop = 0.f;
		float inkBottom = 0.f;

		for (std::size_t i = 0; i < text.getSize(); ++i)
		{
			const char32_t codepoint = text[i];
			if (previous != 0)
			{
				penX += font->getKerning(previous, codepoint, characterSize) + tracking;
			}

			if (codepoint != U' ')
			{
				raw.push_back({ codepoint, penX });
				const sf::FloatRect gb = font->getGlyph(codepoint, characterSize, false).bounds;
				inkTop = std::min(inkTop, gb.position.y);
				inkBottom = std::max(inkBottom, gb.position.y + gb.size.y);
			}

			penX += font->getGlyph(codepoint, characterSize, false).advance;
			previous = codepoint;
		}

		const float halfWidth = penX * 0.5f;
		for (const auto& [codepoint, x] : raw)
		{
			glyphs.push_back({ codepoint, x - halfWidth });
		}

		inkCentreY = (inkTop + inkBottom) * 0.5f;
		inkSize = { penX, std::max(inkBottom - inkTop, 1.f) };
		autoGlowBoxSize = { inkSize.x * 1.5f + 120.f, inkSize.y * 2.4f + 120.f };
	}

	void MenuLabel::SetGlowBoxSize(sf::Vector2f size)
	{
		fixedGlowBoxSize = size;
	}

	sf::Vector2f MenuLabel::GlowBox() const
	{
		return (fixedGlowBoxSize.x > 0.f && fixedGlowBoxSize.y > 0.f) ? fixedGlowBoxSize : autoGlowBoxSize;
	}

	void MenuLabel::Update(float deltaTime)
	{
		waveTime += deltaTime;
	}

	float MenuLabel::WaveOffset(std::size_t index) const
	{
		return WaveAmplitude * std::sin(waveTime * WaveSpeed + static_cast<float>(index) * WavePhaseStep);
	}

	sf::FloatRect MenuLabel::Bounds(sf::Vector2f centre, float scale) const
	{
		return {
			{ centre.x - inkSize.x * 0.5f * scale, centre.y - inkSize.y * 0.5f * scale },
			{ inkSize.x * scale, inkSize.y * scale } };
	}

	void MenuLabel::Draw(sf::RenderTarget& target, sf::Vector2f centre, float scale, sf::Color colour,
		float alphaFraction, float whiten) const
	{
		if (glyphs.empty())
		{
			return;
		}

		alphaFraction = std::clamp(alphaFraction, 0.f, 1.f);
		if (alphaFraction <= 0.f)
		{
			return;
		}

		const sf::Color base = whiten > 0.f ? MixToWhite(colour, whiten) : colour;
		const auto alpha = static_cast<std::uint8_t>(alphaFraction * 255.f);

		sf::Transform transform;
		transform.translate(centre);
		transform.scale({ scale, scale });
		transform.translate({ 0.f, -inkCentreY });

		const sf::Color shadowColour(0, 0, 0, static_cast<std::uint8_t>(ShadowAlpha * alphaFraction * 255.f));
		sf::Color outlineColour = Darken(base, OutlineDarken);   outlineColour.a = alpha;
		sf::Color fillTop = MixToWhite(base, GradientTopMix);     fillTop.a = alpha;
		sf::Color fillBottom = Darken(base, GradientBottom);      fillBottom.a = alpha;

		sf::VertexArray shadow(sf::PrimitiveType::Triangles);
		sf::VertexArray outline(sf::PrimitiveType::Triangles);
		sf::VertexArray fill(sf::PrimitiveType::Triangles);

		for (std::size_t i = 0; i < glyphs.size(); ++i)
		{
			const float waveY = WaveOffset(i);
			const sf::Glyph& body = font->getGlyph(glyphs[i].codepoint, characterSize, false);
			const sf::Glyph& rim = font->getGlyph(glyphs[i].codepoint, characterSize, false, OutlineThickness);

			AppendGlyphQuad(shadow, glyphs[i].penX, body, shadowColour, shadowColour,
				{ ShadowOffset.x, ShadowOffset.y + waveY });
			AppendGlyphQuad(outline, glyphs[i].penX, rim, outlineColour, outlineColour, { 0.f, waveY });
			AppendGlyphQuad(fill, glyphs[i].penX, body, fillTop, fillBottom, { 0.f, waveY });
		}

		sf::RenderStates states;
		states.transform = transform;
		states.texture = &font->getTexture(characterSize);

		target.draw(shadow, states);
		target.draw(outline, states);
		target.draw(fill, states);
	}

	void MenuLabel::DrawGlow(sf::RenderTarget& target, NeonGlow& glow, sf::Vector2f centre, float scale,
		sf::Color tint) const
	{
		if (glyphs.empty())
		{
			return;
		}

		sf::Transform transform;
		transform.translate(centre);
		transform.scale({ scale, scale });
		transform.translate({ 0.f, -inkCentreY });

		const sf::Vector2f box = GlowBox();
		const sf::FloatRect area{ { centre.x - box.x * 0.5f, centre.y - box.y * 0.5f }, box };

		glow.Draw(target, area,
			[this, &transform](sf::RenderTarget& buffer, const sf::RenderStates& states)
			{
				sf::VertexArray white(sf::PrimitiveType::Triangles);
				for (std::size_t i = 0; i < glyphs.size(); ++i)
				{
					AppendGlyphQuad(white, glyphs[i].penX,
						font->getGlyph(glyphs[i].codepoint, characterSize, false),
						sf::Color::White, sf::Color::White, { 0.f, WaveOffset(i) });
				}

				sf::RenderStates s = states;
				s.transform *= transform;
				s.texture = &font->getTexture(characterSize);
				buffer.draw(white, s);
			},
			tint, false);
	}
}
