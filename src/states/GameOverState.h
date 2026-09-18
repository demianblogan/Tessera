#pragma once

#include <optional>
#include <vector>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/System/String.hpp>

#include "../core/State.h"
#include "../primitives/NeonGlow.h"
#include "../ui/Celebration.h"
#include "../ui/ConfirmDialog.h"
#include "../ui/MenuLabel.h"
#include "../primitives/NineSliceFrame.h"

struct Context;

namespace sf
{
	class Event;
	class RenderTarget;
}

// The game-over screen: a framed panel over the darkened, crumbled board with
// the run summary (score, lines, level, time), a "NEW BEST" flourish and a name
// field when the run made the top ten, and Play Again / Main Menu below it.
class GameOverState final : public State
{
public:
	GameOverState(Context& context, int finalScore, int finalLines, int finalLevel, float finalSeconds);

	void HandleEvent(const sf::Event& event) override;
	void Update(float deltaTime) override;
	void Render(sf::RenderTarget& target) override;

	[[nodiscard]] bool IsCursorVisible() const override;

private:
	enum class Focus { Save, PlayAgain, MainMenu };
	enum class Leaving { No, PlayAgain, MainMenu };

	struct Line
	{
		sf::Text text;
		sf::Color base;
	};

	void BuildContent();
	void Activate();
	[[nodiscard]] float GetFlickerBrightness() const;
	void HandleTextInput(char32_t character);
	[[nodiscard]] bool IsNameEntered() const;
	[[nodiscard]] sf::String GetTrimmedName() const;
	[[nodiscard]] bool IsSaveAllowed() const;
	// True whenever leaving would silently drop a qualifying record -- unlike
	// IsSaveAllowed(), this doesn't require a name to have been typed yet.
	[[nodiscard]] bool HasUnsavedRecord() const;
	void SaveRecord();
	void BeginLeave();
	void DrawButton(sf::RenderTarget& target, UI::MenuLabel& label, sf::Vector2f center,
		sf::Color hue, bool isSelected, float alpha);

	static constexpr std::size_t MaxNameLength = 14;

	Context& context;

	const int finalScore;
	const int finalLines;
	const int finalLevel;
	const float finalSeconds;
	const bool isRecord;
	int recordRank = 0;
	const float panelTop;
	const float buttonY;
	// Recomputed once saveLabel has its (localized) text, from the field's
	// actual right edge plus a fixed gap -- "Save Record" runs far longer in
	// some languages than in English and would otherwise overlap the field.
	float saveCenterX = 0.f;

	sf::Sprite backdrop;
	NineSliceFrame panel;
	sf::Text heading;
	std::vector<Line> lines;
	sf::Text recordBadge;
	sf::Text nameField;
	sf::Text namePrompt;

	UI::MenuLabel playAgainLabel;
	UI::MenuLabel mainMenuLabel;
	UI::MenuLabel saveLabel;
	NeonGlow buttonGlow;
	NeonGlow headingGlow;
	UI::Celebration celebration;
	UI::ConfirmDialog leaveDialog;
	Focus focus = Focus::PlayAgain;

	sf::String playerName;
	bool hasSavedRecord = false;

	float appear = 0.f;
	float headingDrop = 0.f;

	// Seconds since a button was activated; kept large until then, which
	// DrawButton() reads as "no press flash in progress".
	static constexpr float NoPressSentinel = 1000.f;
	float pressTime = NoPressSentinel;

	float savePulse = 0.f;
	float cursorTime = 0.f;

	// "GAME OVER" idle: a dying-neon flicker with a rare chromatic glitch.
	float idleTime = 0.f;
	float glitchCooldown = 2.5f;
	float glitchTime = 0.f;
	float glitchDuration = 0.f;
	bool isGlitchActive = false;

	Leaving leaving = Leaving::No;
	float leaveTimer = 0.f;
};
