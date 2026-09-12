#include "GameOverState.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>

#include <SFML/Audio/Music.hpp>
#include <SFML/Graphics/BlendMode.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Mouse.hpp>

#include "../audio/AudioPlayer.h"
#include "../core/Context.h"
#include "../input/MenuInput.h"
#include "../localization/LocalizationManager.h"
#include "../localization/TextKeys.h"
#include "../resources/Assets.h"
#include "../statistics/HighScoreManager.h"
#include "../ui/Easing.h"
#include "../utils/Random.h"
#include "GameplayState.h"
#include "MenuShell.h"

namespace
{
	constexpr sf::Vector2f Screen{ 1920.f, 1080.f };
	constexpr float CentreX = 960.f;

	constexpr float ScreenCentreY = 540.f;
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
	// centred vertically on this offset from the panel top.
	constexpr float NameRowOffset = 742.f;
	constexpr float FieldWidth = 520.f;
	constexpr float FieldHeight = 96.f;
	constexpr float FieldCentreX = 960.f - 170.f;
	constexpr float FieldTextInset = 30.f;

	constexpr float StatSpread = 400.f;

	[[nodiscard]] float PanelHeightFor(bool record) { return record ? PanelHRecord : PanelHPlain; }
	[[nodiscard]] float PanelTopFor(bool record) { return ScreenCentreY - PanelHeightFor(record) * 0.5f; }

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

	constexpr float Pi = 3.14159265f;

	const sf::Color HeadingFill{ 250, 236, 233 };
	const sf::Color HeadingOutline{ 120, 20, 20 };
	const sf::Color HeadingGlowTint{ 224, 34, 34 };

	// The record variant swaps the red neon for warm gold.
	const sf::Color HeadingFillGold{ 255, 246, 220 };
	const sf::Color HeadingOutlineGold{ 132, 88, 8 };
	const sf::Color HeadingGlowTintGold{ 255, 196, 74 };
	const sf::Color LabelColour{ 168, 150, 158 };
	const sf::Color ScoreColour{ 252, 244, 240 };
	const sf::Color StatValueColour{ 224, 228, 236 };
	const sf::Color BadgeColour{ 255, 208, 120 };
	const sf::Color NameColour{ 252, 244, 240 };
	const sf::Color PromptColour{ 150, 148, 160 };
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
		return { { FieldCentreX - FieldWidth * 0.5f, panelTop + NameRowOffset - FieldHeight * 0.5f },
			{ FieldWidth, FieldHeight } };
	}

	[[nodiscard]] sf::FloatRect PanelBoundsFor(bool record)
	{
		return { { PanelX, PanelTopFor(record) }, { PanelW, PanelHeightFor(record) } };
	}

	[[nodiscard]] const sf::Texture& FrameTextureFor(Context& context, bool record)
	{
		return context.textures.Get(record
			? Assets::TextureID::UiFrameWarning
			: Assets::TextureID::UiFrameRed);
	}

	[[nodiscard]] std::uint8_t ToAlpha(float value)
	{
		return static_cast<std::uint8_t>(std::clamp(value, 0.f, 1.f) * 255.f);
	}

	[[nodiscard]] sf::Color Faded(sf::Color colour, float alpha)
	{
		return sf::Color(colour.r, colour.g, colour.b, ToAlpha(alpha));
	}

	void PlaceCentred(sf::Text& text, sf::Vector2f centre)
	{
		const sf::FloatRect bounds = text.getLocalBounds();
		text.setOrigin({ bounds.position.x + bounds.size.x * 0.5f, bounds.position.y + bounds.size.y * 0.5f });
		text.setPosition(centre);
	}

	void PlaceLeft(sf::Text& text, sf::Vector2f leftMiddle)
	{
		const sf::FloatRect bounds = text.getLocalBounds();
		text.setOrigin({ bounds.position.x, bounds.position.y + bounds.size.y * 0.5f });
		text.setPosition(leftMiddle);
	}

	void DrawRecessedField(sf::RenderTarget& target, const sf::FloatRect& bounds, float alpha)
	{
		sf::RectangleShape well(bounds.size);
		well.setPosition(bounds.position);
		well.setFillColor(Faded(FieldFill, alpha));
		well.setOutlineThickness(-2.f);
		well.setOutlineColor(Faded(FieldShadow, alpha));
		target.draw(well);

		sf::RectangleShape topLip({ bounds.size.x, 4.f });
		topLip.setPosition(bounds.position);
		topLip.setFillColor(Faded(FieldShadow, alpha * 0.7f));
		target.draw(topLip);

		sf::RectangleShape bottomEdge({ bounds.size.x, 2.f });
		bottomEdge.setPosition({ bounds.position.x, bounds.position.y + bounds.size.y - 2.f });
		bottomEdge.setFillColor(Faded(FieldHighlight, alpha * 0.5f));
		target.draw(bottomEdge);
	}

	[[nodiscard]] std::string FormatTime(float seconds)
	{
		const int total = std::max(0, static_cast<int>(seconds));
		const int minutes = total / 60;
		const int rest = total % 60;
		return std::to_string(minutes) + ":" + (rest < 10 ? "0" : "") + std::to_string(rest);
	}

	using UI::Easing::EaseOutBack;
	using UI::Easing::SmoothStep;
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
		UI::MenuFrameSourceBorder, PanelTargetBorder)
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
	backdrop.setColor(sf::Color(150, 150, 150));
	glitchCooldown = Random::Float(1.6f, 3.4f);

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
		recordRank = std::min<int>(recordRank, static_cast<int>(HighScoreManager::MAX_RECORDS));
	}

	playAgainLabel.SetText(context.localization.GetText(TextKey::GameOver::PlayAgain));
	mainMenuLabel.SetText(context.localization.GetText(TextKey::GameOver::MainMenu));
	saveLabel.SetText(context.localization.GetText(TextKey::GameOver::SaveRecord));

	// "Save Record" runs much longer in some languages than in English (which
	// is why saveCentreX used to be a fixed offset) -- anchor it from the
	// field's actual right edge instead, so it can never creep onto the field.
	constexpr float SaveButtonGap = 40.f;
	const sf::FloatRect nameFieldBounds = NameFieldBounds(panelTop);
	saveCentreX = nameFieldBounds.position.x + nameFieldBounds.size.x + SaveButtonGap + saveLabel.InkSize().x * 0.5f;

	BuildContent();

	sf::Music& music = context.music.Get(Assets::MusicID::GameOver);
	if (music.getStatus() != sf::Music::Status::Playing)
	{
		music.play();
	}
}

void GameOverState::BuildContent()
{
	const LocalizationManager& text = context.localization;
	const sf::Font& font = context.fonts.Get(Assets::FontID::Main);

	heading.setString(text.GetText(TextKey::GameOver::Title));
	heading.setOutlineThickness(4.f);
	heading.setLetterSpacing(1.1f);

	lines.clear();

	const auto add = [&](const sf::String& string, unsigned int size, sf::Vector2f centre, sf::Color colour)
	{
		sf::Text line(font, string, size);
		line.setLetterSpacing(1.1f);
		PlaceCentred(line, centre);
		lines.push_back({ std::move(line), colour });
	};

	const float scoreLabelY = panelTop + ScoreLabelOffset;
	const float scoreValueY = panelTop + ScoreValueOffset;
	const float statLabelY = panelTop + StatLabelOffset;
	const float statValueY = panelTop + StatValueOffset;

	add(text.GetText(TextKey::GameOver::Score), ScoreLabelSize, { CentreX, scoreLabelY }, LabelColour);
	add(std::to_string(finalScore), ScoreValueSize, { CentreX, scoreValueY }, ScoreColour);

	const float leftX = CentreX - StatSpread;
	add(text.GetText(TextKey::GameOver::Lines), StatLabelSize, { leftX, statLabelY }, LabelColour);
	add(std::to_string(finalLines), StatValueSize, { leftX, statValueY }, StatValueColour);
	add(text.GetText(TextKey::GameOver::Level), StatLabelSize, { CentreX, statLabelY }, LabelColour);
	add(std::to_string(finalLevel), StatValueSize, { CentreX, statValueY }, StatValueColour);
	add(text.GetText(TextKey::GameOver::Time), StatLabelSize, { CentreX + StatSpread, statLabelY }, LabelColour);
	add(FormatTime(finalSeconds), StatValueSize, { CentreX + StatSpread, statValueY }, StatValueColour);

	if (isRecord)
	{
		recordBadge.setString(text.GetText(TextKey::GameOver::NewRecord) + sf::String("   #" + std::to_string(recordRank)));
		recordBadge.setLetterSpacing(1.2f);
		recordBadge.setOutlineThickness(3.f);
		recordBadge.setOutlineColor(sf::Color(70, 44, 0));
		PlaceCentred(recordBadge, { CentreX, panelTop + BadgeOffset });

		namePrompt.setString(text.GetText(TextKey::GameOver::EnterName));
	}
}

float GameOverState::FlickerBrightness() const
{
	if (headingDrop < 1.f)
	{
		return 1.f;
	}

	float brightness = 1.f - 0.05f * std::abs(std::sin(idleTime * 43.f));
	if (std::fmod(idleTime, 2.7f) < 0.05f)
	{
		brightness = 0.22f;   // a full dropout, like a failing tube
	}
	if (std::fmod(idleTime + 1.35f, 4.3f) < 0.09f)
	{
		brightness *= 0.45f;
	}
	return brightness;
}

bool GameOverState::NameEntered() const
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

sf::String GameOverState::TrimmedName() const
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

bool GameOverState::CanSave() const
{
	return isRecord && !recordSaved && NameEntered();
}

void GameOverState::SaveRecord()
{
	if (!CanSave())
	{
		return;
	}

	context.highScores.AddRecord({ TrimmedName(), finalScore, finalLines, finalLevel });
	context.highScores.Save();

	recordSaved = true;
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
		context.audioPlayer.Play(Assets::SoundID::MenuItemPressed, 0.85f);
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

void GameOverState::HandleEvent(const sf::Event& event)
{
	if (leaving != Leaving::No)
	{
		return;
	}

	if (leaveDialog.IsOpen())
	{
		leaveDialog.Navigate(MenuInput::Resolve(event, context.gamepad));

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

	switch (MenuInput::Resolve(event, context.gamepad))
	{
	case MenuInput::Action::Up:
		// Save Record sits above the bottom row; reach it by going up.
		if (CanSave() && focus != Focus::Save)
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

	const auto hit = [&](sf::Vector2f point, UI::MenuLabel& label, sf::Vector2f centre) -> bool
	{
		return label.Bounds(centre, 1.f).contains(point);
	};

	if (const auto* moved = event.getIf<sf::Event::MouseMoved>())
	{
		const sf::Vector2f point = context.window.mapPixelToCoords(moved->position);
		if (CanSave() && hit(point, saveLabel, { saveCentreX, panelTop + NameRowOffset }))
		{
			focus = Focus::Save;
		}
		else if (hit(point, playAgainLabel, { CentreX - ButtonSpacing, buttonY }))
		{
			focus = Focus::PlayAgain;
		}
		else if (hit(point, mainMenuLabel, { CentreX + ButtonSpacing, buttonY }))
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
		if (CanSave() && hit(point, saveLabel, { saveCentreX, panelTop + NameRowOffset }))
		{
			SaveRecord();
			return;
		}
		if (hit(point, playAgainLabel, { CentreX - ButtonSpacing, buttonY }))
		{
			focus = Focus::PlayAgain;
			Activate();
		}
		else if (hit(point, mainMenuLabel, { CentreX + ButtonSpacing, buttonY }))
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
	savePulse = std::max(0.f, savePulse - deltaTime * 2.4f);

	leaveDialog.Update(deltaTime);
	if (const std::optional<bool> answer = leaveDialog.TakeResult(); answer && *answer)
	{
		BeginLeave();
	}

	if (focus == Focus::Save && !CanSave())
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

		if (glitchActive)
		{
			glitchTime += deltaTime;
			if (glitchTime >= glitchDuration)
			{
				glitchActive = false;
				glitchCooldown = Random::Float(1.9f, 4.6f);
			}
		}
		else
		{
			glitchCooldown -= deltaTime;
			if (glitchCooldown <= 0.f)
			{
				glitchActive = true;
				glitchTime = 0.f;
				glitchDuration = Random::Float(0.08f, 0.18f);
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
		RequestPush(std::make_unique<MenuShell>(context));
	}
}

void GameOverState::DrawButton(sf::RenderTarget& target, UI::MenuLabel& label, sf::Vector2f centre,
	sf::Color hue, bool selected, float alpha)
{
	const float press = (selected && pressTime < PressDuration)
		? std::sin(std::clamp(1.f - pressTime / PressDuration, 0.f, 1.f) * Pi)
		: 0.f;
	const float scale = (selected ? SelectedScale : 1.f) + PressPunch * press;
	const float drawAlpha = alpha * (selected ? 1.f : UnselectedAlpha);

	if (selected)
	{
		label.DrawGlow(target, buttonGlow, centre, scale,
			sf::Color(hue.r, hue.g, hue.b, static_cast<std::uint8_t>(std::clamp(alpha, 0.f, 1.f) * 255.f * ButtonGlowIntensity)));
	}

	label.Draw(target, centre, scale, hue, drawAlpha, PressFlash * press);
}

void GameOverState::Render(sf::RenderTarget& target)
{
	target.draw(backdrop);

	sf::RectangleShape dim(Screen);
	dim.setFillColor(sf::Color(0, 0, 0, SceneDim));
	target.draw(dim);

	if (isRecord)
	{
		celebration.RenderFireworks(target);
	}

	const float in = SmoothStep(appear);
	const auto contentAlpha = std::clamp((appear - 0.2f) / 0.8f, 0.f, 1.f);

	panel.SetColor(sf::Color(255, 255, 255, ToAlpha(in)));
	panel.Draw(target);

	if (isRecord)
	{
		celebration.RenderCornerSparks(target);
	}

	if (contentAlpha > 0.f)
	{
		const float rise = (1.f - EaseOutBack(headingDrop)) * 70.f;
		const float flicker = FlickerBrightness();
		const float glitch = glitchActive
			? std::sin(std::clamp(glitchTime / std::max(0.01f, glitchDuration), 0.f, 1.f) * Pi)
			: 0.f;

		const sf::Vector2f jitter = glitch > 0.f
			? sf::Vector2f{ std::sin(glitchTime * 190.f) * glitch * 7.f, std::sin(glitchTime * 250.f) * glitch * 4.f }
			: sf::Vector2f{ 0.f, 0.f };
		const sf::Vector2f base{ CentreX + jitter.x, panelTop + HeadingOffset - rise + jitter.y };

		sf::RenderStates additive;
		additive.blendMode = sf::BlendAdd;

		// Chromatic split during a glitch.
		if (glitch > 0.f)
		{
			const float dx = 6.f + glitch * 10.f;
			heading.setOutlineColor(sf::Color(0, 0, 0, 0));
			heading.setFillColor(sf::Color(255, 60, 60, ToAlpha(contentAlpha * 0.85f)));
			PlaceCentred(heading, { base.x + dx, base.y });
			target.draw(heading, additive);
			heading.setFillColor(sf::Color(60, 200, 255, ToAlpha(contentAlpha * 0.85f)));
			PlaceCentred(heading, { base.x - dx, base.y });
			target.draw(heading, additive);
		}

		const auto lit = [flicker](sf::Color c)
		{
			return sf::Color(
				static_cast<std::uint8_t>(static_cast<float>(c.r) * flicker),
				static_cast<std::uint8_t>(static_cast<float>(c.g) * flicker),
				static_cast<std::uint8_t>(static_cast<float>(c.b) * flicker), c.a);
		};

		const sf::Color headingFill = isRecord ? HeadingFillGold : HeadingFill;
		const sf::Color headingOutline = isRecord ? HeadingOutlineGold : HeadingOutline;
		const sf::Color headingGlowTint = isRecord ? HeadingGlowTintGold : HeadingGlowTint;

		heading.setFillColor(Faded(lit(headingFill), contentAlpha));
		heading.setOutlineColor(Faded(headingOutline, contentAlpha * flicker));
		PlaceCentred(heading, base);

		const sf::FloatRect glowArea{
			{ CentreX - 640.f, panelTop + HeadingOffset - 150.f }, { 1280.f, 280.f } };
		headingGlow.Draw(target, glowArea,
			[this](sf::RenderTarget& buffer, const sf::RenderStates& states) { buffer.draw(heading, states); },
			Faded(headingGlowTint, contentAlpha * flicker * 0.75f), false);

		target.draw(heading);

		for (Line& line : lines)
		{
			line.text.setFillColor(Faded(line.base, contentAlpha));
			target.draw(line.text);
		}

		if (isRecord)
		{
			recordBadge.setFillColor(Faded(BadgeColour, contentAlpha));
			target.draw(recordBadge);

			const sf::FloatRect field = NameFieldBounds(panelTop);
			const float rowY = field.position.y + field.size.y * 0.5f;
			const float textLeft = field.position.x + FieldTextInset;

			DrawRecessedField(target, field, contentAlpha);

			if (NameEntered())
			{
				const bool showCursor = std::fmod(cursorTime, 1.f) < 0.55f;
				nameField.setString(playerName + (showCursor ? sf::String("|") : sf::String(" ")));
				PlaceLeft(nameField, { textLeft, rowY });
				nameField.setFillColor(Faded(NameColour, contentAlpha));
				target.draw(nameField);
			}
			else
			{
				namePrompt.setFillColor(Faded(PromptColour, contentAlpha * 0.9f));
				PlaceLeft(namePrompt, { textLeft, rowY });
				target.draw(namePrompt);
			}

			const bool saveFocused = leaving == Leaving::No && !leaveDialog.IsOpen() && focus == Focus::Save;
			const sf::Color saveHue = recordSaved ? SavedHue : SaveHue;
			const float saveAlpha = contentAlpha * (recordSaved ? 0.55f : (CanSave() ? 1.f : 0.32f));
			const float pulse = savePulse > 0.f ? std::sin(std::clamp(savePulse, 0.f, 1.f) * Pi) : 0.f;
			const float saveScale = (saveFocused ? 1.06f : 1.f) + 0.12f * pulse;

			if (saveFocused)
			{
				saveLabel.DrawGlow(target, buttonGlow, { saveCentreX, rowY }, saveScale,
					sf::Color(saveHue.r, saveHue.g, saveHue.b, ToAlpha(contentAlpha * ButtonGlowIntensity)));
			}
			saveLabel.Draw(target, { saveCentreX, rowY }, saveScale, saveHue, saveAlpha, PressFlash * pulse);
		}
	}

	const float buttonAlpha = contentAlpha;
	DrawButton(target, playAgainLabel, { CentreX - ButtonSpacing, buttonY }, PlayHue,
		leaving == Leaving::No && focus == Focus::PlayAgain, buttonAlpha);
	DrawButton(target, mainMenuLabel, { CentreX + ButtonSpacing, buttonY }, MenuHue,
		leaving == Leaving::No && focus == Focus::MainMenu, buttonAlpha);

	if (leaving == Leaving::MainMenu)
	{
		sf::RectangleShape fade(Screen);
		fade.setFillColor(sf::Color(0, 0, 0, ToAlpha(leaveTimer / MainMenuDelay)));
		target.draw(fade);
	}

	leaveDialog.Render(target);
}
