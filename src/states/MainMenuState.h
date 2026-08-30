#pragma once

#include <SFML/Graphics/Text.hpp>

#include "../core/State.h"
#include "../rendering/NeonGlow.h"
#include "../ui/CarouselMenu.h"
#include "../ui/DropInTitle.h"

struct Context;

namespace sf
{
	class Event;
}

// The main menu: the animated "TESSERA" title drops in, then a rotating ring
// of entries flies in around it. A standalone State (not MenuScreenState) --
// its navigation model is its own.
class MainMenuState final : public State
{
public:
	explicit MainMenuState(Context& context);

	void HandleEvent(const sf::Event& event) override;
	void Update(float deltaTime) override;
	void Render(sf::RenderTarget& target) override;

	// No cursor while the title / ring build animation is still playing.
	[[nodiscard]] bool ShowsCursor() const override { return carousel.IsReady(); }

private:
	Context& context;

	UI::DropInTitle title;
	NeonGlow titleGlow;
	UI::CarouselMenu carousel;
	bool carouselStarted = false;

	sf::Text versionText;
};
