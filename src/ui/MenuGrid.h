#pragma once

namespace sf
{
	class RenderTarget;
}

namespace UI
{
	// A synthwave floor: a neon grid in perspective across the lower part of
	// the screen, its rows scrolling toward the viewer. Purely decorative.
	class MenuGrid
	{
	public:
		void Update(float deltaTime);
		void Render(sf::RenderTarget& target) const;

	private:
		float scroll = 0.f;   // 0..1, loops
	};
}
