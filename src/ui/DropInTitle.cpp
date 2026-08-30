#include "DropInTitle.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

#include <SFML/Graphics/BlendMode.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Glyph.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Transform.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/System/Angle.hpp>

#include "../rendering/NeonGlow.h"

namespace
{
	constexpr float StaggerDelay = 0.09f;    // gap between successive letters starting
	constexpr float FallDuration = 0.32f;
	constexpr float SettleDuration = 0.55f;
	constexpr float DropDistance = 750.f;    // how far above the resting spot a letter starts
	constexpr float SquashY = 0.60f;         // vertical scale at the moment of impact
	constexpr float StretchX = 1.35f;        // horizontal scale at the moment of impact

	constexpr float FlashDuration = 0.16f;   // white-hot flash decaying back to colour
	constexpr float OutlineThickness = 4.f;

	// Gentle idle motion once a letter has settled: a travelling vertical bob,
	// a touch of sway, and a matching pulse in the glow -- all eased in.
	constexpr float WaveAmplitude = 7.f;
	constexpr float WaveSpeed = 2.3f;
	constexpr float WavePhaseStep = 0.7f;    // radians of offset between neighbours
	constexpr float WaveSwayDegrees = 1.6f;
	constexpr float WaveRampDuration = 0.7f;
	constexpr float GlowMin = 0.35f;         // dimmest point of the idle glow breath
	constexpr float GlowMax = 0.85f;         // brightest point of the idle glow breath
	constexpr float GlowFloor = 0.40f;       // never dimmer than this (raised by the impact flash)
	constexpr float GlowIntensity = 0.55f;   // overall multiplier applied to every letter's bloom

	constexpr float AberrationDuration = 0.10f;
	constexpr float AberrationOffset = 9.f;

	constexpr float ShockDuration = 0.30f;
	constexpr float ShockRadiusStart = 10.f;
	constexpr float ShockRadiusEnd = 95.f;

	constexpr int GhostCount = 3;
	constexpr float GhostStep = 0.028f;      // seconds between motion-blur ghosts
	constexpr float GhostAlpha = 70.f;

	constexpr float KickPerLanding = 210.f;  // downward velocity added when a letter lands
	constexpr float KickStiffness = 150.f;
	constexpr float KickDamping = 13.f;

	constexpr float GradientTopMix = 0.35f;      // fill: how far the top edge is pushed to white
	constexpr float GradientBottomFactor = 0.55f; // fill: how far the bottom edge is darkened

	constexpr std::uint8_t ReflectionAlpha = 46;
	constexpr float MirrorDistance = 120.f;      // gap below the word the reflection mirrors about
	constexpr float ReflectionGlowScale = 0.4f;  // how much of the real letter's bloom the reflection gets

	// One per letter, cycled: the seven classic tetromino colours, matching
	// the loading-bar blocks.
	constexpr std::array<sf::Color, 7> LetterPalette{
		sf::Color{ 0, 240, 240 },    // I - cyan
		sf::Color{ 245, 220, 40 },   // O - yellow
		sf::Color{ 180, 60, 240 },   // T - purple
		sf::Color{ 60, 230, 90 },    // S - green
		sf::Color{ 240, 60, 70 },    // Z - red
		sf::Color{ 70, 110, 240 },   // J - blue
		sf::Color{ 245, 160, 40 },   // L - orange
	};

	[[nodiscard]] float Lerp(float a, float b, float t) noexcept
	{
		return a + (b - a) * t;
	}

	[[nodiscard]] std::uint8_t ToByte(float value) noexcept
	{
		return static_cast<std::uint8_t>(std::clamp(value, 0.f, 255.f));
	}

	[[nodiscard]] sf::Color Darken(sf::Color colour, float factor) noexcept
	{
		return { ToByte(colour.r * factor), ToByte(colour.g * factor), ToByte(colour.b * factor) };
	}

	[[nodiscard]] sf::Color MixToWhite(sf::Color colour, float t) noexcept
	{
		return {
			ToByte(colour.r + (255.f - colour.r) * t),
			ToByte(colour.g + (255.f - colour.g) * t),
			ToByte(colour.b + (255.f - colour.b) * t) };
	}

	[[nodiscard]] sf::Color Scale(sf::Color colour, float factor) noexcept
	{
		return { ToByte(colour.r * factor), ToByte(colour.g * factor), ToByte(colour.b * factor), colour.a };
	}

	// Overshooting spring: 0 at t=0, 1 at t=1, wobbling past 1 on the way.
	[[nodiscard]] float EaseOutElastic(float t) noexcept
	{
		if (t <= 0.f) return 0.f;
		if (t >= 1.f) return 1.f;

		constexpr float period = 2.f * 3.14159265f / 3.f;
		return std::pow(2.f, -10.f * t) * std::sin((t * 10.f - 0.75f) * period) + 1.f;
	}

	[[nodiscard]] float EaseOutQuad(float t) noexcept
	{
		const float inv = 1.f - std::clamp(t, 0.f, 1.f);
		return 1.f - inv * inv;
	}

	void AppendQuad(sf::VertexArray& array, const sf::FloatRect& bounds, const sf::FloatRect& texture,
		sf::Color topColour, sf::Color bottomColour)
	{
		const sf::Vector2f tl{ bounds.position.x, bounds.position.y };
		const sf::Vector2f tr{ bounds.position.x + bounds.size.x, bounds.position.y };
		const sf::Vector2f br{ bounds.position.x + bounds.size.x, bounds.position.y + bounds.size.y };
		const sf::Vector2f bl{ bounds.position.x, bounds.position.y + bounds.size.y };

		const sf::Vector2f ttl{ texture.position.x, texture.position.y };
		const sf::Vector2f ttr{ texture.position.x + texture.size.x, texture.position.y };
		const sf::Vector2f tbr{ texture.position.x + texture.size.x, texture.position.y + texture.size.y };
		const sf::Vector2f tbl{ texture.position.x, texture.position.y + texture.size.y };

		array.append({ tl, topColour, ttl });
		array.append({ tr, topColour, ttr });
		array.append({ br, bottomColour, tbr });
		array.append({ tl, topColour, ttl });
		array.append({ br, bottomColour, tbr });
		array.append({ bl, bottomColour, tbl });
	}
}

namespace UI
{
	DropInTitle::DropInTitle(const sf::Font& fontRef, const sf::String& text, unsigned int size)
		: font(fontRef)
		, characterSize(size)
	{
		// Vertical extent of the whole word, for keeping the letters on one line.
		const sf::FloatRect wordBounds = sf::Text(font, text, characterSize).getLocalBounds();
		const float wordCentreY = wordBounds.position.y + wordBounds.size.y * 0.5f;

		// Walk the pen across the string using the font's own advances / kerning
		// (sf::Text::findCharacterPos is deprecated in this SFML build).
		struct Placed { char32_t codepoint; float centreX; };
		std::vector<Placed> placed;
		float penX = 0.f;
		char32_t previous = 0;

		for (std::size_t i = 0; i < text.getSize(); ++i)
		{
			const char32_t codepoint = text[i];
			if (previous != 0)
			{
				penX += font.getKerning(previous, codepoint, characterSize);
			}

			const float advance = font.getGlyph(codepoint, characterSize, false).advance;
			if (codepoint != U' ')
			{
				placed.push_back({ codepoint, penX + advance * 0.5f });
			}

			penX += advance;
			previous = codepoint;
		}

		const float wordCentreX = penX * 0.5f;

		for (std::size_t i = 0; i < placed.size(); ++i)
		{
			const sf::Color colour = LetterPalette[i % LetterPalette.size()];

			Glyph glyph{ sf::Text(font, sf::String(placed[i].codepoint), characterSize), placed[i].codepoint,
				colour, 0.f, 0.f, 0.f, 0.f };

			const sf::FloatRect gb = glyph.text.getLocalBounds();
			glyph.text.setOrigin({ gb.position.x + gb.size.x * 0.5f, gb.position.y + gb.size.y * 0.5f });
			glyph.text.setOutlineThickness(OutlineThickness);

			glyph.offsetX = placed[i].centreX - wordCentreX;
			// The glyph's own ink centre, against the whole word's, keeps the
			// letters sitting on a common line once they land.
			glyph.offsetY = (gb.position.y + gb.size.y * 0.5f) - wordCentreY;
			glyph.startDelay = static_cast<float>(i) * StaggerDelay;

			glowBoxSize.x = std::max(glowBoxSize.x, gb.size.x);
			glowBoxSize.y = std::max(glowBoxSize.y, gb.size.y);

			glyphs.push_back(std::move(glyph));
		}

		// One box size for every letter: the widest/tallest glyph plus slack
		// for the impact stretch. NeonGlow adds its own bloom padding on top.
		glowBoxSize.x *= StretchX + 0.2f;
		glowBoxSize.y *= 1.25f;
	}

	void DropInTitle::SetCenter(sf::Vector2f newCenter)
	{
		center = newCenter;
	}

	void DropInTitle::SetReflectionEnabled(bool enabled)
	{
		reflectionEnabled = enabled;
	}

	void DropInTitle::Skip()
	{
		// Land every letter exactly at rest -- past the settle, before the wave
		// ramps in, so nothing jumps.
		for (Glyph& glyph : glyphs)
		{
			glyph.elapsed = glyph.startDelay + FallDuration + SettleDuration;
		}
		kickOffset = 0.f;
		kickVelocity = 0.f;
	}

	void DropInTitle::Update(float deltaTime)
	{
		for (Glyph& glyph : glyphs)
		{
			const float before = glyph.elapsed - glyph.startDelay;
			glyph.elapsed += deltaTime;
			const float after = glyph.elapsed - glyph.startDelay;

			if (before < FallDuration && after >= FallDuration)
			{
				kickVelocity += KickPerLanding;
			}
		}

		// Damped spring back to rest.
		kickVelocity += (-KickStiffness * kickOffset - KickDamping * kickVelocity) * deltaTime;
		kickOffset += kickVelocity * deltaTime;
	}

	DropInTitle::Pose DropInTitle::EvaluateGlyph(std::size_t index) const
	{
		const float local = glyphs[index].elapsed - glyphs[index].startDelay;

		Pose pose;
		pose.local = local;

		if (local <= 0.f)
		{
			pose.y = -DropDistance;
			return pose;
		}

		pose.visible = true;

		if (local < FallDuration)
		{
			const float p = local / FallDuration;
			pose.y = -DropDistance * (1.f - p * p);
			pose.falling = true;
			return pose;
		}

		const float impact = local - FallDuration;
		const float e = EaseOutElastic(std::clamp(impact / SettleDuration, 0.f, 1.f));
		pose.scaleX = Lerp(StretchX, 1.f, e);
		pose.scaleY = Lerp(SquashY, 1.f, e);

		if (impact < FlashDuration)
		{
			const float f = 1.f - impact / FlashDuration;
			pose.flash = f * f;
		}
		if (impact < AberrationDuration)
		{
			pose.aberration = 1.f - impact / AberrationDuration;
		}
		if (impact < ShockDuration)
		{
			pose.shock = impact / ShockDuration;
		}

		const float sinceSettled = impact - SettleDuration;
		if (sinceSettled > 0.f)
		{
			const float ramp = std::clamp(sinceSettled / WaveRampDuration, 0.f, 1.f);
			const float phase = local * WaveSpeed + static_cast<float>(index) * WavePhaseStep;

			pose.y = ramp * WaveAmplitude * std::sin(phase);
			pose.rotationDegrees = ramp * WaveSwayDegrees * std::sin(phase * 0.5f);

			// Brightest as the letter rides up (sin negative => higher on screen).
			const float glowWave = 0.5f - 0.5f * std::sin(phase);
			pose.glowStrength = Lerp(1.f, Lerp(GlowMin, GlowMax, glowWave), ramp);
		}

		return pose;
	}

	sf::Vector2f DropInTitle::RestingPosition(std::size_t index, const Pose& pose) const
	{
		return { center.x + glyphs[index].offsetX,
				 center.y + kickOffset + glyphs[index].offsetY + pose.y };
	}

	void DropInTitle::DrawGradientLetter(sf::RenderTarget& target, std::size_t index, const Pose& pose,
		sf::Vector2f drawPosition, float scaleSignY, std::uint8_t alpha) const
	{
		const Glyph& glyph = glyphs[index];

		sf::Color base = glyph.colour;
		if (pose.flash > 0.f)
		{
			base = MixToWhite(base, pose.flash);
		}

		sf::Color top = MixToWhite(base, GradientTopMix);
		sf::Color bottom = Darken(base, GradientBottomFactor);
		top.a = alpha;
		bottom.a = alpha;

		const sf::Glyph& fontGlyph = font.getGlyph(glyph.codepoint, characterSize, false);
		const sf::FloatRect texRect(fontGlyph.textureRect);
		const sf::Vector2f inkCentre{
			fontGlyph.bounds.position.x + fontGlyph.bounds.size.x * 0.5f,
			fontGlyph.bounds.position.y + fontGlyph.bounds.size.y * 0.5f };

		sf::Transform transform;
		transform.translate(drawPosition);
		transform.rotate(sf::degrees(pose.rotationDegrees));
		transform.scale({ pose.scaleX, pose.scaleY * scaleSignY });
		transform.translate(-inkCentre);

		// Outline: the same glyph, grown, in a dark shade of its own hue.
		const sf::Color outlineColour = Darken(base, 0.28f);

		sf::Text outline = glyph.text;
		outline.setPosition(drawPosition);
		outline.setRotation(sf::degrees(pose.rotationDegrees));
		outline.setScale({ pose.scaleX, pose.scaleY * scaleSignY });
		outline.setFillColor(sf::Color::Transparent);
		outline.setOutlineColor(sf::Color(outlineColour.r, outlineColour.g, outlineColour.b, alpha));
		target.draw(outline);

		sf::VertexArray fill(sf::PrimitiveType::Triangles);
		AppendQuad(fill, fontGlyph.bounds, texRect, top, bottom);

		sf::RenderStates states;
		states.transform = transform;
		states.texture = &font.getTexture(characterSize);
		target.draw(fill, states);

		// Chromatic split: additive red / blue copies pulled apart briefly.
		if (pose.aberration > 0.f && scaleSignY > 0.f)
		{
			const float shift = pose.aberration * AberrationOffset;
			const std::uint8_t aberAlpha = ToByte(pose.aberration * 150.f);

			for (int side = 0; side < 2; ++side)
			{
				const sf::Color tint = side == 0
					? sf::Color(255, 40, 40, aberAlpha)
					: sf::Color(40, 60, 255, aberAlpha);
				const float dx = side == 0 ? -shift : shift;

				sf::Transform ghost;
				ghost.translate({ drawPosition.x + dx, drawPosition.y });
				ghost.rotate(sf::degrees(pose.rotationDegrees));
				ghost.scale({ pose.scaleX, pose.scaleY * scaleSignY });
				ghost.translate(-inkCentre);

				sf::VertexArray channel(sf::PrimitiveType::Triangles);
				AppendQuad(channel, fontGlyph.bounds, texRect, tint, tint);

				sf::RenderStates ghostStates;
				ghostStates.transform = ghost;
				ghostStates.texture = &font.getTexture(characterSize);
				ghostStates.blendMode = sf::BlendAdd;
				target.draw(channel, ghostStates);
			}
		}
	}

	void DropInTitle::DrawLetterGlow(sf::RenderTarget& target, NeonGlow& glow, std::size_t index, const Pose& pose,
		sf::Vector2f position, float scaleSignY, float intensityScale) const
	{
		// Fixed-size box centred on the letter -- same size for every letter and
		// every frame, so NeonGlow's buffers never resize after the first call.
		const sf::FloatRect area{
			{ position.x - glowBoxSize.x * 0.5f, position.y - glowBoxSize.y * 0.5f },
			glowBoxSize };

		const float strength = std::max(pose.glowStrength, GlowFloor + 0.4f * pose.flash);
		const sf::Color tint = Scale(glyphs[index].colour, strength * GlowIntensity * intensityScale);

		sf::Text silhouette = glyphs[index].text;
		silhouette.setPosition(position);
		silhouette.setRotation(sf::degrees(pose.rotationDegrees));
		silhouette.setScale({ pose.scaleX, pose.scaleY * scaleSignY });

		glow.Draw(target, area,
			[&silhouette](sf::RenderTarget& buffer, const sf::RenderStates& states)
			{
				sf::Text white = silhouette;
				white.setFillColor(sf::Color::White);
				white.setOutlineColor(sf::Color::White);
				buffer.draw(white, states);
			},
			tint, false);
	}

	void DropInTitle::Render(sf::RenderTarget& target, NeonGlow* glow) const
	{
		std::vector<Pose> poses(glyphs.size());
		for (std::size_t i = 0; i < glyphs.size(); ++i)
		{
			poses[i] = EvaluateGlyph(i);
		}

		// -- Motion-blur ghosts, for letters still in the air --------------
		for (std::size_t i = 0; i < glyphs.size(); ++i)
		{
			if (!poses[i].falling)
			{
				continue;
			}

			for (int g = 1; g <= GhostCount; ++g)
			{
				const float ghostLocal = poses[i].local - static_cast<float>(g) * GhostStep;
				if (ghostLocal <= 0.f)
				{
					break;
				}

				const float p = ghostLocal / FallDuration;
				const float ghostY = -DropDistance * (1.f - p * p);

				sf::Text ghost = glyphs[i].text;
				ghost.setOutlineThickness(0.f);
				ghost.setPosition({ center.x + glyphs[i].offsetX,
					center.y + kickOffset + glyphs[i].offsetY + ghostY });
				ghost.setFillColor(sf::Color(glyphs[i].colour.r, glyphs[i].colour.g, glyphs[i].colour.b,
					ToByte(GhostAlpha * (1.f - static_cast<float>(g) / (GhostCount + 1)))));
				target.draw(ghost);
			}
		}

		// -- Per-letter neon bloom (word, then its reflection) -------------
		if (glow != nullptr)
		{
			for (std::size_t i = 0; i < glyphs.size(); ++i)
			{
				if (poses[i].visible)
				{
					DrawLetterGlow(target, *glow, i, poses[i], RestingPosition(i, poses[i]), 1.f, 1.f);
				}
			}

			if (reflectionEnabled)
			{
				const float mirrorY = center.y + kickOffset + MirrorDistance;
				for (std::size_t i = 0; i < glyphs.size(); ++i)
				{
					if (!poses[i].visible)
					{
						continue;
					}

					const sf::Vector2f upright = RestingPosition(i, poses[i]);
					const sf::Vector2f mirrored{ upright.x, 2.f * mirrorY - upright.y };
					DrawLetterGlow(target, *glow, i, poses[i], mirrored, -1.f, ReflectionGlowScale);
				}
			}
		}

		// -- Shockwave rings --------------------------------------------------
		for (std::size_t i = 0; i < glyphs.size(); ++i)
		{
			if (poses[i].shock < 0.f)
			{
				continue;
			}

			const float t = EaseOutQuad(poses[i].shock);
			const float radius = Lerp(ShockRadiusStart, ShockRadiusEnd, t);

			sf::CircleShape ring(radius);
			ring.setOrigin({ radius, radius });
			ring.setPosition(RestingPosition(i, poses[i]));
			ring.setFillColor(sf::Color::Transparent);
			ring.setOutlineThickness(Lerp(4.f, 1.f, t));
			ring.setOutlineColor(sf::Color(glyphs[i].colour.r, glyphs[i].colour.g, glyphs[i].colour.b,
				ToByte((1.f - poses[i].shock) * 190.f)));
			target.draw(ring);
		}

		// -- Reflection (dim, mirrored, drawn under the word) ----------------
		if (reflectionEnabled)
		{
			const float mirrorY = center.y + kickOffset + MirrorDistance;
			for (std::size_t i = 0; i < glyphs.size(); ++i)
			{
				if (!poses[i].visible)
				{
					continue;
				}

				const sf::Vector2f upright = RestingPosition(i, poses[i]);
				const sf::Vector2f mirrored{ upright.x, 2.f * mirrorY - upright.y };
				DrawGradientLetter(target, i, poses[i], mirrored, -1.f, ReflectionAlpha);
			}
		}

		// -- The letters themselves ----------------------------------------
		for (std::size_t i = 0; i < glyphs.size(); ++i)
		{
			if (!poses[i].visible)
			{
				continue;
			}

			DrawGradientLetter(target, i, poses[i], RestingPosition(i, poses[i]), 1.f, 255);
		}
	}

	bool DropInTitle::IsFinished() const
	{
		for (const Glyph& glyph : glyphs)
		{
			if (glyph.elapsed - glyph.startDelay < FallDuration + SettleDuration)
			{
				return false;
			}
		}

		return true;
	}
}
