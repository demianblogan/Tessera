#pragma once

#include <cstddef>
#include <functional>
#include <vector>

#include <SFML/Graphics/Text.hpp>
#include <SFML/System/String.hpp>

#include "../core/State.h"
#include "../rendering/NeonGlow.h"
#include "../ui/DropInTitle.h"

struct Context;

namespace sf
{
	class Event;
}

// The main menu: the animated "TESSERA" title drops in, then a list of entries.
// A standalone State (not MenuScreenState) -- its navigation model is its own.
class MainMenuState final : public State
{
public:
	explicit MainMenuState(Context& context);

	void HandleEvent(const sf::Event& event) override;
	void Update(float deltaTime) override;
	void Render(sf::RenderTarget& target) override;

private:
	struct Entry
	{
		sf::Text label;
		std::function<void()> activate;
	};

	Context& context;

	UI::DropInTitle title;
	NeonGlow titleGlow;
	std::vector<Entry> entries;
	std::size_t selected = 0;

	sf::Text versionText;

	void AddEntry(const sf::String& text, std::function<void()> activate);
	[[nodiscard]] bool MenuReady() const;
	void Select(std::size_t index);
	void Activate();
};
