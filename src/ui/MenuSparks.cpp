#include "MenuSparks.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

#include <SFML/Graphics/BlendMode.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

#include "TetrominoPalette.h"

namespace
{
	constexpr sf::Vector2f VirtualSize{ 1920.f, 1080.f };

	constexpr int SparkCount = 46;

	constexpr float MinSize = 1.6f;
	constexpr float MaxSize = 4.6f;
	constexpr float MinRise = 11.f;
	constexpr float MaxRise = 42.f;
	constexpr float MinLifetime = 6.f;
	constexpr float MaxLifetime = 13.f;
	constexpr float MinPeakAlpha = 0.10f;
	constexpr float MaxPeakAlpha = 0.32f;
}

namespace UI
{
	MenuSparks::MenuSparks()
		: rng(std::random_device{}())
	{
		sparks.resize(SparkCount);
		for (Spark& spark : sparks)
		{
			Respawn(spark, true);
		}
	}

	void MenuSparks::Respawn(Spark& spark, bool initial)
	{
		std::uniform_real_distribution<float> unit(0.f, 1.f);
		std::uniform_int_distribution<int> colourPick(0, static_cast<int>(UI::TetrominoColours.size()) - 1);

		spark.baseX = unit(rng) * VirtualSize.x;
		spark.position = {
			spark.baseX,
			initial ? unit(rng) * VirtualSize.y : VirtualSize.y + 20.f + unit(rng) * 120.f };
		spark.riseSpeed = MinRise + unit(rng) * (MaxRise - MinRise);
		spark.swayPhase = unit(rng) * 6.2832f;
		spark.swaySpeed = 0.35f + unit(rng) * 0.95f;
		spark.swayAmp = 8.f + unit(rng) * 20.f;
		spark.size = MinSize + unit(rng) * (MaxSize - MinSize);
		spark.lifetime = MinLifetime + unit(rng) * (MaxLifetime - MinLifetime);
		spark.age = initial ? unit(rng) * spark.lifetime : 0.f;
		spark.peakAlpha = MinPeakAlpha + unit(rng) * (MaxPeakAlpha - MinPeakAlpha);
		spark.colour = UI::TetrominoColours[static_cast<std::size_t>(colourPick(rng))];
	}

	void MenuSparks::Update(float deltaTime)
	{
		for (Spark& spark : sparks)
		{
			spark.age += deltaTime;
			spark.position.y -= spark.riseSpeed * deltaTime;
			spark.position.x = spark.baseX + std::sin(spark.age * spark.swaySpeed + spark.swayPhase) * spark.swayAmp;

			if (spark.age >= spark.lifetime || spark.position.y < -40.f)
			{
				Respawn(spark, false);
			}
		}
	}

	void MenuSparks::Render(sf::RenderTarget& target) const
	{
		sf::RenderStates additive;
		additive.blendMode = sf::BlendAdd;

		sf::CircleShape dot;
		dot.setPointCount(10);

		for (const Spark& spark : sparks)
		{
			const float life = std::clamp(spark.age / spark.lifetime, 0.f, 1.f);
			const float envelope = std::min(life / 0.18f, (1.f - life) / 0.35f);   // fade in, hold, fade out
			const float alpha = spark.peakAlpha * std::clamp(envelope, 0.f, 1.f);
			if (alpha <= 0.f)
			{
				continue;
			}

			dot.setRadius(spark.size);
			dot.setOrigin({ spark.size, spark.size });
			dot.setPosition(spark.position);
			dot.setFillColor(sf::Color(spark.colour.r, spark.colour.g, spark.colour.b,
				static_cast<std::uint8_t>(alpha * 255.f)));
			target.draw(dot, additive);
		}
	}
}
