#include "MenuAurora.h"

#include <SFML/Graphics/Glsl.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Shader.hpp>

namespace
{
	constexpr float VirtualWidth = 1920.f;
	constexpr float VirtualHeight = 1080.f;
}

namespace UI
{
	MenuAurora::MenuAurora(sf::Shader& auroraShader)
		: shader(auroraShader)
	{
	}

	void MenuAurora::Update(float deltaTime)
	{
		time += deltaTime;
	}

	void MenuAurora::Render(sf::RenderTarget& target) const
	{
		shader.setUniform("time", time);
		shader.setUniform("resolution", sf::Glsl::Vec2(VirtualWidth, VirtualHeight));

		sf::RectangleShape quad({ VirtualWidth, VirtualHeight });

		sf::RenderStates states;
		states.shader = &shader;
		target.draw(quad, states);
	}
}
