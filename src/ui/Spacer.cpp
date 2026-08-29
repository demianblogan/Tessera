#include "Spacer.h"

namespace UI
{
	Spacer::Spacer(sf::Vector2f size)
		: preferredSize(size)
	{
		SetWidthPixels(size.x);
		SetHeightPixels(size.y);
	}

	sf::Vector2f Spacer::Measure() const
	{
		return preferredSize;
	}

	void Spacer::Render(sf::RenderTarget& /* target */) const
	{
		// A spacer only reserves layout room; it never draws.
	}
}