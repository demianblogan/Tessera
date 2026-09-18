#include "DropInTitle.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <utility>

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

#include "ColorUtils.h"
#include "../utils/Easing.h"
#include "TetrominoPalette.h"
#include "../primitives/NeonGlow.h"

namespace
{
	constexpr float StaggerDelay = 0.14f;    // gap between successive letters starting
	constexpr float FallDuration = 0.40f;
	constexpr float SettleDuration = 0.55f;
	constexpr float DropDistance = 750.f;    // how far above the resting spot a letter starts
	constexpr float SquashY = 0.60f;         // vertical scale at the moment of impact
	constexpr float StretchX = 1.35f;        // horizontal scale at the moment of impact

	constexpr float FlashDuration = 0.16f;   // white-hot flash decaying back to color
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
	constexpr float ShockOutlineThicknessStart = 4.f;
	constexpr float ShockOutlineThicknessEnd = 1.f;
	constexpr float ShockAlphaScale = 190.f;

	constexpr int GhostCount = 3;
	constexpr float GhostStep = 0.028f;      // seconds between motion-blur ghosts
	constexpr float GhostAlpha = 70.f;

	constexpr float KickPerLanding = 210.f;  // downward velocity added when a letter lands
	constexpr float KickStiffness = 150.f;
	constexpr float KickDamping = 13.f;

	constexpr float GradientTopMix = 0.35f;      // fill: how far the top edge is pushed to white
	constexpr float GradientBottomFactor = 0.55f; // fill: how far the bottom edge is darkened
	constexpr float OutlineDarkenFactor = 0.28f;  // outline: how far the base color is darkened

	// One fixed glow box for every letter: the widest/tallest glyph plus this
	// much slack for the impact stretch.
	constexpr float GlowBoxWidthSlack = 0.2f;
	constexpr float GlowBoxHeightScale = 1.25f;
	constexpr float GlowFlashWeight = 0.4f;   // how much the impact flash raises the glow floor

	// Chromatic split on impact: additive red / blue copies pulled apart.
	constexpr sf::Color AberrationRedTint{ 255, 40, 40 };
	constexpr sf::Color AberrationBlueTint{ 40, 60, 255 };
	constexpr float AberrationAlphaScale = 150.f;

	// Exit: the word accelerates straight up off the screen and fades.
	constexpr float ExitDuration = 0.34f;
	constexpr float ExitLiftDistance = 950.f;

	using UI::Darken;
	using UI::MixToWhite;
	using UI::ScaleRgb;
	using UI::ToByte;

	using Easing::EaseOutElastic;   // overshooting spring
	using Easing::EaseOutQuad;
	using Easing::Lerp;

	void AppendQuad(sf::VertexArray& array, const sf::FloatRect& bounds, const sf::FloatRect& texture,
		sf::Color topColor, sf::Color bottomColor)
	{
		const sf::Vector2f tl{ bounds.position.x, bounds.position.y };
		const sf::Vector2f tr{ bounds.position.x + bounds.size.x, bounds.position.y };
		const sf::Vector2f br{ bounds.position.x + bounds.size.x, bounds.position.y + bounds.size.y };
		const sf::Vector2f bl{ bounds.position.x, bounds.position.y + bounds.size.y };

		const sf::Vector2f ttl{ texture.position.x, texture.position.y };
		const sf::Vector2f ttr{ texture.position.x + texture.size.x, texture.position.y };
		const sf::Vector2f tbr{ texture.position.x + texture.size.x, texture.position.y + texture.size.y };
		const sf::Vector2f tbl{ texture.position.x, texture.position.y + texture.size.y };

		array.append({ tl, topColor, ttl });
		array.append({ tr, topColor, ttr });
		array.append({ br, bottomColor, tbr });
		array.append({ tl, topColor, ttl });
		array.append({ br, bottomColor, tbr });
		array.append({ bl, bottomColor, tbl });
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
		const float wordCenterY = wordBounds.position.y + wordBounds.size.y * 0.5f;

		// Walk the pen across the string using the font's own advances / kerning
		// (sf::Text::findCharacterPos is deprecated in this SFML build).
		struct Placed { char32_t codepoint; float centerX; };
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

		const float wordCenterX = penX * 0.5f;

		for (std::size_t i = 0; i < placed.size(); ++i)
		{
			const sf::Color color = UI::TetrominoColors[i % UI::TetrominoColors.size()];

			Glyph glyph{ sf::Text(font, sf::String(placed[i].codepoint), characterSize), placed[i].codepoint,
				color, 0.f, 0.f, 0.f, 0.f };

			const sf::FloatRect gb = glyph.text.getLocalBounds();
			glyph.text.setOrigin({ gb.position.x + gb.size.x * 0.5f, gb.position.y + gb.size.y * 0.5f });
			glyph.text.setOutlineThickness(OutlineThickness);

			glyph.offsetX = placed[i].centerX - wordCenterX;
			// The glyph's own ink center, against the whole word's, keeps the
			// letters sitting on a common line once they land.
			glyph.offsetY = (gb.position.y + gb.size.y * 0.5f) - wordCenterY;
			glyph.startDelay = static_cast<float>(i) * StaggerDelay;

			glowBoxSize.x = std::max(glowBoxSize.x, gb.size.x);
			glowBoxSize.y = std::max(glowBoxSize.y, gb.size.y);

			glyphs.push_back(std::move(glyph));
		}

		// One box size for every letter: the widest/tallest glyph plus slack
		// for the impact stretch. NeonGlow adds its own bloom padding on top.
		glowBoxSize.x *= StretchX + GlowBoxWidthSlack;
		glowBoxSize.y *= GlowBoxHeightScale;
	}

	void DropInTitle::SetCenter(sf::Vector2f newCenter)
	{
		center = newCenter;
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

	void DropInTitle::SetLandCallback(std::function<void(std::size_t)> callback)
	{
		onLetterLand = std::move(callback);
	}

	void DropInTitle::PlayExit()
	{
		if (exitTime < 0.f)
		{
			exitTime = 0.f;
		}
	}

	float DropInTitle::GetExitLift() const
	{
		if (exitTime < 0.f)
		{
			return 0.f;
		}

		const float p = std::clamp(exitTime / ExitDuration, 0.f, 1.f);
		return -p * p * ExitLiftDistance;   // ease-in: slow start, whipping up
	}

	float DropInTitle::GetExitAlpha() const
	{
		if (exitTime < 0.f)
		{
			return 1.f;
		}

		return std::clamp(1.f - exitTime / ExitDuration, 0.f, 1.f);
	}

	bool DropInTitle::IsExitComplete() const
	{
		return exitTime >= ExitDuration;
	}

	void DropInTitle::Update(float deltaTime)
	{
		if (exitTime >= 0.f)
		{
			exitTime += deltaTime;
		}

		for (std::size_t i = 0; i < glyphs.size(); ++i)
		{
			Glyph& glyph = glyphs[i];
			const float before = glyph.elapsed - glyph.startDelay;
			glyph.elapsed += deltaTime;
			const float after = glyph.elapsed - glyph.startDelay;

			if (before < FallDuration && after >= FallDuration)
			{
				kickVelocity += KickPerLanding;
				if (onLetterLand)
				{
					onLetterLand(i);
				}
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
		pose.localTime = local;

		if (local <= 0.f)
		{
			pose.offsetY = -DropDistance;
			return pose;
		}

		pose.isVisible = true;

		if (local < FallDuration)
		{
			const float fallFraction = local / FallDuration;
			pose.offsetY = -DropDistance * (1.f - fallFraction * fallFraction);
			pose.isFalling = true;
			return pose;
		}

		const float impact = local - FallDuration;
		const float easedProgress = EaseOutElastic(std::clamp(impact / SettleDuration, 0.f, 1.f));
		pose.scaleX = Lerp(StretchX, 1.f, easedProgress);
		pose.scaleY = Lerp(SquashY, 1.f, easedProgress);

		if (impact < FlashDuration)
		{
			const float flashFraction = 1.f - impact / FlashDuration;
			pose.flash = flashFraction * flashFraction;
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

			pose.offsetY = ramp * WaveAmplitude * std::sin(phase);
			pose.rotationDegrees = ramp * WaveSwayDegrees * std::sin(phase * 0.5f);

			// Brightest as the letter rides up (sin negative => higher on screen).
			const float glowWave = 0.5f - 0.5f * std::sin(phase);
			pose.glowStrength = Lerp(1.f, Lerp(GlowMin, GlowMax, glowWave), ramp);
		}

		return pose;
	}

	sf::Vector2f DropInTitle::GetRestingPosition(std::size_t index, const Pose& pose) const
	{
		return { center.x + glyphs[index].offsetX,
				 center.y + kickOffset + glyphs[index].offsetY + pose.offsetY + GetExitLift() };
	}

	void DropInTitle::DrawGradientLetter(sf::RenderTarget& target, std::size_t index, const Pose& pose,
		sf::Vector2f drawPosition) const
	{
		const Glyph& glyph = glyphs[index];

		sf::Color base = glyph.color;
		if (pose.flash > 0.f)
		{
			base = MixToWhite(base, pose.flash);
		}

		sf::Color top = MixToWhite(base, GradientTopMix);
		sf::Color bottom = Darken(base, GradientBottomFactor);
		sf::Color outlineColor = Darken(base, OutlineDarkenFactor);

		// Fade the whole word out while it flies off.
		if (const float exitAlpha = GetExitAlpha(); exitAlpha < 1.f)
		{
			top.a = ToByte(static_cast<float>(top.a) * exitAlpha);
			bottom.a = ToByte(static_cast<float>(bottom.a) * exitAlpha);
			outlineColor.a = ToByte(static_cast<float>(outlineColor.a) * exitAlpha);
		}

		const sf::Glyph& fontGlyph = font.getGlyph(glyph.codepoint, characterSize, false);
		const sf::FloatRect texRect(fontGlyph.textureRect);
		const sf::Vector2f inkCenter{
			fontGlyph.bounds.position.x + fontGlyph.bounds.size.x * 0.5f,
			fontGlyph.bounds.position.y + fontGlyph.bounds.size.y * 0.5f };

		sf::Transform transform;
		transform.translate(drawPosition);
		transform.rotate(sf::degrees(pose.rotationDegrees));
		transform.scale({ pose.scaleX, pose.scaleY });
		transform.translate(-inkCenter);

		// Outline: the same glyph, grown, in a dark shade of its own hue.
		sf::Text outline = glyph.text;
		outline.setPosition(drawPosition);
		outline.setRotation(sf::degrees(pose.rotationDegrees));
		outline.setScale({ pose.scaleX, pose.scaleY });
		outline.setFillColor(sf::Color::Transparent);
		outline.setOutlineColor(outlineColor);
		target.draw(outline);

		sf::VertexArray fill(sf::PrimitiveType::Triangles);
		AppendQuad(fill, fontGlyph.bounds, texRect, top, bottom);

		sf::RenderStates states;
		states.transform = transform;
		states.texture = &font.getTexture(characterSize);
		target.draw(fill, states);

		// Chromatic split: additive red / blue copies pulled apart briefly.
		if (pose.aberration > 0.f)
		{
			const float shift = pose.aberration * AberrationOffset;
			const std::uint8_t aberAlpha = ToByte(pose.aberration * AberrationAlphaScale);

			for (int side = 0; side < 2; ++side)
			{
				const sf::Color tint = side == 0
					? sf::Color(AberrationRedTint.r, AberrationRedTint.g, AberrationRedTint.b, aberAlpha)
					: sf::Color(AberrationBlueTint.r, AberrationBlueTint.g, AberrationBlueTint.b, aberAlpha);
				const float dx = side == 0 ? -shift : shift;

				sf::Transform ghost;
				ghost.translate({ drawPosition.x + dx, drawPosition.y });
				ghost.rotate(sf::degrees(pose.rotationDegrees));
				ghost.scale({ pose.scaleX, pose.scaleY });
				ghost.translate(-inkCenter);

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
		sf::Vector2f position) const
	{
		// Fixed-size box centered on the letter -- same size for every letter and
		// every frame, so NeonGlow's buffers never resize after the first call.
		const sf::FloatRect area{
			{ position.x - glowBoxSize.x * 0.5f, position.y - glowBoxSize.y * 0.5f },
			glowBoxSize };

		const float strength = std::max(pose.glowStrength, GlowFloor + GlowFlashWeight * pose.flash);
		const sf::Color tint = ScaleRgb(glyphs[index].color, strength * GlowIntensity);

		sf::Text silhouette = glyphs[index].text;
		silhouette.setPosition(position);
		silhouette.setRotation(sf::degrees(pose.rotationDegrees));
		silhouette.setScale({ pose.scaleX, pose.scaleY });

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
			if (!poses[i].isFalling)
			{
				continue;
			}

			for (int ghostIndex = 1; ghostIndex <= GhostCount; ++ghostIndex)
			{
				const float ghostLocalTime = poses[i].localTime - static_cast<float>(ghostIndex) * GhostStep;
				if (ghostLocalTime <= 0.f)
				{
					break;
				}

				const float ghostFallFraction = ghostLocalTime / FallDuration;
				const float ghostY = -DropDistance * (1.f - ghostFallFraction * ghostFallFraction);

				sf::Text ghost = glyphs[i].text;
				ghost.setOutlineThickness(0.f);
				ghost.setPosition({ center.x + glyphs[i].offsetX,
					center.y + kickOffset + glyphs[i].offsetY + ghostY });
				ghost.setFillColor(sf::Color(glyphs[i].color.r, glyphs[i].color.g, glyphs[i].color.b,
					ToByte(GhostAlpha * (1.f - static_cast<float>(ghostIndex) / (GhostCount + 1)))));
				target.draw(ghost);
			}
		}

		// -- Per-letter neon bloom (dropped once the word starts leaving) ----
		if (glow != nullptr && exitTime < 0.f)
		{
			for (std::size_t i = 0; i < glyphs.size(); ++i)
			{
				if (poses[i].isVisible)
				{
					DrawLetterGlow(target, *glow, i, poses[i], GetRestingPosition(i, poses[i]));
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

			const float shockEase = EaseOutQuad(poses[i].shock);
			const float radius = Lerp(ShockRadiusStart, ShockRadiusEnd, shockEase);

			sf::CircleShape ring(radius);
			ring.setOrigin({ radius, radius });
			ring.setPosition(GetRestingPosition(i, poses[i]));
			ring.setFillColor(sf::Color::Transparent);
			ring.setOutlineThickness(Lerp(ShockOutlineThicknessStart, ShockOutlineThicknessEnd, shockEase));
			ring.setOutlineColor(sf::Color(glyphs[i].color.r, glyphs[i].color.g, glyphs[i].color.b,
				ToByte((1.f - poses[i].shock) * ShockAlphaScale)));
			target.draw(ring);
		}

		// -- The letters themselves ----------------------------------------
		for (std::size_t i = 0; i < glyphs.size(); ++i)
		{
			if (!poses[i].isVisible)
			{
				continue;
			}

			DrawGradientLetter(target, i, poses[i], GetRestingPosition(i, poses[i]));
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
