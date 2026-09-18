#include "SceneMotion.h"

#include <algorithm>
#include <cmath>

namespace
{
	// Spring pulling the offset back to rest.
	constexpr float Stiffness = 26.f;
	constexpr float Damping = 7.5f;

	// Hard limit so a flurry of nudges can't slide the backdrop off its overscan.
	constexpr float MaxX = 46.f;
	constexpr float MaxY = 30.f;

	// The always-on drift: two independent sine waves, deliberately at
	// different speeds and phases so the motion doesn't trace a simple
	// repeating line/ellipse.
	constexpr float IdleAmpX = 7.f;
	constexpr float IdleAmpY = 4.5f;
	constexpr float IdleSpeedX = 0.13f;
	constexpr float IdleSpeedY = 0.09f;
	constexpr float IdlePhaseOffsetY = 1.7f;

	[[nodiscard]] float Clamp(float value, float limit)
	{
		return std::clamp(value, -limit, limit);
	}
}

void SceneMotion::Nudge(sf::Vector2f impulse)
{
	velocity += impulse;
}

void SceneMotion::Update(float deltaTime)
{
	// Semi-implicit Euler on a damped spring toward the origin.
	velocity += (-Stiffness * position - Damping * velocity) * deltaTime;
	position += velocity * deltaTime;

	position.x = Clamp(position.x, MaxX);
	position.y = Clamp(position.y, MaxY);

	idleTime += deltaTime;
}

sf::Vector2f SceneMotion::GetOffset() const
{
	const sf::Vector2f drift
	{
		std::sin(idleTime * IdleSpeedX) * IdleAmpX,
		std::sin(idleTime * IdleSpeedY + IdlePhaseOffsetY) * IdleAmpY
	};

	return position + drift;
}
