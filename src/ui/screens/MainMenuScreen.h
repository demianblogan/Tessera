#pragma once

#include <optional>

#include <SFML/Graphics/Color.hpp>

#include "../../primitives/NeonGlow.h"
#include "../CarouselMenu.h"
#include "../DropInTitle.h"
#include "MenuScreen.h"

namespace sf
{
	class Event;
	class RenderTarget;
}

// The top-level menu screen: the animated "TESSERA" title drops in, then a
// rotating ring of entries flies in around it. Horizontal navigation turns the
// ring; the front entry activates. The shared background, version stamp and
// lightbar handoff belong to the host.
class MainMenuScreen final : public MenuScreen
{
public:
	// `animate` false skips the build-in animation and `frontEntry` picks which
	// ring entry starts at the front -- used when the host brings the main menu
	// back after a sub-screen, so it reappears focused on the entry that was
	// activated.
	explicit MainMenuScreen(ScreenHost& host, bool isAnimated = true, std::size_t frontEntry = 0);

	void HandleEvent(const sf::Event& event) override;
	void Update(float deltaTime) override;
	void Render(sf::RenderTarget& target) override;

	void PlayActivatePulse() override;
	void StartExit() override;
	[[nodiscard]] bool IsExitFinished() const override;

	// The front ring entry -- the host's header sinks back to this when
	// returning from a sub-screen.
	[[nodiscard]] sf::Vector2f GetHeaderReturnCenter() const override;
	[[nodiscard]] float GetHeaderReturnHeight() const override;

	// The ring index currently at the front, for the host to focus the rebuilt
	// menu on the same entry when returning.
	[[nodiscard]] std::size_t GetCurrentFrontIndex() const;

	[[nodiscard]] std::optional<sf::Color> GetLightbarColor() const override;

	// No cursor while the title / ring build animation is still playing.
	[[nodiscard]] bool IsCursorVisible() const override;

private:
	UI::DropInTitle title;
	NeonGlow titleGlow;
	NeonGlow entryGlow;
	UI::CarouselMenu carousel;
	bool hasCarouselStarted = false;

	bool isExiting = false;
	float exitTimer = 0.f;
};
