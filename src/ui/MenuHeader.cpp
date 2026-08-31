#include "MenuHeader.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <utility>
#include <vector>

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Glyph.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Transform.hpp>
#include <SFML/Graphics/VertexArray.hpp>

#include "ColourUtils.h"
#include "GlyphQuad.h"

namespace
{
	constexpr unsigned int HeaderTextSize = 110;
	constexpr sf::Vector2f HeaderCentre{ 960.f, 120.f };

	constexpr float RiseDuration = 0.28f;
	constexpr float SinkDuration = 0.24f;

	// Same styling the carousel entries use, so the entry looks unchanged as it
	// becomes the header (mirrors CarouselMenu -- a shared "menu text" helper is
	// a fair future cleanup).
	constexpr float LetterSpacing = 0.09f;        // tracking, fraction of the char size
	constexpr float OutlineThickness = 4.f;
	constexpr sf::Vector2f ShadowOffset{ 5.f, 7.f };
	constexpr float ShadowAlpha = 0.5f;
	constexpr float GradientTopMix = 0.42f;
	constexpr float GradientBottom = 0.5f;
	constexpr float OutlineDarken = 0.18f;

	// Neon bloom + idle wave.
	constexpr float GlowIntensity = 0.55f;
	constexpr float GlowBreathSpeed = 2.0f;
	constexpr float WaveAmplitude = 4.f;
	constexpr float WaveSpeed = 2.0f;
	constexpr float WavePhaseStep = 0.6f;

	using UI::Darken;
	using UI::MixToWhite;
	using UI::ScaleRgb;
	using UI::ToByte;

	[[nodiscard]] float EaseOutCubic(float t) noexcept
	{
		const float inv = 1.f - std::clamp(t, 0.f, 1.f);
		return 1.f - inv * inv * inv;
	}

	[[nodiscard]] float Lerp(float a, float b, float t) noexcept
	{
		return a + (b - a) * t;
	}

	[[nodiscard]] sf::Vector2f Lerp(sf::Vector2f a, sf::Vector2f b, float t) noexcept
	{
		return { Lerp(a.x, b.x, t), Lerp(a.y, b.y, t) };
	}
}

namespace UI
{
	MenuHeader::MenuHeader(const sf::Font& fontRef, sf::Shader& dilateShader, sf::Shader& blurShader)
		: font(fontRef)
		, characterSize(HeaderTextSize)
		, glow(dilateShader, blurShader)
	{
	}

	void MenuHeader::SetLabel(const sf::String& label)
	{
		glyphs.clear();

		const float tracking = LetterSpacing * static_cast<float>(characterSize);

		std::vector<std::pair<char32_t, float>> raw;
		float penX = 0.f;
		char32_t previous = 0;
		float inkTop = 0.f;
		float inkBottom = 0.f;

		for (std::size_t i = 0; i < label.getSize(); ++i)
		{
			const char32_t codepoint = label[i];
			if (previous != 0)
			{
				penX += font.getKerning(previous, codepoint, characterSize) + tracking;
			}

			if (codepoint != U' ')
			{
				raw.push_back({ codepoint, penX });
				const sf::FloatRect gb = font.getGlyph(codepoint, characterSize, false).bounds;
				inkTop = std::min(inkTop, gb.position.y);
				inkBottom = std::max(inkBottom, gb.position.y + gb.size.y);
			}

			penX += font.getGlyph(codepoint, characterSize, false).advance;
			previous = codepoint;
		}

		const float halfWidth = penX * 0.5f;
		for (const auto& [codepoint, x] : raw)
		{
			glyphs.push_back({ codepoint, x - halfWidth });
		}

		inkCentreY = (inkTop + inkBottom) * 0.5f;
		inkSize = { penX, std::max(inkBottom - inkTop, 1.f) };

		// One fixed glow box for this label, sized for the settled header, so
		// NeonGlow allocates its buffers once and not every animation frame.
		glowBoxSize = { inkSize.x * 1.5f + 120.f, inkSize.y * 2.4f + 120.f };
	}

	void MenuHeader::RiseFrom(sf::Vector2f fromCentre, float fromHeight, const sf::String& label, sf::Color newColour)
	{
		colour = newColour;
		SetLabel(label);

		fromPosition = fromCentre;
		toPosition = HeaderCentre;
		fromScale = std::max(fromHeight, 1.f) / inkSize.y;
		toScale = 1.f;

		mode = Mode::Rising;
		timer = 0.f;
	}

	void MenuHeader::SinkTo(sf::Vector2f toCentre, float toHeight)
	{
		fromPosition = HeaderCentre;
		toPosition = toCentre;
		fromScale = 1.f;
		toScale = std::max(toHeight, 1.f) / inkSize.y;

		mode = Mode::Sinking;
		timer = 0.f;
	}

	void MenuHeader::Update(float deltaTime)
	{
		waveTime += deltaTime;
		glow.Update(deltaTime);

		if (mode == Mode::Rising)
		{
			timer = std::min(1.f, timer + deltaTime / RiseDuration);
			if (timer >= 1.f)
			{
				mode = Mode::Shown;
			}
		}
		else if (mode == Mode::Sinking)
		{
			timer = std::min(1.f, timer + deltaTime / SinkDuration);
			if (timer >= 1.f)
			{
				mode = Mode::Hidden;
			}
		}
	}

	MenuHeader::Pose MenuHeader::CurrentPose() const
	{
		if (mode == Mode::Shown)
		{
			return { toPosition, toScale, 1.f };
		}

		const float e = EaseOutCubic(timer);
		Pose pose;
		pose.position = Lerp(fromPosition, toPosition, e);
		pose.scale = Lerp(fromScale, toScale, e);
		pose.alpha = mode == Mode::Rising ? std::min(1.f, timer * 1.6f) : 1.f - timer;
		return pose;
	}

	void MenuHeader::Render(sf::RenderTarget& target) const
	{
		if (mode == Mode::Hidden || glyphs.empty())
		{
			return;
		}

		const Pose pose = CurrentPose();
		const float alphaFraction = std::clamp(pose.alpha, 0.f, 1.f);
		if (alphaFraction <= 0.f)
		{
			return;
		}

		const auto alpha = static_cast<std::uint8_t>(alphaFraction * 255.f);

		sf::Transform transform;
		transform.translate(pose.position);
		transform.scale({ pose.scale, pose.scale });
		transform.translate({ 0.f, -inkCentreY });

		// --- Neon bloom, in a fixed-size box so NeonGlow never re-sizes ---
		const sf::FloatRect glowArea{
			{ pose.position.x - glowBoxSize.x * 0.5f, pose.position.y - glowBoxSize.y * 0.5f },
			glowBoxSize };
		const float breath = 0.85f + 0.15f * std::sin(waveTime * GlowBreathSpeed);
		const sf::Color glowTint = ScaleRgb(colour, GlowIntensity * breath * alphaFraction);

		glow.Draw(target, glowArea,
			[this, &transform](sf::RenderTarget& buffer, const sf::RenderStates& states)
			{
				sf::VertexArray white(sf::PrimitiveType::Triangles);
				for (std::size_t i = 0; i < glyphs.size(); ++i)
				{
					const float waveY = WaveAmplitude * std::sin(waveTime * WaveSpeed + static_cast<float>(i) * WavePhaseStep);
					AppendGlyphQuad(white, glyphs[i].penX,
						font.getGlyph(glyphs[i].codepoint, characterSize, false),
						sf::Color::White, sf::Color::White, { 0.f, waveY });
				}

				sf::RenderStates s = states;
				s.transform *= transform;
				s.texture = &font.getTexture(characterSize);
				buffer.draw(white, s);
			},
			glowTint, false);

		// --- Shadow, dark outline, vertical-gradient fill ---
		const sf::Color shadowColour(0, 0, 0, static_cast<std::uint8_t>(ShadowAlpha * alphaFraction * 255.f));
		sf::Color outlineColour = Darken(colour, OutlineDarken);   outlineColour.a = alpha;
		sf::Color fillTop = MixToWhite(colour, GradientTopMix);     fillTop.a = alpha;
		sf::Color fillBottom = Darken(colour, GradientBottom);      fillBottom.a = alpha;

		sf::VertexArray shadow(sf::PrimitiveType::Triangles);
		sf::VertexArray outline(sf::PrimitiveType::Triangles);
		sf::VertexArray fill(sf::PrimitiveType::Triangles);

		for (std::size_t i = 0; i < glyphs.size(); ++i)
		{
			const float waveY = WaveAmplitude * std::sin(waveTime * WaveSpeed + static_cast<float>(i) * WavePhaseStep);
			const sf::Glyph& body = font.getGlyph(glyphs[i].codepoint, characterSize, false);
			const sf::Glyph& rim = font.getGlyph(glyphs[i].codepoint, characterSize, false, OutlineThickness);

			AppendGlyphQuad(shadow, glyphs[i].penX, body, shadowColour, shadowColour,
				{ ShadowOffset.x, ShadowOffset.y + waveY });
			AppendGlyphQuad(outline, glyphs[i].penX, rim, outlineColour, outlineColour, { 0.f, waveY });
			AppendGlyphQuad(fill, glyphs[i].penX, body, fillTop, fillBottom, { 0.f, waveY });
		}

		sf::RenderStates states;
		states.transform = transform;
		states.texture = &font.getTexture(characterSize);

		target.draw(shadow, states);
		target.draw(outline, states);
		target.draw(fill, states);
	}

	bool MenuHeader::IsIdle() const
	{
		return mode == Mode::Hidden;
	}

	bool MenuHeader::IsSettled() const
	{
		return mode == Mode::Shown;
	}
}
