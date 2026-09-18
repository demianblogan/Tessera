#include "Celebration.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <cstdint>

#include <SFML/Graphics/BlendMode.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

#include "../display/DisplaySettings.h"
#include "../utils/Random.h"

namespace
{
	using Display::VirtualSize;

	constexpr float Pi = std::numbers::pi_v<float>;

	constexpr float MinLaunchGap = 0.45f;
	constexpr float MaxLaunchGap = 1.25f;

	constexpr float RocketGravity = 110.f;
	constexpr int MinBurst = 34;
	constexpr int MaxBurst = 52;

	// Corner jets: total sparks per second across all four corners.
	constexpr float CornerEmitRate = 190.f;

	// Spawn a few pixels inside the frame edge so the jets don't visibly start
	// from a hard line on the border.
	constexpr float CornerInset = 7.f;
	constexpr float CornerFlashFade = 1.f / 0.09f;

	// Celebration::Celebration -- initial delay before the very first shell.
	constexpr float InitialLaunchDelayMin = 0.1f;
	constexpr float InitialLaunchDelayMax = 0.4f;

	// Explode() -- burst spark tuning ranges.
	constexpr float ExplodeBaseSpeedMin = 150.f;
	constexpr float ExplodeBaseSpeedMax = 260.f;
	constexpr float ExplodeSpeedSpreadMin = 0.35f;
	constexpr float ExplodeSpeedSpreadMax = 1.15f;
	constexpr float ExplodeSizeMin = 1.8f;
	constexpr float ExplodeSizeMax = 3.6f;
	constexpr float ExplodeLifeMin = 0.7f;
	constexpr float ExplodeLifeMax = 1.5f;
	constexpr float ExplodeGravityMin = 160.f;
	constexpr float ExplodeGravityMax = 260.f;
	constexpr float ExplodeDragMin = 0.28f;
	constexpr float ExplodeDragMax = 0.5f;
	constexpr int ExplodeColorJitterAmount = 22;

	// Update() -- rocket spawn / flight tuning.
	constexpr float RocketSpawnMarginX = 320.f;      // kept clear of the screen edges
	constexpr float RocketSpawnBelowScreenY = 20.f;  // spawns this far below the bottom edge
	constexpr float RocketVelocityXMin = -45.f;
	constexpr float RocketVelocityXMax = 45.f;
	constexpr float RocketVelocityYMin = -780.f;
	constexpr float RocketVelocityYMax = -600.f;
	constexpr float RocketFuseMin = 0.75f;
	constexpr float RocketFuseMax = 1.15f;
	constexpr float ApexVelocityThreshold = -30.f;   // near the top of the arc: explode even if the fuse hasn't burned out

	// Update() -- corner jet spark tuning.
	constexpr float CornerJetSpreadAngleMin = -0.6f;
	constexpr float CornerJetSpreadAngleMax = 0.6f;
	constexpr float CornerJetSpeedMin = 190.f;
	constexpr float CornerJetSpeedMax = 470.f;
	constexpr float CornerJetSizeMin = 2.6f;
	constexpr float CornerJetSizeMax = 5.4f;
	constexpr float CornerJetLifeMin = 0.5f;
	constexpr float CornerJetLifeMax = 1.1f;
	constexpr float CornerJetGravityMin = 120.f;
	constexpr float CornerJetGravityMax = 240.f;
	constexpr float CornerJetDragMin = 0.18f;
	constexpr float CornerJetDragMax = 0.4f;
	constexpr float CornerJetColorPickChance = 0.4f;   // chance of the paler of the two corner-jet colors

	// Shared culling boundary for both firework showers and corner jets: fades
	// out just above the buttons below the panel.
	constexpr float PanelBottomFadeY = 1006.f;
	constexpr float PanelBottomFadeSpan = 26.f;

	// RenderCornerSparks() -- muzzle-flash geometry and alpha scales.
	constexpr float CornerFlashOuterRadiusBase = 8.f;
	constexpr float CornerFlashOuterRadiusScale = 20.f;
	constexpr float CornerFlashInnerRadiusBase = 3.f;
	constexpr float CornerFlashInnerRadiusScale = 7.f;
	constexpr float CornerFlashOuterAlphaScale = 90.f;
	constexpr float CornerFlashInnerAlphaScale = 210.f;
	const sf::Color CornerFlashOuterColor{ 255, 210, 130 };
	const sf::Color CornerFlashInnerColor{ 255, 248, 232 };

	// Update() -- once a corner's muzzle flash fades out, it waits a random
	// span in this range before firing again.
	constexpr float CornerFlashRetriggerMin = 0.05f;
	constexpr float CornerFlashRetriggerMax = 0.20f;

	// Update() -- the two corner-jet spark colors (see CornerJetColorPickChance).
	const sf::Color CornerJetColorPale{ 255, 248, 224 };
	const sf::Color CornerJetColorWarm{ 255, 206, 110 };

	// SetCorners() -- unit diagonal (sqrt(2)/2), for aiming a jet at a true 45
	// degrees regardless of the panel's aspect ratio.
	constexpr float DiagonalUnit = 0.70710678f;

	// Render*() -- particle point counts (rounder shapes cost more to draw, so
	// only the few large muzzle-flash discs get the smoothest circle).
	constexpr unsigned int RocketPointCount = 10;
	constexpr unsigned int CornerFlashPointCount = 16;
	constexpr unsigned int CornerSparkPointCount = 8;

	// RenderFireworks() -- the rising shell itself, before it bursts.
	constexpr float RocketDotRadius = 2.6f;
	const sf::Color RocketDotColor{ 255, 240, 210 };

	const std::array<sf::Color, 6> WarmPalette = { {
		sf::Color(255, 214, 128),
		sf::Color(255, 176, 92),
		sf::Color(255, 240, 200),
		sf::Color(255, 148, 96),
		sf::Color(255, 226, 150),
		sf::Color(170, 214, 255),   // an occasional cool one
	} };

	[[nodiscard]] sf::Color PickWarm()
	{
		return WarmPalette[static_cast<std::size_t>(Random::Int(0, static_cast<int>(WarmPalette.size()) - 1))];
	}

	[[nodiscard]] sf::Color Jitter(sf::Color color, int amount)
	{
		const auto nudge = [&](std::uint8_t channel)
		{
			return static_cast<std::uint8_t>(std::clamp(static_cast<int>(channel) + Random::Int(-amount, amount), 0, 255));
		};
		return sf::Color(nudge(color.r), nudge(color.g), nudge(color.b));
	}
}

namespace UI
{
	Celebration::Celebration()
	{
		launchTimer = Random::Float(InitialLaunchDelayMin, InitialLaunchDelayMax);
		rockets.reserve(8);
		burstSparks.reserve(512);
		cornerSparks.reserve(256);
	}

	void Celebration::SetCorners(const std::array<sf::Vector2f, 4>& corners)
	{
		sf::Vector2f center{ 0.f, 0.f };
		for (const sf::Vector2f& corner : corners)
		{
			center += corner;
		}
		center /= 4.f;

		for (std::size_t i = 0; i < corners.size(); ++i)
		{
			const sf::Vector2f away = corners[i] - center;
			const float length = std::sqrt(away.x * away.x + away.y * away.y);
			const sf::Vector2f inward = length > 0.f ? -away / length : sf::Vector2f{ 0.f, 0.f };

			// Spawn just inside the frame.
			cornerPoints[i] = corners[i] + inward * CornerInset;

			// Each jet fires diagonally outward from its own corner: the top-left
			// corner up-and-left, the bottom-right down-and-right, and so on. Use
			// the sign of the offset (not its magnitude) so it is a true 45
			// degrees regardless of how wide the panel is.
			const float signX = away.x >= 0.f ? 1.f : -1.f;
			const float signY = away.y >= 0.f ? 1.f : -1.f;
			cornerDirections[i] = { signX * DiagonalUnit, signY * DiagonalUnit };
		}

		isCornersSet = true;
	}

	void Celebration::Explode(const Rocket& rocket)
	{
		const int count = Random::Int(MinBurst, MaxBurst);
		const float baseSpeed = Random::Float(ExplodeBaseSpeedMin, ExplodeBaseSpeedMax);

		for (int i = 0; i < count; ++i)
		{
			const float angle = Random::Float(0.f, 2.f * Pi);
			const float speed = baseSpeed * Random::Float(ExplodeSpeedSpreadMin, ExplodeSpeedSpreadMax);

			Spark spark;
			spark.position = rocket.position;
			spark.velocity = { std::cos(angle) * speed, std::sin(angle) * speed };
			spark.size = Random::Float(ExplodeSizeMin, ExplodeSizeMax);
			spark.maxLife = Random::Float(ExplodeLifeMin, ExplodeLifeMax);
			spark.life = spark.maxLife;
			spark.gravity = Random::Float(ExplodeGravityMin, ExplodeGravityMax);
			spark.drag = Random::Float(ExplodeDragMin, ExplodeDragMax);
			spark.color = Jitter(rocket.color, ExplodeColorJitterAmount);
			burstSparks.push_back(spark);
		}
	}

	void Celebration::Update(float deltaTime)
	{
		// Launch new shells.
		launchTimer -= deltaTime;
		if (launchTimer <= 0.f)
		{
			launchTimer = Random::Float(MinLaunchGap, MaxLaunchGap);

			Rocket rocket;
			rocket.position = { Random::Float(RocketSpawnMarginX, VirtualSize.x - RocketSpawnMarginX),
				VirtualSize.y + RocketSpawnBelowScreenY };
			rocket.velocity = { Random::Float(RocketVelocityXMin, RocketVelocityXMax),
				Random::Float(RocketVelocityYMin, RocketVelocityYMax) };
			rocket.fuse = Random::Float(RocketFuseMin, RocketFuseMax);
			rocket.color = PickWarm();
			rockets.push_back(rocket);
		}

		for (Rocket& rocket : rockets)
		{
			rocket.velocity.y += RocketGravity * deltaTime;
			rocket.position += rocket.velocity * deltaTime;
			rocket.fuse -= deltaTime;
		}

		for (auto it = rockets.begin(); it != rockets.end();)
		{
			if (it->fuse <= 0.f || it->velocity.y >= ApexVelocityThreshold)
			{
				Explode(*it);
				it = rockets.erase(it);
			}
			else
			{
				++it;
			}
		}

		const auto stepSpark = [deltaTime](Spark& spark)
		{
			const float keep = std::pow(spark.drag, deltaTime);
			spark.velocity *= keep;
			spark.velocity.y += spark.gravity * deltaTime;
			spark.position += spark.velocity * deltaTime;
			spark.life -= deltaTime;
		};

		for (Spark& spark : burstSparks)
		{
			stepSpark(spark);
		}
		std::erase_if(burstSparks, [](const Spark& spark) { return spark.life <= 0.f; });

		// Corner jets.
		if (isCornersSet)
		{
			for (std::size_t corner = 0; corner < cornerFlash.size(); ++corner)
			{
				cornerFlash[corner] = std::max(0.f, cornerFlash[corner] - deltaTime * CornerFlashFade);
				cornerFlashTimer[corner] -= deltaTime;
				if (cornerFlashTimer[corner] <= 0.f)
				{
					cornerFlash[corner] = 1.f;
					cornerFlashTimer[corner] = Random::Float(CornerFlashRetriggerMin, CornerFlashRetriggerMax);
				}
			}

			cornerEmitCarry += CornerEmitRate * deltaTime;
			const int toEmit = static_cast<int>(cornerEmitCarry);
			cornerEmitCarry -= static_cast<float>(toEmit);

			for (int i = 0; i < toEmit; ++i)
			{
				const std::size_t corner = static_cast<std::size_t>(Random::Int(0, 3));
				const sf::Vector2f direction = cornerDirections[corner];
				const float spread = Random::Float(CornerJetSpreadAngleMin, CornerJetSpreadAngleMax);
				const float cosSpread = std::cos(spread);
				const float sinSpread = std::sin(spread);
				const sf::Vector2f aimed{
					direction.x * cosSpread - direction.y * sinSpread,
					direction.x * sinSpread + direction.y * cosSpread };
				const float speed = Random::Float(CornerJetSpeedMin, CornerJetSpeedMax);

				Spark spark;
				spark.position = cornerPoints[corner];
				spark.velocity = aimed * speed;
				spark.size = Random::Float(CornerJetSizeMin, CornerJetSizeMax);
				spark.maxLife = Random::Float(CornerJetLifeMin, CornerJetLifeMax);
				spark.life = spark.maxLife;
				spark.gravity = Random::Float(CornerJetGravityMin, CornerJetGravityMax);
				spark.drag = Random::Float(CornerJetDragMin, CornerJetDragMax);
				spark.color = Random::Float(0.f, 1.f) < CornerJetColorPickChance
					? CornerJetColorPale
					: CornerJetColorWarm;
				cornerSparks.push_back(spark);
			}
		}

		for (Spark& spark : cornerSparks)
		{
			stepSpark(spark);
		}
		std::erase_if(cornerSparks, [](const Spark& spark) { return spark.life <= 0.f; });
	}

	void Celebration::RenderFireworks(sf::RenderTarget& target) const
	{
		sf::RenderStates additive;
		additive.blendMode = sf::BlendAdd;

		sf::CircleShape dot;
		dot.setPointCount(RocketPointCount);

		for (const Rocket& rocket : rockets)
		{
			dot.setRadius(RocketDotRadius);
			dot.setOrigin({ RocketDotRadius, RocketDotRadius });
			dot.setPosition(rocket.position);
			dot.setFillColor(RocketDotColor);
			target.draw(dot, additive);
		}

		for (const Spark& spark : burstSparks)
		{
			const float fade = std::clamp(spark.life / spark.maxLife, 0.f, 1.f);
			// Keep the shower clear of the buttons below the panel.
			const float regionFade = std::clamp((PanelBottomFadeY - spark.position.y) / PanelBottomFadeSpan, 0.f, 1.f);
			const float alpha = fade * fade * regionFade;
			if (alpha <= 0.f)
			{
				continue;
			}

			dot.setRadius(spark.size);
			dot.setOrigin({ spark.size, spark.size });
			dot.setPosition(spark.position);
			dot.setFillColor(sf::Color(spark.color.r, spark.color.g, spark.color.b,
				static_cast<std::uint8_t>(alpha * 255.f)));
			target.draw(dot, additive);
		}
	}

	void Celebration::RenderCornerSparks(sf::RenderTarget& target) const
	{
		sf::RenderStates additive;
		additive.blendMode = sf::BlendAdd;

		sf::CircleShape dot;
		dot.setPointCount(CornerFlashPointCount);

		// The muzzle flash at each corner where the jet fires from.
		for (std::size_t corner = 0; corner < cornerFlash.size(); ++corner)
		{
			const float flash = cornerFlash[corner];
			if (flash <= 0.f)
			{
				continue;
			}

			const float outer = CornerFlashOuterRadiusBase + CornerFlashOuterRadiusScale * flash;
			dot.setRadius(outer);
			dot.setOrigin({ outer, outer });
			dot.setPosition(cornerPoints[corner]);
			dot.setFillColor(sf::Color(CornerFlashOuterColor.r, CornerFlashOuterColor.g, CornerFlashOuterColor.b,
				static_cast<std::uint8_t>(flash * CornerFlashOuterAlphaScale)));
			target.draw(dot, additive);

			const float inner = CornerFlashInnerRadiusBase + CornerFlashInnerRadiusScale * flash;
			dot.setRadius(inner);
			dot.setOrigin({ inner, inner });
			dot.setPosition(cornerPoints[corner]);
			dot.setFillColor(sf::Color(CornerFlashInnerColor.r, CornerFlashInnerColor.g, CornerFlashInnerColor.b,
				static_cast<std::uint8_t>(flash * CornerFlashInnerAlphaScale)));
			target.draw(dot, additive);
		}

		dot.setPointCount(CornerSparkPointCount);

		for (const Spark& spark : cornerSparks)
		{
			const float fade = std::clamp(spark.life / spark.maxLife, 0.f, 1.f);
			const float regionFade = std::clamp((PanelBottomFadeY - spark.position.y) / PanelBottomFadeSpan, 0.f, 1.f);
			const float alpha = fade * regionFade;
			if (alpha <= 0.f)
			{
				continue;
			}

			dot.setRadius(spark.size);
			dot.setOrigin({ spark.size, spark.size });
			dot.setPosition(spark.position);
			dot.setFillColor(sf::Color(spark.color.r, spark.color.g, spark.color.b,
				static_cast<std::uint8_t>(alpha * 255.f)));
			target.draw(dot, additive);
		}
	}
}
