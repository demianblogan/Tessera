#include "MenuHeader.h"

#include <algorithm>
#include <cmath>

#include <SFML/Graphics/RenderTarget.hpp>

#include "ColourUtils.h"
#include "Easing.h"

namespace
{
	constexpr unsigned int HeaderTextSize = 110;
	constexpr sf::Vector2f HeaderCentre{ 960.f, 120.f };

	constexpr float RiseDuration = 0.28f;
	constexpr float SinkDuration = 0.24f;

	constexpr float GlowIntensity = 0.55f;
	constexpr float GlowBreathSpeed = 2.0f;

	using UI::Easing::EaseOutCubic;
	using UI::Easing::Lerp;
}

namespace UI
{
	MenuHeader::MenuHeader(const sf::Font& font, sf::Shader& dilateShader, sf::Shader& blurShader)
		: label(font, HeaderTextSize)
		, glow(dilateShader, blurShader)
	{
	}

	void MenuHeader::RiseFrom(sf::Vector2f fromCentre, float fromHeight, const sf::String& text, sf::Color newColour)
	{
		colour = newColour;
		label.SetText(text);

		fromPosition = fromCentre;
		toPosition = HeaderCentre;
		fromScale = std::max(fromHeight, 1.f) / label.InkSize().y;
		toScale = 1.f;

		mode = Mode::Rising;
		timer = 0.f;
	}

	void MenuHeader::SinkTo(sf::Vector2f toCentre, float toHeight)
	{
		fromPosition = HeaderCentre;
		toPosition = toCentre;
		fromScale = 1.f;
		toScale = std::max(toHeight, 1.f) / label.InkSize().y;

		mode = Mode::Sinking;
		timer = 0.f;
	}

	void MenuHeader::Update(float deltaTime)
	{
		animTime += deltaTime;
		label.Update(deltaTime);
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
		if (mode == Mode::Hidden)
		{
			return;
		}

		const Pose pose = CurrentPose();
		if (pose.alpha <= 0.f)
		{
			return;
		}

		const float breath = 0.85f + 0.15f * std::sin(animTime * GlowBreathSpeed);
		const sf::Color glowTint = ScaleRgb(colour, GlowIntensity * breath * std::clamp(pose.alpha, 0.f, 1.f));

		label.DrawGlow(target, glow, pose.position, pose.scale, glowTint);
		label.Draw(target, pose.position, pose.scale, colour, pose.alpha);
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
