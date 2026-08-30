#pragma once

#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Text.hpp>

#include "../core/State.h"
#include "../rendering/NeonGlow.h"
#include "../ui/CarouselMenu.h"
#include "../ui/DropInTitle.h"
#include "../ui/MenuBackdrop.h"
#include "../ui/MenuSparks.h"

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

	// TEMP: CRT off on the menu while the background is being designed.
	[[nodiscard]] bool UsesCrtEffect() const override { return false; }

private:
	Context& context;

	sf::Sprite backgroundSprite;
	UI::MenuBackdrop backdrop;
	UI::MenuSparks sparks;
	UI::DropInTitle title;
	NeonGlow titleGlow;
	UI::CarouselMenu carousel;
	bool carouselStarted = false;

	sf::Text versionText;
};
