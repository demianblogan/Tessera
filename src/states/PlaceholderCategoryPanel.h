#pragma once

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/System/String.hpp>

#include "../ui/NineSliceFrame.h"
#include "OptionsCategoryPanel.h"

struct Context;

namespace sf
{
	class Event;
	class RenderTarget;
}

// A stand-in category panel used while the real content (Graphics, Audio, ...)
// is being built: a framed area with the category name and "Coming soon".
class PlaceholderCategoryPanel final : public OptionsCategoryPanel
{
public:
	PlaceholderCategoryPanel(Context& context, const sf::String& title, sf::Color accent);

	void Open() override;
	void Update(float deltaTime) override;
	void RenderPreview(sf::RenderTarget& target, float alpha) override;
	void RenderContent(sf::RenderTarget& target) override;
	bool HandleEvent(const sf::Event& event) override;

private:
	void Draw(sf::RenderTarget& target, float alpha);

	sf::Color accent;
	UI::NineSliceFrame frame;
	sf::Text titleText;
	sf::Text bodyText;

	float contentFade = 0.f;
};
