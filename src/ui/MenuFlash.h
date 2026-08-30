#pragma once

namespace sf
{
	class RenderTarget;
}

namespace UI
{
	// A brief, subtle full-screen brighten of the menu background, triggered
	// each time the ring is rotated. Additive, decays fast.
	class MenuFlash
	{
	public:
		void Pulse();
		void Update(float deltaTime);
		void Render(sf::RenderTarget& target) const;

	private:
		float intensity = 0.f;   // 0..1, decaying
	};
}
