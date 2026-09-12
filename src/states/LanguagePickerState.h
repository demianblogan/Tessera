#pragma once

#include <SFML/Graphics/Text.hpp>

#include "../core/State.h"
#include "../localization/Language.h"
#include "../ui/MenuButtonColumn.h"

struct Context;

namespace sf
{
	class Event;
	class RenderTarget;
}

// Shown once, between the company splash and the main menu, on a fresh
// install (GameSettings::languageChosen is false). A plain vertical list of
// the 5 languages, each labelled in its own native name so it reads before
// any language is even chosen. Picking one applies it, persists the choice,
// and hands off to MenuShell; Options > Language is how a player changes
// their mind afterwards -- this screen never reappears on its own.
class LanguagePickerState final : public State
{
public:
	explicit LanguagePickerState(Context& context);

	void HandleEvent(const sf::Event& event) override;
	void Update(float deltaTime) override;
	void Render(sf::RenderTarget& target) override;

private:
	static constexpr float FadeDuration = 0.4f;

	void Choose(Language language);
	void Finish();

	Context& context;

	sf::Text prompt;
	UI::MenuButtonColumn column;

	float fade = 1.f;   // 1 = fully black, 0 = clear
	bool leaving = false;
};
