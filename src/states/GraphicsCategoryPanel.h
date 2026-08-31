#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Text.hpp>

#include "../settings/GameSettings.h"
#include "../ui/NineSliceFrame.h"
#include "../ui/OptionRow.h"
#include "OptionsCategoryPanel.h"

struct Context;

namespace sf
{
	class Event;
	class RenderTarget;
}

// The Graphics category content: a framed list of setting rows -- screen
// resolution and window mode (carousels), vertical sync / show FPS / CRT filter
// (toggles). Edits go into a working copy; nothing is applied or saved yet --
// the Apply / Reset / Back row does that.
class GraphicsCategoryPanel final : public OptionsCategoryPanel
{
public:
	GraphicsCategoryPanel(Context& context, sf::Color accent);

	void Open() override;

	void SetVisibility(Visibility visibility, float previewFade) override;
	void Update(float deltaTime) override;
	void Render(sf::RenderTarget& target) override;
	bool HandleEvent(const sf::Event& event) override;

private:
	void BuildRows();
	void MoveSelection(int direction);
	[[nodiscard]] std::size_t ResolutionIndexFor(sf::Vector2u resolution) const;

	Context& context;
	sf::Color accent;

	UI::NineSliceFrame frame;
	sf::Text titleText;

	std::vector<std::unique_ptr<UI::OptionRow>> rows;
	std::size_t selected = 0;
	std::size_t resolutionRow = 0;

	std::vector<sf::Vector2u> resolutions;
	GameSettings working;

	float alpha = 0.f;
	float targetAlpha = 0.f;
	bool active = false;
};
