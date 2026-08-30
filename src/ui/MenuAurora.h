#pragma once

namespace sf
{
	class RenderTarget;
	class Shader;
}

namespace UI
{
	// Slow aurora curtains across the main-menu background, drawn as a
	// full-screen quad through menu_aurora.frag. Purely decorative.
	class MenuAurora
	{
	public:
		explicit MenuAurora(sf::Shader& shader);

		void Update(float deltaTime);
		void Render(sf::RenderTarget& target) const;

	private:
		sf::Shader& shader;
		float time = 0.f;
	};
}
