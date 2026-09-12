#pragma once

#include <array>
#include <vector>

#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Text.hpp>

#include "../core/State.h"
#include "../localization/Language.h"
#include "../ui/MenuAurora.h"
#include "../ui/MenuBackdrop.h"
#include "../ui/MenuButtonColumn.h"
#include "../ui/MenuSparks.h"

struct Context;

namespace sf
{
	class Event;
	class RenderTarget;
}

// Shown once, between the company splash and the main menu, on a fresh
// install (GameSettings::languageChosen is false). The same ambient
// background as MenuShell (aurora, drifting tetrominoes, sparks) sits behind
// a "choose your language" line -- one phrase per language, dropping in
// letter by letter like the main-menu title -- and a vertical list of the 5
// languages that flies in from below at the same time. Each language has its
// own accent colour (LanguageAccent); hovering or selecting one brightens,
// grows and glows the matching phrase so the connection is obvious. Picking
// one applies it, persists the choice, and hands off to MenuShell; Options >
// Language is how a player changes their mind afterwards -- this screen
// never reappears on its own.
class LanguagePickerState final : public State
{
public:
	explicit LanguagePickerState(Context& context);

	void HandleEvent(const sf::Event& event) override;
	void Update(float deltaTime) override;
	void Render(sf::RenderTarget& target) override;

private:
	static constexpr float FadeDuration = 0.4f;

	// One falling letter of the prompt line. Belongs to a language's phrase,
	// or -- for the "  ·  " dividers between phrases -- to none.
	struct PromptGlyph
	{
		sf::Text text;
		int segmentIndex = -1;
		float restX = 0.f;
		float restY = 0.f;
		float startDelay = 0.f;
	};

	// One phrase of the prompt line ("Choose your language", ...), the
	// language it names, and how brightly it is currently picked out.
	struct PromptSegment
	{
		Language language = Language::English;
		float pivotX = 0.f;
		float highlight = 0.f;   // eased 0..1, 1 = this language is selected
	};

	void BuildPrompt();
	void SetHovered(Language language);
	void Choose(Language language);
	void Finish();

	Context& context;

	// Background -- identical ambient to MenuShell.
	sf::Sprite backgroundSprite;
	UI::MenuAurora aurora;
	UI::MenuBackdrop backdrop;
	UI::MenuSparks sparks;

	std::vector<PromptGlyph> promptGlyphs;
	std::array<PromptSegment, LanguageCount> segments;
	Language hoveredLanguage = Language::English;
	float promptElapsed = 0.f;
	float fallDuration = 0.35f;
	float fallStagger = 0.01f;

	UI::MenuButtonColumn column;

	float fade = 1.f;   // 1 = fully black, 0 = clear
	bool leaving = false;
};
