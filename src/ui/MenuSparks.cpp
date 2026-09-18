#include "MenuSparks.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numbers>

#include <SFML/Graphics/BlendMode.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

#include "../display/DisplaySettings.h"
#include "TetrominoPalette.h"

namespace
{
	using Display::VirtualSize;

	constexpr float Pi = std::numbers::pi_v<float>;
	constexpr float TwoPi = 2.f * Pi;

	constexpr int SparkCount = 46;

	constexpr float MinSize = 1.6f;
	constexpr float MaxSize = 4.6f;
	constexpr float MinRise = 11.f;
	constexpr float MaxRise = 42.f;
	constexpr float MinLifetime = 6.f;
	constexpr float MaxLifetime = 13.f;
	constexpr float MinPeakAlpha = 0.10f;
	constexpr float MaxPeakAlpha = 0.32f;

	constexpr float MinSwaySpeed = 0.35f;
	constexpr float MaxSwaySpeed = 0.95f;
	constexpr float MinSwayAmp = 8.f;
	constexpr float MaxSwayAmp = 20.f;

	// Render() -- fade envelope: rises over the first fraction of the
	// lifetime, then falls over the last fraction, holding at full alpha
	// in between.
	constexpr float FadeInSpan = 0.18f;
	constexpr float FadeOutSpan = 0.35f;

	// Respawn() -- a spark respawning after rising off the top starts a bit
	// below the bottom edge instead, scattered across this span.
	constexpr float RespawnBelowScreenOffset = 20.f;
	constexpr float RespawnBelowScreenRange = 120.f;

	// Update() -- a spark despawns once it has risen this far above the top
	// edge (it should already have faded out by then).
	constexpr float DespawnAboveScreenY = -40.f;

	constexpr std::size_t SparkPointCount = 10;
}

namespace UI
{
	MenuSparks::MenuSparks()
		: randomEngine(std::random_device{}())
	{
		sparks.resize(SparkCount);
		for (Spark& spark : sparks)
			Respawn(spark, true);
	}

	void MenuSparks::Respawn(Spark& spark, bool isInitial)
	{
		std::uniform_real_distribution<float> unit(0.f, 1.f);
		std::uniform_int_distribution<int> colorPick(0, static_cast<int>(UI::TetrominoColors.size()) - 1);

		spark.baseX = unit(randomEngine) * VirtualSize.x;
		spark.position =
		{
			spark.baseX,
			isInitial
				? unit(randomEngine) * VirtualSize.y
				: VirtualSize.y + RespawnBelowScreenOffset + unit(randomEngine) * RespawnBelowScreenRange
		};
		spark.riseSpeed = MinRise + unit(randomEngine) * (MaxRise - MinRise);
		spark.swayPhase = unit(randomEngine) * TwoPi;
		spark.swaySpeed = MinSwaySpeed + unit(randomEngine) * (MaxSwaySpeed - MinSwaySpeed);
		spark.swayAmp = MinSwayAmp + unit(randomEngine) * (MaxSwayAmp - MinSwayAmp);
		spark.size = MinSize + unit(randomEngine) * (MaxSize - MinSize);
		spark.lifetime = MinLifetime + unit(randomEngine) * (MaxLifetime - MinLifetime);
		spark.age = isInitial ? unit(randomEngine) * spark.lifetime : 0.f;
		spark.peakAlpha = MinPeakAlpha + unit(randomEngine) * (MaxPeakAlpha - MinPeakAlpha);
		spark.color = UI::TetrominoColors[static_cast<std::size_t>(colorPick(randomEngine))];
	}

	void MenuSparks::Update(float deltaTime)
	{
		for (Spark& spark : sparks)
		{
			spark.age += deltaTime;
			spark.position.y -= spark.riseSpeed * deltaTime;
			spark.position.x = spark.baseX + std::sin(spark.age * spark.swaySpeed + spark.swayPhase) * spark.swayAmp;

			if (spark.age >= spark.lifetime || spark.position.y < DespawnAboveScreenY)
				Respawn(spark, false);
		}
	}

	void MenuSparks::Render(sf::RenderTarget& target) const
	{
		sf::RenderStates additive;
		additive.blendMode = sf::BlendAdd;

		sf::CircleShape dot;
		dot.setPointCount(SparkPointCount);

		for (const Spark& spark : sparks)
		{
			const float life = std::clamp(spark.age / spark.lifetime, 0.f, 1.f);
			const float envelope = std::min(life / FadeInSpan, (1.f - life) / FadeOutSpan);   // fade in, hold, fade out
			const float alpha = spark.peakAlpha * std::clamp(envelope, 0.f, 1.f);
			if (alpha <= 0.f)
				continue;

			dot.setRadius(spark.size);
			dot.setOrigin({ spark.size, spark.size });
			dot.setPosition(spark.position);
			dot.setFillColor(sf::Color(spark.color.r, spark.color.g, spark.color.b, static_cast<std::uint8_t>(alpha * 255.f)));

			target.draw(dot, additive);
		}
	}
}
