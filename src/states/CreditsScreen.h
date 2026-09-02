#pragma once

#include <optional>
#include <vector>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Text.hpp>

#include "../rendering/NeonGlow.h"
#include "../ui/MenuLabel.h"
#include "../ui/NineSliceFrame.h"
#include "MenuScreen.h"

namespace sf
{
	class Event;
	class RenderTarget;
}

// The Credits sub-screen: a 9-slice framed panel with a short note about the
// developer, and -- below the frame -- a single "Back to Main Menu" text button
// drawn like the main-menu entries (outline, gradient, glow, idle wave). The
// "CREDITS" header above it is the shell's, morphed from the menu entry.
class CreditsScreen final : public MenuScreen
{
public:
	CreditsScreen(ScreenHost& host, sf::Color accent);

	void HandleEvent(const sf::Event& event) override;
	void Update(float deltaTime) override;
	void Render(sf::RenderTarget& target) override;

	void PlayIntro() override;
	void StartExit() override;
	[[nodiscard]] bool ExitFinished() const override;

	[[nodiscard]] std::optional<sf::Color> LightbarColour() const override { return accent; }

private:
	struct Line
	{
		sf::Text text;
		sf::Color colour;
	};

	void Leave();
	[[nodiscard]] float PanelAlpha() const;

	sf::Color accent;
	UI::NineSliceFrame panel;
	std::vector<Line> lines;

	UI::MenuLabel backLabel;
	NeonGlow backGlow;

	float introTime = 0.f;
	float exitTime = -1.f;      // >= 0 once leaving
	float pressTime = 1000.f;   // since the button was activated
	bool leaving = false;
};
