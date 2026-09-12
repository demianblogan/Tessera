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
// own accent colour (LanguageAccent); hovering or selecting one brightens and
// grows the matching phrase so the connection is obvious. Picking one applies
// it, persists the choice, and hands off to MenuShell; Options > Language is
// how a player changes their mind afterwards -- this screen never reappears
// on its own.
//
// The intro (letters falling in) uses one sf::Text per character, which only
// exist and draw while it plays. Once every letter has landed, the whole
// prompt switches to one sf::Text per language phrase (steady() becomes
// true) -- far fewer draws for a screen that otherwise just sits idle
// waiting for a click.
class LanguagePickerState final : public State
{
public:
	explicit LanguagePickerState(Context& context);

	void HandleEvent(const sf::Event& event) override;
	void Update(float deltaTime) override;
	void Render(sf::RenderTarget& target) override;

private:
	static constexpr float FadeDuration = 0.4f;

	// One falling letter of the intro. Only used until introDone().
	struct IntroGlyph
	{
		sf::Text text;
		std::size_t segmentIndex = 0;
		float restX = 0.f;
		float restY = 0.f;
		float startDelay = 0.f;
	};

	// One phrase of the prompt line ("Choose your language", ...): the
	// language it names, its steady-state combined text (origin at its own
	// centre, so scaling grows it in place), and how brightly it is picked
	// out right now.
	struct PromptSegment
	{
		Language language = Language::English;
		sf::Text text;
		float highlight = 0.f;   // eased 0..1, 1 = this language is selected/hovered
	};

	void BuildPrompt();
	[[nodiscard]] bool IntroDone() const;
	void SetHovered(Language language);
	void Choose(Language language);
	void Finish();

	Context& context;

	// Background -- identical ambient to MenuShell.
	sf::Sprite backgroundSprite;
	UI::MenuAurora aurora;
	UI::MenuBackdrop backdrop;
	UI::MenuSparks sparks;

	std::vector<IntroGlyph> introGlyphs;
	float introElapsed = 0.f;
	float fallDuration = 0.35f;
	float fallStagger = 0.01f;
	float introTotalDuration = 0.f;

	std::array<PromptSegment, LanguageCount> segments;
	std::vector<sf::Text> dividers;   // static "  ·  " marks between phrases
	Language hoveredLanguage = Language::English;

	UI::MenuButtonColumn column;

	float fade = 1.f;   // 1 = fully black, 0 = clear
	bool leaving = false;
};
