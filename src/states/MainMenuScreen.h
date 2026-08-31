#pragma once

#include <optional>

#include <SFML/Graphics/Color.hpp>

#include "../rendering/NeonGlow.h"
#include "../ui/CarouselMenu.h"
#include "../ui/DropInTitle.h"
#include "MenuScreen.h"

namespace sf
{
	class Event;
	class RenderTarget;
}

// The top-level menu screen: the animated "TESSERA" title drops in, then a
// rotating ring of entries flies in around it. Horizontal navigation turns the
// ring; the front entry activates. The shared background, version stamp and
// lightbar handoff belong to the MenuShell.
class MainMenuScreen final : public MenuScreen
{
public:
	// `animate` false skips the build-in animation -- used when the shell brings
	// the main menu back after a sub-screen, where it should appear at once.
	explicit MainMenuScreen(MenuShell& shell, bool animate = true);

	void HandleEvent(const sf::Event& event) override;
	void Update(float deltaTime) override;
	void Render(sf::RenderTarget& target) override;

	void StartExit() override;
	[[nodiscard]] bool ExitFinished() const override;

	[[nodiscard]] std::optional<sf::Color> LightbarColour() const override;

	// No cursor while the title / ring build animation is still playing.
	[[nodiscard]] bool ShowsCursor() const override { return carousel.IsReady(); }

private:
	UI::DropInTitle title;
	NeonGlow titleGlow;
	NeonGlow entryGlow;
	UI::CarouselMenu carousel;
	bool carouselStarted = false;

	bool exiting = false;
	float exitTimer = 0.f;
};
