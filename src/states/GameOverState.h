#pragma once

#include <optional>
#include <vector>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/System/String.hpp>

#include "../core/State.h"
#include "../rendering/NeonGlow.h"
#include "../ui/Celebration.h"
#include "../ui/ConfirmDialog.h"
#include "../ui/MenuLabel.h"
#include "../ui/NineSliceFrame.h"

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

	[[nodiscard]] bool ShowsCursor() const override { return true; }

private:
	enum class Focus { Save, PlayAgain, MainMenu };
	enum class Leaving { No, PlayAgain, MainMenu };

	struct Line
	{
		sf::Text text;
		sf::Color base;
	};

	static constexpr std::size_t MaxNameLength = 14;

	void BuildContent();
	void Activate();
	[[nodiscard]] float FlickerBrightness() const;
	void HandleTextInput(char32_t character);
	[[nodiscard]] bool NameEntered() const;
	[[nodiscard]] sf::String TrimmedName() const;
	[[nodiscard]] bool CanSave() const;
	// True whenever leaving would silently drop a qualifying record -- unlike
	// CanSave(), this doesn't require a name to have been typed yet.
	[[nodiscard]] bool HasUnsavedRecord() const { return isRecord && !recordSaved; }
	void SaveRecord();
	void BeginLeave();
	void DrawButton(sf::RenderTarget& target, UI::MenuLabel& label, sf::Vector2f centre,
		sf::Color hue, bool selected, float alpha);

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
	float saveCentreX = 0.f;

	sf::Sprite backdrop;
	UI::NineSliceFrame panel;
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
	bool recordSaved = false;

	float appear = 0.f;
	float headingDrop = 0.f;
	float pressTime = 1000.f;
	float savePulse = 0.f;
	float cursorTime = 0.f;

	// "GAME OVER" idle: a dying-neon flicker with a rare chromatic glitch.
	float idleTime = 0.f;
	float glitchCooldown = 2.5f;
	float glitchTime = 0.f;
	float glitchDuration = 0.f;
	bool glitchActive = false;

	Leaving leaving = Leaving::No;
	float leaveTimer = 0.f;
};
