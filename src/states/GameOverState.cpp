#include "GameOverState.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <cstdint>
#include <string>

#include <SFML/Graphics/BlendMode.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Mouse.hpp>

#include "../audio/AudioPlayer.h"
#include "../audio/MusicPlayer.h"
#include "../core/Context.h"
#include "../display/DisplaySettings.h"
#include "../input/MenuInput.h"
#include "../localization/LocalizationManager.h"
#include "../localization/TextKeys.h"
#include "../resources/Assets.h"
#include "../statistics/HighScoreManager.h"
#include "../utils/Easing.h"
#include "../utils/Random.h"
#include "GameplayState.h"
#include "MenuShellState.h"

namespace
{
	using Display::VirtualSize;

	constexpr float CenterX = 960.f;

	constexpr float ScreenCenterY = 540.f;
	constexpr float PanelX = 260.f;
	constexpr float PanelW = 1400.f;
	constexpr float PanelHRecord = 840.f;
	constexpr float PanelHPlain = 620.f;
	constexpr sf::Vector2f PanelTargetBorder{ 46.f, 46.f };
	constexpr float ButtonGap = 66.f;   // below the panel

	// Content offsets from the panel's top edge; chosen so the block sits with
	// equal margins top and bottom of the plain panel.
	constexpr float HeadingOffset = 122.f;
	constexpr float ScoreLabelOffset = 240.f;
	constexpr float ScoreValueOffset = 332.f;
	constexpr float StatLabelOffset = 462.f;
	constexpr float StatValueOffset = 528.f;
	constexpr float BadgeOffset = 636.f;

	// The name row: a recessed field with the Save Record button to its right,
	// centered vertically on this offset from the panel top.
	constexpr float NameRowOffset = 742.f;
	constexpr float FieldWidth = 520.f;
	constexpr float FieldHeight = 96.f;
	constexpr float FieldCenterX = 960.f - 170.f;
	constexpr float FieldTextInset = 30.f;

	constexpr float StatSpread = 400.f;

	[[nodiscard]] float PanelHeightFor(bool isRecord) { return isRecord ? PanelHRecord : PanelHPlain; }
	[[nodiscard]] float PanelTopFor(bool isRecord) { return ScreenCenterY - PanelHeightFor(isRecord) * 0.5f; }

	constexpr unsigned int HeadingSize = 128;
	constexpr unsigned int ScoreLabelSize = 44;
	constexpr unsigned int ScoreValueSize = 132;
	constexpr unsigned int StatLabelSize = 40;
	constexpr unsigned int StatValueSize = 68;
	constexpr unsigned int BadgeSize = 56;
	constexpr unsigned int NameSize = 56;
	constexpr unsigned int PromptSize = 34;
	constexpr unsigned int ButtonSize = 44;
	constexpr unsigned int SaveButtonSize = 38;

	constexpr float ButtonSpacing = 290.f;

	// Matches GameplayState's dimmed backdrop at the end of the death beat, so
	// the screen arrives with no visible cut.
	constexpr std::uint8_t SceneDim = 140;

	constexpr float AppearSpeed = 1.f / 0.26f;
	constexpr float HeadingDropSpeed = 1.f / 0.42f;
	constexpr float PressDuration = 0.18f;
	constexpr float PressPunch = 0.12f;
	constexpr float PressFlash = 0.55f;
	constexpr float SelectedScale = 1.05f;
	constexpr float UnselectedAlpha = 0.5f;
	constexpr float ButtonGlowIntensity = 0.5f;

	constexpr float PlayAgainDelay = 0.16f;
	constexpr float MainMenuDelay = 0.24f;

	constexpr float Pi = std::numbers::pi_v<float>;

	// GetFlickerBrightness() -- "GAME OVER" idle neon flicker: a constant faint
	// shimmer, plus two independent periodic dropouts (a full blackout and a
	// dimmer half-power dip) that don't line up, so the failure never repeats.
	constexpr float FlickerShimmerAmount = 0.05f;
	constexpr float FlickerShimmerSpeed = 43.f;
	constexpr float FlickerDropoutPeriod = 2.7f;
	constexpr float FlickerDropoutWindow = 0.05f;
	constexpr float FlickerDropoutBrightness = 0.22f;
	constexpr float FlickerDimPeriod = 4.3f;
	constexpr float FlickerDimPhaseOffset = 1.35f;
	constexpr float FlickerDimWindow = 0.09f;
	constexpr float FlickerDimFactor = 0.45f;

	// Render() -- content fades in only once appear has cleared this floor,
	// over the remaining span, so the panel itself is visibly first.
	constexpr float ContentAlphaRampStart = 0.2f;
	constexpr float ContentAlphaRampSpan = 0.8f;

	// Render() -- how far the heading rises into place as headingDrop settles.
	constexpr float HeadingRiseDistance = 70.f;

	// Render() -- glitch shake: independent sine jitter on each axis, scaled by
	// how far into the glitch pulse we are.
	constexpr float GlitchJitterXSpeed = 190.f;
	constexpr float GlitchJitterXAmount = 7.f;
	constexpr float GlitchJitterYSpeed = 250.f;
	constexpr float GlitchJitterYAmount = 4.f;

	// Render() -- chromatic split: red/cyan copies of the heading pulled apart
	// during a glitch.
	constexpr float ChromaticSplitBase = 6.f;
	constexpr float ChromaticSplitScale = 10.f;
	constexpr float ChromaticSplitAlpha = 0.85f;
	const sf::Color ChromaticRed{ 255, 60, 60 };
	const sf::Color ChromaticCyan{ 60, 200, 255 };

	// Render() -- the heading's own neon bloom box, sized to comfortably clear
	// the widest word in any language.
	constexpr float HeadingGlowBoxWidth = 1280.f;
	constexpr float HeadingGlowBoxHeight = 280.f;
	constexpr float HeadingGlowFlickerWeight = 0.75f;

	// Render() -- the name-field text cursor blinks on for this fraction of
	// each one-second cycle.
	constexpr float CursorBlinkOnFraction = 0.55f;

	constexpr float PromptAlphaFraction = 0.9f;

	// Render() -- Save Record button alpha: full once available, dim once
	// already saved, dimmer still while disabled (no name typed yet).
	constexpr float SavedAlphaFraction = 0.55f;
	constexpr float DisabledSaveAlphaFraction = 0.32f;

	constexpr float SaveFocusedScale = 1.06f;
	constexpr float SavePulseScaleBoost = 0.12f;

	// Constructor/Update() -- glitch timing: how long between glitches, and
	// how long each one lasts. The gap widens once idle (first glitch is
	// sooner, to catch the eye early).
	constexpr float InitialGlitchCooldownMin = 1.6f;
	constexpr float InitialGlitchCooldownMax = 3.4f;
	constexpr float GlitchCooldownMin = 1.9f;
	constexpr float GlitchCooldownMax = 4.6f;
	constexpr float GlitchDurationMin = 0.08f;
	constexpr float GlitchDurationMax = 0.18f;

	// Constructor -- the backdrop is desaturated toward this grey before the
	// scene-dim overlay darkens it further.
	const sf::Color BackdropTint{ 150, 150, 150 };

	// BuildContent() -- shared text outline/tracking for the heading and the
	// summary lines.
	constexpr float HeadingOutlineThickness = 4.f;
	constexpr float LineLetterSpacing = 1.1f;
	constexpr float BadgeLetterSpacing = 1.2f;
	constexpr float BadgeOutlineThickness = 3.f;
	const sf::Color BadgeOutlineColor{ 70, 44, 0 };

	// SaveRecord()/Update() -- how fast the save-button pulse decays per second.
	constexpr float SavePulseDecaySpeed = 2.4f;

	// Activate()'s unsaved-record confirmation chime, pitched down a touch.
	constexpr float UnsavedConfirmPitch = 0.85f;

	const sf::Color HeadingFill{ 250, 236, 233 };
	const sf::Color HeadingOutline{ 120, 20, 20 };
	const sf::Color HeadingGlowTint{ 224, 34, 34 };

	// The record variant swaps the red neon for warm gold.
	const sf::Color HeadingFillGold{ 255, 246, 220 };
	const sf::Color HeadingOutlineGold{ 132, 88, 8 };
	const sf::Color HeadingGlowTintGold{ 255, 196, 74 };
	const sf::Color LabelColor{ 168, 150, 158 };
	const sf::Color ScoreColor{ 252, 244, 240 };
	const sf::Color StatValueColor{ 224, 228, 236 };
	const sf::Color BadgeColor{ 255, 208, 120 };
	const sf::Color NameColor{ 252, 244, 240 };
	const sf::Color PromptColor{ 150, 148, 160 };
	const sf::Color PlayHue{ 90, 205, 130 };
	const sf::Color MenuHue{ 228, 232, 240 };
	const sf::Color SaveHue{ 255, 208, 120 };
	const sf::Color SavedHue{ 120, 210, 140 };

	// The recessed name field: a near-black well with a dark top/left lip and a
	// faint warm highlight along the bottom/right, so it reads as pushed in.
	const sf::Color FieldFill{ 6, 7, 11 };
	const sf::Color FieldShadow{ 0, 0, 0 };
	const sf::Color FieldHighlight{ 128, 112, 74 };

	[[nodiscard]] sf::FloatRect NameFieldBounds(float panelTop)
	{
		return { { FieldCenterX - FieldWidth * 0.5f, panelTop + NameRowOffset - FieldHeight * 0.5f },
			{ FieldWidth, FieldHeight } };
	}

	[[nodiscard]] sf::FloatRect PanelBoundsFor(bool isRecord)
	{
		return { { PanelX, PanelTopFor(isRecord) }, { PanelW, PanelHeightFor(isRecord) } };
	}

	[[nodiscard]] const sf::Texture& FrameTextureFor(Context& context, bool isRecord)
	{
		return context.textures.Get(isRecord
			? Assets::TextureID::UiFrameWarning
			: Assets::TextureID::UiFrameRed);
	}

	[[nodiscard]] std::uint8_t ToAlpha(float value)
	{
		return static_cast<std::uint8_t>(std::clamp(value, 0.f, 1.f) * 255.f);
	}

	[[nodiscard]] sf::Color Faded(sf::Color color, float alpha)
	{
		return sf::Color(color.r, color.g, color.b, ToAlpha(alpha));
	}

	void PlaceCentered(sf::Text& text, sf::Vector2f center)
	{
		const sf::FloatRect bounds = text.getLocalBounds();
		text.setOrigin({ bounds.position.x + bounds.size.x * 0.5f, bounds.position.y + bounds.size.y * 0.5f });
		text.setPosition(center);
	}

	void PlaceLeft(sf::Text& text, sf::Vector2f leftMiddle)
	{
		const sf::FloatRect bounds = text.getLocalBounds();
		text.setOrigin({ bounds.position.x, bounds.position.y + bounds.size.y * 0.5f });
		text.setPosition(leftMiddle);
	}

	constexpr float FieldOutlineThickness = 2.f;
	constexpr float FieldLipThickness = 4.f;
	constexpr float FieldEdgeThickness = 2.f;
	constexpr float FieldLipAlphaFraction = 0.7f;
	constexpr float FieldEdgeAlphaFraction = 0.5f;

	void DrawRecessedField(sf::RenderTarget& target, const sf::FloatRect& bounds, float alpha)
	{
		sf::RectangleShape well(bounds.size);
		well.setPosition(bounds.position);
		well.setFillColor(Faded(FieldFill, alpha));
		well.setOutlineThickness(-FieldOutlineThickness);
		well.setOutlineColor(Faded(FieldShadow, alpha));
		target.draw(well);

		sf::RectangleShape topLip({ bounds.size.x, FieldLipThickness });
		topLip.setPosition(bounds.position);
		topLip.setFillColor(Faded(FieldShadow, alpha * FieldLipAlphaFraction));
		target.draw(topLip);

		sf::RectangleShape bottomEdge({ bounds.size.x, FieldEdgeThickness });
		bottomEdge.setPosition({ bounds.position.x, bounds.position.y + bounds.size.y - FieldEdgeThickness });
		bottomEdge.setFillColor(Faded(FieldHighlight, alpha * FieldEdgeAlphaFraction));
		target.draw(bottomEdge);
	}

	[[nodiscard]] std::string FormatTime(float seconds)
	{
		const int total = std::max(0, static_cast<int>(seconds));
		const int minutes = total / 60;
		const int rest = total % 60;
		return std::to_string(minutes) + ":" + (rest < 10 ? "0" : "") + std::to_string(rest);
	}

	using Easing::EaseOutBack;
	using Easing::SmoothStep;
}

GameOverState::GameOverState(Context& context, int finalScore, int finalLines, int finalLevel, float finalSeconds)
	: State(context.stateMachine)
	, context(context)
	, finalScore(finalScore)
	, finalLines(finalLines)
	, finalLevel(finalLevel)
	, finalSeconds(finalSeconds)
	, isRecord(context.highScores.IsHighScore(finalScore))
	, panelTop(PanelTopFor(isRecord))
	, buttonY(panelTop + PanelHeightFor(isRecord) + ButtonGap)
	, backdrop(context.textures.Get(Assets::TextureID::GameplayBackground))
	, panel(FrameTextureFor(context, isRecord), PanelBoundsFor(isRecord),
		MenuFrameSourceBorder, PanelTargetBorder)
	, heading(context.fonts.Get(Assets::FontID::Main), "", HeadingSize)
	, recordBadge(context.fonts.Get(Assets::FontID::Main), "", BadgeSize)
	, nameField(context.fonts.Get(Assets::FontID::Main), "", NameSize)
	, namePrompt(context.fonts.Get(Assets::FontID::Main), "", PromptSize)
	, playAgainLabel(context.fonts.Get(Assets::FontID::Menu), ButtonSize)
	, mainMenuLabel(context.fonts.Get(Assets::FontID::Menu), ButtonSize)
	, saveLabel(context.fonts.Get(Assets::FontID::Menu), SaveButtonSize)
	, buttonGlow(context.shaders.Get(Assets::ShaderID::NeonDilate), context.shaders.Get(Assets::ShaderID::NeonBlur))
	, headingGlow(context.shaders.Get(Assets::ShaderID::NeonDilate), context.shaders.Get(Assets::ShaderID::NeonBlur))
	, leaveDialog(context.fonts.Get(Assets::FontID::Main), context.fonts.Get(Assets::FontID::Menu),
		context.textures.Get(Assets::TextureID::UiFrameWarning),
		context.shaders.Get(Assets::ShaderID::NeonDilate), context.shaders.Get(Assets::ShaderID::NeonBlur),
		context.audioPlayer)
{
	backdrop.setColor(BackdropTint);
	glitchCooldown = Random::Float(InitialGlitchCooldownMin, InitialGlitchCooldownMax);

	if (isRecord)
	{
		const sf::FloatRect bounds = PanelBoundsFor(true);
		celebration.SetCorners({ {
			bounds.position,
			{ bounds.position.x + bounds.size.x, bounds.position.y },
			{ bounds.position.x, bounds.position.y + bounds.size.y },
			bounds.position + bounds.size,
		} });

		recordRank = 1;
		for (const HighScoreEntry& entry : context.highScores.GetRecords())
		{
			if (finalScore < entry.score)
			{
				recordRank++;
			}
		}
		recordRank = std::min<int>(recordRank, static_cast<int>(HighScoreManager::MaxRecords));
	}

	playAgainLabel.SetText(context.localization.GetText(TextKey::GameOver::PlayAgain));
	mainMenuLabel.SetText(context.localization.GetText(TextKey::GameOver::MainMenu));
	saveLabel.SetText(context.localization.GetText(TextKey::GameOver::SaveRecord));

	// "Save Record" runs much longer in some languages than in English (which
	// is why saveCenterX used to be a fixed offset) -- anchor it from the
	// field's actual right edge instead, so it can never creep onto the field.
	constexpr float SaveButtonGap = 40.f;
	const sf::FloatRect nameFieldBounds = NameFieldBounds(panelTop);
	saveCenterX = nameFieldBounds.position.x + nameFieldBounds.size.x + SaveButtonGap + saveLabel.GetInkSize().x * 0.5f;

	BuildContent();

	// The gameplay music keeps playing underneath, just muffled -- same duck
	// as the pause menu -- with the game-over sting a one-shot sound on top.
	context.musicPlayer.SetDucked(true);
	context.audioPlayer.Play(Assets::SoundID::GameOver);
}

void GameOverState::BuildContent()
{
	const LocalizationManager& text = context.localization;
	const sf::Font& font = context.fonts.Get(Assets::FontID::Main);

	heading.setString(text.GetText(TextKey::GameOver::Title));
	heading.setOutlineThickness(HeadingOutlineThickness);
	heading.setLetterSpacing(LineLetterSpacing);

	lines.clear();

	const auto add = [&](const sf::String& string, unsigned int size, sf::Vector2f center, sf::Color color)
	{
		sf::Text line(font, string, size);
		line.setLetterSpacing(LineLetterSpacing);
		PlaceCentered(line, center);
		lines.push_back({ std::move(line), color });
	};

	const float scoreLabelY = panelTop + ScoreLabelOffset;
	const float scoreValueY = panelTop + ScoreValueOffset;
	const float statLabelY = panelTop + StatLabelOffset;
	const float statValueY = panelTop + StatValueOffset;

	add(text.GetText(TextKey::GameOver::Score), ScoreLabelSize, { CenterX, scoreLabelY }, LabelColor);
	add(std::to_string(finalScore), ScoreValueSize, { CenterX, scoreValueY }, ScoreColor);

	const float leftX = CenterX - StatSpread;
	add(text.GetText(TextKey::GameOver::Lines), StatLabelSize, { leftX, statLabelY }, LabelColor);
	add(std::to_string(finalLines), StatValueSize, { leftX, statValueY }, StatValueColor);
	add(text.GetText(TextKey::GameOver::Level), StatLabelSize, { CenterX, statLabelY }, LabelColor);
	add(std::to_string(finalLevel), StatValueSize, { CenterX, statValueY }, StatValueColor);
	add(text.GetText(TextKey::GameOver::Time), StatLabelSize, { CenterX + StatSpread, statLabelY }, LabelColor);
	add(FormatTime(finalSeconds), StatValueSize, { CenterX + StatSpread, statValueY }, StatValueColor);

	if (isRecord)
	{
		recordBadge.setString(text.GetText(TextKey::GameOver::NewRecord) + sf::String("   #" + std::to_string(recordRank)));
		recordBadge.setLetterSpacing(BadgeLetterSpacing);
		recordBadge.setOutlineThickness(BadgeOutlineThickness);
		recordBadge.setOutlineColor(BadgeOutlineColor);
		PlaceCentered(recordBadge, { CenterX, panelTop + BadgeOffset });

		namePrompt.setString(text.GetText(TextKey::GameOver::EnterName));
	}
}

float GameOverState::GetFlickerBrightness() const
{
	if (headingDrop < 1.f)
	{
		return 1.f;
	}

	float brightness = 1.f - FlickerShimmerAmount * std::abs(std::sin(idleTime * FlickerShimmerSpeed));
	if (std::fmod(idleTime, FlickerDropoutPeriod) < FlickerDropoutWindow)
	{
		brightness = FlickerDropoutBrightness;   // a full dropout, like a failing tube
	}
	if (std::fmod(idleTime + FlickerDimPhaseOffset, FlickerDimPeriod) < FlickerDimWindow)
	{
		brightness *= FlickerDimFactor;
	}
	return brightness;
}

bool GameOverState::IsNameEntered() const
{
	for (const char32_t character : playerName)
	{
		if (character != U' ')
		{
			return true;
		}
	}
	return false;
}

sf::String GameOverState::GetTrimmedName() const
{
	std::size_t start = 0;
	std::size_t end = playerName.getSize();
	while (start < end && playerName[start] == U' ')
	{
		start++;
	}
	while (end > start && playerName[end - 1] == U' ')
	{
		end--;
	}
	return playerName.substring(start, end - start);
}

bool GameOverState::IsSaveAllowed() const
{
	return isRecord && !hasSavedRecord && IsNameEntered();
}

void GameOverState::SaveRecord()
{
	if (!IsSaveAllowed())
	{
		return;
	}

	context.highScores.AddRecord({ GetTrimmedName(), finalScore, finalLines, finalLevel });
	context.highScores.Save();

	hasSavedRecord = true;
	savePulse = 1.f;
	saveLabel.SetText(context.localization.GetText(TextKey::GameOver::Saved));
	context.audioPlayer.Play(Assets::SoundID::NextLevel);
}

void GameOverState::HandleTextInput(char32_t character)
{
	if (character == U'\b')
	{
		if (!playerName.isEmpty())
		{
			playerName.erase(playerName.getSize() - 1, 1);
		}
	}
	else if (character >= 32 && character != 127)
	{
		if (playerName.getSize() >= MaxNameLength || (character == U' ' && playerName.isEmpty()))
		{
			return;
		}
		playerName += character;
	}
}

void GameOverState::Activate()
{
	if (leaving != Leaving::No || leaveDialog.IsOpen())
	{
		return;
	}

	// A qualifying, not-yet-saved record: make the player confirm they mean to
	// drop it, whether or not they got as far as typing a name for it.
	if (HasUnsavedRecord())
	{
		leaveDialog.Show(context.localization.GetText(TextKey::GameOver::UnsavedRecord),
			context.localization.GetText(TextKey::Common::Yes),
			context.localization.GetText(TextKey::Common::No));
		context.audioPlayer.Play(Assets::SoundID::MenuItemPressed, UnsavedConfirmPitch);
		return;
	}

	BeginLeave();
}

void GameOverState::BeginLeave()
{
	pressTime = 0.f;
	context.audioPlayer.Play(Assets::SoundID::MenuItemPressed);
	leaving = focus == Focus::PlayAgain ? Leaving::PlayAgain : Leaving::MainMenu;
	leaveTimer = 0.f;
}

bool GameOverState::IsCursorVisible() const
{
	return true;
}

bool GameOverState::HasUnsavedRecord() const
{
	return isRecord && !hasSavedRecord;
}

void GameOverState::HandleEvent(const sf::Event& event)
{
	if (leaving != Leaving::No)
	{
		return;
	}

	if (leaveDialog.IsOpen())
	{
		leaveDialog.Navigate(MenuInput::ResolveAction(event, context.gamepad));

		if (const auto* moved = event.getIf<sf::Event::MouseMoved>())
		{
			leaveDialog.PointerMoved(context.window.mapPixelToCoords(moved->position));
		}
		else if (const auto* pressed = event.getIf<sf::Event::MouseButtonPressed>())
		{
			if (pressed->button == sf::Mouse::Button::Left)
			{
				leaveDialog.PointerPressed(context.window.mapPixelToCoords(pressed->position));
			}
		}

		return;
	}

	if (isRecord)
	{
		if (const auto* entered = event.getIf<sf::Event::TextEntered>())
		{
			HandleTextInput(entered->unicode);
			return;
		}
	}

	const auto selectSound = [this] { context.audioPlayer.Restart(Assets::SoundID::MenuItemSelected); };

	switch (MenuInput::ResolveAction(event, context.gamepad))
	{
	case MenuInput::Action::Up:
		// Save Record sits above the bottom row; reach it by going up.
		if (IsSaveAllowed() && focus != Focus::Save)
		{
			focus = Focus::Save;
			selectSound();
		}
		return;
	case MenuInput::Action::Down:
		if (focus == Focus::Save)
		{
			focus = Focus::PlayAgain;
			selectSound();
		}
		return;
	case MenuInput::Action::Left:
		focus = Focus::PlayAgain;
		selectSound();
		return;
	case MenuInput::Action::Right:
		focus = Focus::MainMenu;
		selectSound();
		return;
	case MenuInput::Action::Confirm:
		if (focus == Focus::Save)
		{
			SaveRecord();
			return;
		}
		Activate();
		return;
	case MenuInput::Action::Back:
		focus = Focus::MainMenu;
		Activate();
		return;
	default:
		break;
	}

	const auto hit = [&](sf::Vector2f point, UI::MenuLabel& label, sf::Vector2f center) -> bool
	{
		return label.GetBounds(center, 1.f).contains(point);
	};

	if (const auto* moved = event.getIf<sf::Event::MouseMoved>())
	{
		const sf::Vector2f point = context.window.mapPixelToCoords(moved->position);
		if (IsSaveAllowed() && hit(point, saveLabel, { saveCenterX, panelTop + NameRowOffset }))
		{
			focus = Focus::Save;
		}
		else if (hit(point, playAgainLabel, { CenterX - ButtonSpacing, buttonY }))
		{
			focus = Focus::PlayAgain;
		}
		else if (hit(point, mainMenuLabel, { CenterX + ButtonSpacing, buttonY }))
		{
			focus = Focus::MainMenu;
		}
	}
	else if (const auto* pressed = event.getIf<sf::Event::MouseButtonPressed>())
	{
		if (pressed->button != sf::Mouse::Button::Left)
		{
			return;
		}
		const sf::Vector2f point = context.window.mapPixelToCoords(pressed->position);
		if (IsSaveAllowed() && hit(point, saveLabel, { saveCenterX, panelTop + NameRowOffset }))
		{
			SaveRecord();
			return;
		}
		if (hit(point, playAgainLabel, { CenterX - ButtonSpacing, buttonY }))
		{
			focus = Focus::PlayAgain;
			Activate();
		}
		else if (hit(point, mainMenuLabel, { CenterX + ButtonSpacing, buttonY }))
		{
			focus = Focus::MainMenu;
			Activate();
		}
	}
}

void GameOverState::Update(float deltaTime)
{
	appear = std::min(1.f, appear + deltaTime * AppearSpeed);
	headingDrop = std::min(1.f, headingDrop + deltaTime * HeadingDropSpeed);
	pressTime += deltaTime;
	cursorTime += deltaTime;
	savePulse = std::max(0.f, savePulse - deltaTime * SavePulseDecaySpeed);

	leaveDialog.Update(deltaTime);
	if (const std::optional<bool> answer = leaveDialog.TakeResult(); answer.has_value() && *answer)
	{
		BeginLeave();
	}

	if (focus == Focus::Save && !IsSaveAllowed())
	{
		focus = Focus::PlayAgain;
	}

	const bool interactive = leaving == Leaving::No && !leaveDialog.IsOpen();
	playAgainLabel.SetWaveEnabled(interactive && focus == Focus::PlayAgain);
	mainMenuLabel.SetWaveEnabled(interactive && focus == Focus::MainMenu);
	saveLabel.SetWaveEnabled(interactive && focus == Focus::Save);
	playAgainLabel.Update(deltaTime);
	mainMenuLabel.Update(deltaTime);
	saveLabel.Update(deltaTime);
	buttonGlow.Update(deltaTime);
	headingGlow.Update(deltaTime);

	if (isRecord)
	{
		celebration.Update(deltaTime);
	}

	if (headingDrop >= 1.f)
	{
		idleTime += deltaTime;

		if (isGlitchActive)
		{
			glitchTime += deltaTime;
			if (glitchTime >= glitchDuration)
			{
				isGlitchActive = false;
				glitchCooldown = Random::Float(GlitchCooldownMin, GlitchCooldownMax);
			}
		}
		else
		{
			glitchCooldown -= deltaTime;
			if (glitchCooldown <= 0.f)
			{
				isGlitchActive = true;
				glitchTime = 0.f;
				glitchDuration = Random::Float(GlitchDurationMin, GlitchDurationMax);
			}
		}
	}

	if (leaving == Leaving::No)
	{
		return;
	}

	leaveTimer += deltaTime;

	if (leaving == Leaving::PlayAgain && leaveTimer >= PlayAgainDelay)
	{
		RequestChange(std::make_unique<GameplayState>(context));
	}
	else if (leaving == Leaving::MainMenu && leaveTimer >= MainMenuDelay)
	{
		RequestClear();
		RequestPush(std::make_unique<MenuShellState>(context));
	}
}

void GameOverState::DrawButton(sf::RenderTarget& target, UI::MenuLabel& label, sf::Vector2f center,
	sf::Color hue, bool isSelected, float alpha)
{
	const float press = (isSelected && pressTime < PressDuration)
		? std::sin(std::clamp(1.f - pressTime / PressDuration, 0.f, 1.f) * Pi)
		: 0.f;
	const float scale = (isSelected ? SelectedScale : 1.f) + PressPunch * press;
	const float drawAlpha = alpha * (isSelected ? 1.f : UnselectedAlpha);

	if (isSelected)
	{
		label.DrawGlow(target, buttonGlow, center, scale,
			sf::Color(hue.r, hue.g, hue.b, static_cast<std::uint8_t>(std::clamp(alpha, 0.f, 1.f) * 255.f * ButtonGlowIntensity)));
	}

	label.Draw(target, center, scale, hue, drawAlpha, PressFlash * press);
}

void GameOverState::Render(sf::RenderTarget& target)
{
	target.draw(backdrop);

	sf::RectangleShape dim(VirtualSize);
	dim.setFillColor(sf::Color(0, 0, 0, SceneDim));
	target.draw(dim);

	if (isRecord)
	{
		celebration.RenderFireworks(target);
	}

	const float in = SmoothStep(appear);
	const auto contentAlpha = std::clamp((appear - ContentAlphaRampStart) / ContentAlphaRampSpan, 0.f, 1.f);

	panel.SetColor(sf::Color(255, 255, 255, ToAlpha(in)));
	panel.Draw(target);

	if (isRecord)
	{
		celebration.RenderCornerSparks(target);
	}

	if (contentAlpha > 0.f)
	{
		const float rise = (1.f - EaseOutBack(headingDrop)) * HeadingRiseDistance;
		const float flicker = GetFlickerBrightness();
		const float glitch = isGlitchActive
			? std::sin(std::clamp(glitchTime / std::max(0.01f, glitchDuration), 0.f, 1.f) * Pi)
			: 0.f;

		const sf::Vector2f jitter = glitch > 0.f
			? sf::Vector2f{
				std::sin(glitchTime * GlitchJitterXSpeed) * glitch * GlitchJitterXAmount,
				std::sin(glitchTime * GlitchJitterYSpeed) * glitch * GlitchJitterYAmount }
			: sf::Vector2f{ 0.f, 0.f };
		const sf::Vector2f base{ CenterX + jitter.x, panelTop + HeadingOffset - rise + jitter.y };

		sf::RenderStates additive;
		additive.blendMode = sf::BlendAdd;

		// Chromatic split during a glitch.
		if (glitch > 0.f)
		{
			const float dx = ChromaticSplitBase + glitch * ChromaticSplitScale;
			heading.setOutlineColor(sf::Color(0, 0, 0, 0));
			heading.setFillColor(Faded(ChromaticRed, contentAlpha * ChromaticSplitAlpha));
			PlaceCentered(heading, { base.x + dx, base.y });
			target.draw(heading, additive);
			heading.setFillColor(Faded(ChromaticCyan, contentAlpha * ChromaticSplitAlpha));
			PlaceCentered(heading, { base.x - dx, base.y });
			target.draw(heading, additive);
		}

		const auto lit = [flicker](sf::Color color)
		{
			return sf::Color(
				static_cast<std::uint8_t>(static_cast<float>(color.r) * flicker),
				static_cast<std::uint8_t>(static_cast<float>(color.g) * flicker),
				static_cast<std::uint8_t>(static_cast<float>(color.b) * flicker), color.a);
		};

		const sf::Color headingFill = isRecord ? HeadingFillGold : HeadingFill;
		const sf::Color headingOutline = isRecord ? HeadingOutlineGold : HeadingOutline;
		const sf::Color headingGlowTint = isRecord ? HeadingGlowTintGold : HeadingGlowTint;

		heading.setFillColor(Faded(lit(headingFill), contentAlpha));
		heading.setOutlineColor(Faded(headingOutline, contentAlpha * flicker));
		PlaceCentered(heading, base);

		const sf::FloatRect glowArea{
			{ CenterX - HeadingGlowBoxWidth * 0.5f, panelTop + HeadingOffset - HeadingGlowBoxHeight * 0.5f },
			{ HeadingGlowBoxWidth, HeadingGlowBoxHeight } };
		headingGlow.Draw(target, glowArea,
			[this](sf::RenderTarget& buffer, const sf::RenderStates& states) { buffer.draw(heading, states); },
			Faded(headingGlowTint, contentAlpha * flicker * HeadingGlowFlickerWeight), false);

		target.draw(heading);

		for (Line& line : lines)
		{
			line.text.setFillColor(Faded(line.base, contentAlpha));
			target.draw(line.text);
		}

		if (isRecord)
		{
			recordBadge.setFillColor(Faded(BadgeColor, contentAlpha));
			target.draw(recordBadge);

			const sf::FloatRect field = NameFieldBounds(panelTop);
			const float rowY = field.position.y + field.size.y * 0.5f;
			const float textLeft = field.position.x + FieldTextInset;

			DrawRecessedField(target, field, contentAlpha);

			if (IsNameEntered())
			{
				const bool isCursorShown = std::fmod(cursorTime, 1.f) < CursorBlinkOnFraction;
				nameField.setString(playerName + (isCursorShown ? sf::String("|") : sf::String(" ")));
				PlaceLeft(nameField, { textLeft, rowY });
				nameField.setFillColor(Faded(NameColor, contentAlpha));
				target.draw(nameField);
			}
			else
			{
				namePrompt.setFillColor(Faded(PromptColor, contentAlpha * PromptAlphaFraction));
				PlaceLeft(namePrompt, { textLeft, rowY });
				target.draw(namePrompt);
			}

			const bool isSaveFocused = leaving == Leaving::No && !leaveDialog.IsOpen() && focus == Focus::Save;
			const sf::Color saveHue = hasSavedRecord ? SavedHue : SaveHue;
			const float saveAlpha = contentAlpha
				* (hasSavedRecord ? SavedAlphaFraction : (IsSaveAllowed() ? 1.f : DisabledSaveAlphaFraction));
			const float pulse = savePulse > 0.f ? std::sin(std::clamp(savePulse, 0.f, 1.f) * Pi) : 0.f;
			const float saveScale = (isSaveFocused ? SaveFocusedScale : 1.f) + SavePulseScaleBoost * pulse;

			if (isSaveFocused)
			{
				saveLabel.DrawGlow(target, buttonGlow, { saveCenterX, rowY }, saveScale,
					sf::Color(saveHue.r, saveHue.g, saveHue.b, ToAlpha(contentAlpha * ButtonGlowIntensity)));
			}
			saveLabel.Draw(target, { saveCenterX, rowY }, saveScale, saveHue, saveAlpha, PressFlash * pulse);
		}
	}

	const float buttonAlpha = contentAlpha;
	DrawButton(target, playAgainLabel, { CenterX - ButtonSpacing, buttonY }, PlayHue,
		leaving == Leaving::No && focus == Focus::PlayAgain, buttonAlpha);
	DrawButton(target, mainMenuLabel, { CenterX + ButtonSpacing, buttonY }, MenuHue,
		leaving == Leaving::No && focus == Focus::MainMenu, buttonAlpha);

	if (leaving == Leaving::MainMenu)
	{
		sf::RectangleShape fade(VirtualSize);
		fade.setFillColor(sf::Color(0, 0, 0, ToAlpha(leaveTimer / MainMenuDelay)));
		target.draw(fade);
	}

	leaveDialog.Render(target);
}
