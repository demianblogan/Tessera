#include "CompanySplashState.h"

#include <algorithm>
#include <cstdint>
#include <memory>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Window/Event.hpp>

#include "../audio/AudioPlayer.h"
#include "../core/Context.h"
#include "../core/StateMachine.h"
#include "../resources/Assets.h"
#include "MainMenuState.h"

CompanySplashState::CompanySplashState(Context& context)
	: State(context.stateMachine)
	, context(context)
	, logo(context.textures.Get(Assets::TextureID::CompanyLogo))
{
	const sf::Vector2f textureSize(logo.getTexture().getSize());
	logo.setOrigin(textureSize * 0.5f);
	logo.setColor(sf::Color(255, 255, 255, 0));

	context.audioPlayer.Play(Assets::SoundID::CompanySplash);
}

void CompanySplashState::HandleEvent(const sf::Event& event)
{
	if (IsSkipEvent(event))
	{
		Finish();
	}
}

void CompanySplashState::Update(float deltaTime)
{
	if (isFinishing)
	{
		return;
	}

	elapsedTime += deltaTime;
	UpdateOpacity();

	if (elapsedTime >= FadeInDuration + HoldDuration + FadeOutDuration)
	{
		Finish();
	}
}

void CompanySplashState::Render(sf::RenderTarget& target)
{
	const sf::Vector2f targetSize = target.getView().getSize();
	const sf::Vector2f textureSize(logo.getTexture().getSize());

	if (textureSize.x != 0.f && textureSize.y != 0.f)
	{
		logo.setScale({ targetSize.x / textureSize.x, targetSize.y / textureSize.y });
	}

	logo.setPosition(targetSize * 0.5f);
	target.draw(logo);
}

bool CompanySplashState::IsSkipEvent(const sf::Event& event)
{
	return event.is<sf::Event::KeyPressed>()
		|| event.is<sf::Event::MouseButtonPressed>()
		|| event.is<sf::Event::JoystickButtonPressed>();
}

void CompanySplashState::Finish()
{
	if (isFinishing)
	{
		return;
	}

	isFinishing = true;
	RequestChange(std::make_unique<MainMenuState>(context));
}

void CompanySplashState::UpdateOpacity()
{
	float opacity = 1.f;

	if (elapsedTime < FadeInDuration)
	{
		opacity = elapsedTime / FadeInDuration;
	}
	else if (elapsedTime > FadeInDuration + HoldDuration)
	{
		const float fadeOutElapsed = elapsedTime - FadeInDuration - HoldDuration;
		opacity = 1.f - fadeOutElapsed / FadeOutDuration;
	}

	opacity = std::clamp(opacity, 0.f, 1.f);
	logo.setColor(sf::Color(255, 255, 255, static_cast<std::uint8_t>(opacity * 255.f)));
}
