#include "GameplayHUD.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>

#include "BoardRenderer.h"
#include "../core/Context.h"
#include "../gameplay/Board.h"
#include "../input/gamepad/GamepadManager.h"
#include "../input/gamepad/GamepadPrompts.h"
#include "../input/KeyName.h"
#include "../localization/LocalizationManager.h"
#include "../localization/TextKeys.h"
#include "../resources/Assets.h"
#include "../settings/GameSettings.h"
#include "../settings/SettingsManager.h"
#include "../utils/Easing.h"
#include "../utils/TimeFormat.h"

namespace
{
	// The HUD hugs the well: a narrow gap off its *outer* wall (the board draws a
	// one-block wall around the playfield) keeps every panel close so the eye
	// barely has to travel off the stack.
	constexpr float ScreenHeight = 1080.f;
	constexpr float WellOuterLeft = BoardRenderer::BoardPosition.x - BoardRenderer::BlockSize;
	constexpr float WellOuterRight = BoardRenderer::BoardPosition.x
		+ static_cast<float>(Board::Width + 1) * BoardRenderer::BlockSize;
	constexpr float WellOuterTop = BoardRenderer::BoardPosition.y - BoardRenderer::BlockSize;
	constexpr float WellOuterBottom = BoardRenderer::BoardPosition.y
		+ static_cast<float>(Board::VisibleHeight + 1) * BoardRenderer::BlockSize;

	constexpr float WellGap = 34.f;
	constexpr float PanelWidth = 220.f;
	constexpr float RowInset = 22.f;

	// Both side panels share the same internal shape: a caption, a square "hero"
	// area (the hold piece / the next queue), a divider, then two stacked stat
	// rows. They sit flush with the well's top edge -- the well is by far the
	// tallest thing on screen, so there's nothing to center either panel against.
	constexpr float CaptionOffset = 42.f;
	constexpr float HeroTop = 60.f;
	constexpr float HoldBoxSize = 150.f;
	constexpr float NextBoxHeight = 290.f;
	constexpr float DividerGap = 20.f;
	constexpr float RowsGap = 30.f;
	constexpr float RowHeight = 80.f;
	constexpr float BottomPadding = 30.f;

	constexpr float LeftPanelHeight = HeroTop + HoldBoxSize + DividerGap + RowsGap + RowHeight * 2.f + BottomPadding;
	constexpr float RightPanelHeight = HeroTop + NextBoxHeight + DividerGap + RowsGap + RowHeight * 2.f + BottomPadding;

	constexpr float PanelTop = WellOuterTop;

	constexpr float LeftX = WellOuterLeft - WellGap - PanelWidth;
	constexpr float RightX = WellOuterRight + WellGap;

	constexpr sf::FloatRect LeftPanelBounds{ { LeftX, PanelTop }, { PanelWidth, LeftPanelHeight } };
	constexpr sf::FloatRect RightPanelBounds{ { RightX, PanelTop }, { PanelWidth, RightPanelHeight } };

	constexpr float LeftColumnCenterX = LeftX + PanelWidth * 0.5f;
	constexpr float RightColumnCenterX = RightX + PanelWidth * 0.5f;

	constexpr float LeftRowsTop = PanelTop + HeroTop + HoldBoxSize + DividerGap + RowsGap;
	constexpr float RightRowsTop = PanelTop + HeroTop + NextBoxHeight + DividerGap + RowsGap;

	constexpr float LevelRowTop = LeftRowsTop;
	constexpr float TimeRowTop = LeftRowsTop + RowHeight;
	constexpr float ScoreRowTop = RightRowsTop;
	constexpr float LinesRowTop = RightRowsTop + RowHeight;

	constexpr float StatLabelOffset = 16.f;
	constexpr float StatValueOffset = 50.f;

	// The controls legend: one horizontal strip under the well, spanning the
	// full HUD width (both panels plus the well between them). One line per
	// entry ("Move: Left / Right") -- it's a reference the player rarely needs
	// mid-game, so it sits apart from HOLD/NEXT instead of competing with them
	// for a side column, and stays as short as the text it actually holds.
	constexpr float ControlsGap = 24.f;
	constexpr float ControlsBarHeight = 80.f;
	constexpr float ControlsTop = WellOuterBottom + ControlsGap;
	constexpr sf::FloatRect ControlsBounds
	{
		{ LeftX, ControlsTop },
		{ (RightX + PanelWidth) - LeftX, ControlsBarHeight }
	};

	constexpr unsigned int CaptionSize = 34;
	constexpr unsigned int StatLabelSize = 22;
	constexpr unsigned int StatValueSize = 40;
	constexpr unsigned int ControlsLabelSize = 30;
	constexpr unsigned int ControlsValueSize = 34;

	constexpr sf::Vector2f FrameTargetBorder{ 32.f, 32.f };
	constexpr float FillInset = 16.f;

	constexpr float FlashDuration = 0.5f;
	constexpr float PanelPulseDuration = 0.45f;
	constexpr sf::Color PanelPulseColor{ 255, 235, 150 };

	constexpr float DividerThickness = 2.f;
	constexpr float CaptionLetterSpacing = 1.4f;
	constexpr float StatLabelLetterSpacing = 1.2f;

	// The held-piece placeholder outline: inset from the hold box on every
	// side, so it doesn't touch the box's own edge.
	constexpr float HoldPlaceholderInset = 6.f;
	constexpr float PlaceholderOutlineThickness = 2.f;

	// Action names run much longer in some languages than in English; never
	// shrink the controls-legend text past this fraction of its full size.
	constexpr float MinControlsTextScale = 0.55f;

	// DrawValue: how much a stat's value grows at the peak of its flash.
	constexpr float FlashScaleBoost = 0.18f;

	// DrawPanelFrame: shake amplitude, and its two (deliberately different, so
	// the jitter doesn't trace a repeating line) frequencies on X and Y.
	constexpr float PanelShakeBase = 2.f;
	constexpr float PanelShakePerRank = 2.f;
	constexpr float PanelShakeFrequencyX = 47.f;
	constexpr float PanelShakeFrequencyY = 53.f;

	const sf::Color FillColor{ 8, 11, 17, 214 };
	const sf::Color CaptionColor{ 150, 172, 196 };
	const sf::Color ValueColor{ 255, 255, 255 };
	const sf::Color FlashColor{ 120, 230, 255 };
	const sf::Color DividerColor{ 150, 172, 196, 55 };
	const sf::Color PlaceholderColor{ 90, 110, 130, 130 };
	const sf::Color ControlsLabelColor{ 146, 162, 178 };
	const sf::Color ControlsValueColor{ 236, 240, 246 };

	[[nodiscard]] sf::Vector2f Center(const sf::FloatRect& rect)
	{
		return { rect.position.x + rect.size.x * 0.5f, rect.position.y + rect.size.y * 0.5f };
	}

	[[nodiscard]] sf::Color MixColor(sf::Color from, sf::Color to, float t)
	{
		return sf::Color(
			static_cast<std::uint8_t>(Easing::Lerp(static_cast<float>(from.r), static_cast<float>(to.r), t)),
			static_cast<std::uint8_t>(Easing::Lerp(static_cast<float>(from.g), static_cast<float>(to.g), t)),
			static_cast<std::uint8_t>(Easing::Lerp(static_cast<float>(from.b), static_cast<float>(to.b), t)),
			static_cast<std::uint8_t>(Easing::Lerp(static_cast<float>(from.a), static_cast<float>(to.a), t)));
	}

	void CenterText(sf::Text& text, sf::Vector2f center)
	{
		const sf::FloatRect bounds = text.getLocalBounds();
		text.setOrigin({ bounds.position.x + bounds.size.x * 0.5f, bounds.position.y + bounds.size.y * 0.5f });
		text.setPosition(center);
	}

	void AlignLeft(sf::Text& text, sf::Vector2f leftMiddle)
	{
		const sf::FloatRect bounds = text.getLocalBounds();
		text.setOrigin({ bounds.position.x, bounds.position.y + bounds.size.y * 0.5f });
		text.setPosition(leftMiddle);
	}
}

GameplayHUD::StatRow GameplayHUD::MakeStatRow(std::string_view labelKey, std::string_view initialValue,
	float centerX, float rowTop) const
{
	const sf::Font& font = context.fonts.Get(Assets::FontID::Main);

	StatRow row
	{
		sf::Text(font, context.localization.GetText(labelKey), StatLabelSize),
		sf::Text(font, initialValue, StatValueSize)
	};

	row.label.setFillColor(CaptionColor);
	row.label.setLetterSpacing(StatLabelLetterSpacing);
	CenterText(row.label, { centerX, rowTop + StatLabelOffset });

	row.value.setFillColor(ValueColor);
	CenterText(row.value, { centerX, rowTop + StatValueOffset });

	return row;
}

GameplayHUD::GameplayHUD(Context& context)
	: context(context)
	, leftFill({ LeftPanelBounds.size.x - FillInset * 2.f, LeftPanelBounds.size.y - FillInset * 2.f })
	, leftFrame(context.textures.Get(Assets::TextureID::UiFrameBlue), LeftPanelBounds,
		MenuFrameSourceBorder, FrameTargetBorder)
	, holdCaption(context.fonts.Get(Assets::FontID::Main), context.localization.GetText(TextKey::HUD::Hold), CaptionSize)
	, holdBoxBounds{ { LeftX + (PanelWidth - HoldBoxSize) * 0.5f, PanelTop + HeroTop }, { HoldBoxSize, HoldBoxSize } }
	, holdPlaceholder({ holdBoxBounds.size.x - HoldPlaceholderInset * 2.f, holdBoxBounds.size.y - HoldPlaceholderInset * 2.f })
	, leftDivider({ PanelWidth - RowInset * 2.f, DividerThickness })
	, levelRow(MakeStatRow(TextKey::HUD::Level, "1", LeftColumnCenterX, LevelRowTop))
	, timeRow(MakeStatRow(TextKey::HUD::Time, "0:00", LeftColumnCenterX, TimeRowTop))
	, rightFill({ RightPanelBounds.size.x - FillInset * 2.f, RightPanelBounds.size.y - FillInset * 2.f })
	, rightFrame(context.textures.Get(Assets::TextureID::UiFrameBlue), RightPanelBounds,
		MenuFrameSourceBorder, FrameTargetBorder)
	, nextCaption(context.fonts.Get(Assets::FontID::Main), context.localization.GetText(TextKey::HUD::Next), CaptionSize)
	, nextBoxBounds{ { RightX, PanelTop + HeroTop }, { PanelWidth, NextBoxHeight } }
	, rightDivider({ PanelWidth - RowInset * 2.f, DividerThickness })
	, scoreRow(MakeStatRow(TextKey::HUD::Score, "0", RightColumnCenterX, ScoreRowTop))
	, linesRow(MakeStatRow(TextKey::HUD::Lines, "0", RightColumnCenterX, LinesRowTop))
	, controlsFill({ ControlsBounds.size.x - FillInset * 2.f, ControlsBounds.size.y - FillInset * 2.f })
	, controlsFrame(context.textures.Get(Assets::TextureID::UiFrameBlue), ControlsBounds,
		MenuFrameSourceBorder, FrameTargetBorder)
{
	leftFill.setPosition({ LeftPanelBounds.position.x + FillInset, LeftPanelBounds.position.y + FillInset });
	leftFill.setFillColor(FillColor);

	CenterText(holdCaption, { Center(LeftPanelBounds).x, LeftPanelBounds.position.y + CaptionOffset });
	holdCaption.setFillColor(CaptionColor);
	holdCaption.setLetterSpacing(CaptionLetterSpacing);

	holdPlaceholder.setPosition({ holdBoxBounds.position.x + HoldPlaceholderInset, holdBoxBounds.position.y + HoldPlaceholderInset });
	holdPlaceholder.setFillColor(sf::Color::Transparent);
	holdPlaceholder.setOutlineColor(PlaceholderColor);
	holdPlaceholder.setOutlineThickness(PlaceholderOutlineThickness);

	leftDivider.setPosition({ LeftX + RowInset, PanelTop + HeroTop + HoldBoxSize + DividerGap });
	leftDivider.setFillColor(DividerColor);

	rightFill.setPosition({ RightPanelBounds.position.x + FillInset, RightPanelBounds.position.y + FillInset });
	rightFill.setFillColor(FillColor);

	CenterText(nextCaption, { Center(RightPanelBounds).x, RightPanelBounds.position.y + CaptionOffset });
	nextCaption.setFillColor(CaptionColor);
	nextCaption.setLetterSpacing(CaptionLetterSpacing);

	rightDivider.setPosition({ RightX + RowInset, PanelTop + HeroTop + NextBoxHeight + DividerGap });
	rightDivider.setFillColor(DividerColor);

	controlsFill.setPosition({ ControlsBounds.position.x + FillInset, ControlsBounds.position.y + FillInset });
	controlsFill.setFillColor(FillColor);

	BuildControlsLegend(context.settings.GetSettings().controls, context.settings.GetSettings().isHoldTetrominoEnabled,
		CurrentPromptMode());
}

void GameplayHUD::RefreshText()
{
	const LocalizationManager& text = context.localization;

	// setString alone leaves the old origin in place, computed from the old
	// string's (possibly narrower) ink bounds -- re-center every caption on
	// the same anchor it was built with, or a longer translation drifts off
	// its row's center line.
	holdCaption.setString(text.GetText(TextKey::HUD::Hold));
	CenterText(holdCaption, { Center(LeftPanelBounds).x, LeftPanelBounds.position.y + CaptionOffset });

	nextCaption.setString(text.GetText(TextKey::HUD::Next));
	CenterText(nextCaption, { Center(RightPanelBounds).x, RightPanelBounds.position.y + CaptionOffset });

	levelRow.label.setString(text.GetText(TextKey::HUD::Level));
	CenterText(levelRow.label, { LeftColumnCenterX, LevelRowTop + StatLabelOffset });

	timeRow.label.setString(text.GetText(TextKey::HUD::Time));
	CenterText(timeRow.label, { LeftColumnCenterX, TimeRowTop + StatLabelOffset });

	scoreRow.label.setString(text.GetText(TextKey::HUD::Score));
	CenterText(scoreRow.label, { RightColumnCenterX, ScoreRowTop + StatLabelOffset });

	linesRow.label.setString(text.GetText(TextKey::HUD::Lines));
	CenterText(linesRow.label, { RightColumnCenterX, LinesRowTop + StatLabelOffset });

	BuildControlsLegend(context.settings.GetSettings().controls, context.settings.GetSettings().isHoldTetrominoEnabled,
		CurrentPromptMode());
}

GameplayHUD::PromptMode GameplayHUD::CurrentPromptMode() const
{
	if (!context.gamepad.IsInUse())
		return PromptMode::Keyboard;

	switch (context.gamepad.GetLayout())
	{
	case GamepadManager::Layout::Xbox:
		return PromptMode::Xbox;
	case GamepadManager::Layout::PlayStation:
		return PromptMode::PlayStation;

	default:
		return PromptMode::Keyboard;   // Generic has no icon set
	}
}

void GameplayHUD::RefreshControlsLegend(const ControlSettings& controls, bool isHoldEnabled)
{
	const PromptMode mode = CurrentPromptMode();

	const bool isUnchanged =
		controls.moveTetrominoLeft == legendControls.moveTetrominoLeft &&
		controls.moveTetrominoRight == legendControls.moveTetrominoRight &&
		controls.softDropTetromino == legendControls.softDropTetromino &&
		controls.hardDropTetromino == legendControls.hardDropTetromino &&
		controls.rotateTetrominoClockwise == legendControls.rotateTetrominoClockwise &&
		controls.rotateTetrominoCounterClockwise == legendControls.rotateTetrominoCounterClockwise &&
		controls.holdTetromino == legendControls.holdTetromino &&
		controls.pauseGame == legendControls.pauseGame &&
		isHoldEnabled == isLegendHoldEnabled &&
		mode == legendPromptMode;

	if (isUnchanged)
		return;

	BuildControlsLegend(controls, isHoldEnabled, mode);
}

void GameplayHUD::BuildControlsLegend(const ControlSettings& controls, bool isHoldEnabled, PromptMode mode)
{
	legendControls = controls;
	isLegendHoldEnabled = isHoldEnabled;
	legendPromptMode = mode;
	controlsEntries.clear();

	// One entry per action, spread evenly across the strip. In keyboard mode,
	// key names are read from the live bindings (layout-independent, via
	// GetKeyName); in gamepad mode, the value is one or two button-prompt
	// icons instead -- Xbox or PlayStation, whichever GamepadManager reports.
	const sf::Font& font = context.fonts.Get(Assets::FontID::Main);
	const bool isUsingIcons = mode != PromptMode::Keyboard;

	const GamepadManager::Layout layout =
		mode == PromptMode::PlayStation ? GamepadManager::Layout::PlayStation : GamepadManager::Layout::Xbox;
	const sf::Texture* atlas = isUsingIcons ? &context.textures.Get(GamepadPrompts::GetAtlasFor(layout)) : nullptr;

	const auto twoKeys =
		[](sf::Keyboard::Scancode a, sf::Keyboard::Scancode b)
		{
			return GetKeyName(a) + sf::String(" / ") + GetKeyName(b);
		};

	using Prompt = GamepadPrompts::Action;

	struct EntryDefinition
	{
		std::string_view labelKey;
		sf::String text;                  // keyboard mode
		std::vector<Prompt> icons;         // gamepad mode
	};

	std::vector<EntryDefinition> definitions =
	{
		{ TextKey::HUD::Move, twoKeys(controls.moveTetrominoLeft, controls.moveTetrominoRight),
			{ Prompt::MoveLeft, Prompt::MoveRight } },
		{ TextKey::HUD::SoftDrop, GetKeyName(controls.softDropTetromino), { Prompt::SoftDrop } },
		{ TextKey::HUD::HardDrop, GetKeyName(controls.hardDropTetromino), { Prompt::HardDrop } },
		{ TextKey::HUD::Rotate, twoKeys(controls.rotateTetrominoCounterClockwise, controls.rotateTetrominoClockwise),
			{ Prompt::RotateCounterClockwise, Prompt::RotateClockwise } },
	};

	// Omitted when hold itself is turned off in Options -- nothing to bind.
	if (isHoldEnabled)
		definitions.push_back({ TextKey::HUD::HoldKey, GetKeyName(controls.holdTetromino), { Prompt::Hold } });

	definitions.push_back({ TextKey::HUD::Pause, GetKeyName(controls.pauseGame), { Prompt::Pause } });

	const float lineY = Center(ControlsBounds).y;
	constexpr float IconGap = 8.f;
	constexpr float IconScale = 2.6f;

	// Build every entry first (so its value -- text or icons -- and width are
	// known), then lay them out with equal gaps, including from the frame's own
	// inner edges: the gap before the first entry and after the last one is the
	// same size as the gaps between entries, rather than each entry just
	// centring in an equal share of the strip (which does not give equal
	// *visual* spacing, since the entries themselves are different widths).
	std::vector<float> labelWidths;
	std::vector<float> valueWidths;
	float totalWidth = 0.f;

	for (const EntryDefinition& def : definitions)
	{
		ControlEntry entry
		{
			sf::Text(font, context.localization.GetText(def.labelKey) + sf::String(": "), ControlsLabelSize),
			sf::Text(font, sf::String(), ControlsValueSize)
		};

		entry.label.setFillColor(ControlsLabelColor);

		float valueWidth = 0.f;

		if (isUsingIcons)
		{
			for (const Prompt icon : def.icons)
			{
				sf::Sprite sprite(*atlas);
				sprite.setTextureRect(GamepadPrompts::GetIconFor(layout, icon));
				sprite.setScale({ IconScale, IconScale });
				valueWidth += sprite.getLocalBounds().size.x * IconScale;
				entry.icons.push_back(std::move(sprite));
			}
			valueWidth += IconGap * static_cast<float>(def.icons.size() - 1);
		}
		else
		{
			entry.value.setString(def.text);
			entry.value.setFillColor(ControlsValueColor);
			valueWidth = entry.value.getLocalBounds().size.x;
		}

		const float labelWidth = entry.label.getLocalBounds().size.x;
		totalWidth += labelWidth + valueWidth;
		labelWidths.push_back(labelWidth);
		valueWidths.push_back(valueWidth);
		controlsEntries.push_back(std::move(entry));
	}

	const float innerLeft = ControlsBounds.position.x + FillInset;
	const float innerWidth = ControlsBounds.size.x - FillInset * 2.f;

	// Action names run much longer in some languages than in English; shrink
	// the label/value text (never below 55%) rather than let entries spill
	// past the frame or overlap the gamepad icons, and keep a sane minimum
	// gap either way.
	constexpr float MinGap = 12.f;
	const std::size_t count = controlsEntries.size();
	const float available = innerWidth - MinGap * static_cast<float>(count + 1);
	const float textScale = (totalWidth > available && totalWidth > 0.f)
		? std::max(MinControlsTextScale, available / totalWidth)
		: 1.f;

	const float scaledTotalWidth = totalWidth * textScale;
	const float gap = std::max(MinGap, (innerWidth - scaledTotalWidth) / static_cast<float>(count + 1));

	float cursorX = innerLeft + gap;

	for (std::size_t i = 0; i < controlsEntries.size(); i++)
	{
		ControlEntry& entry = controlsEntries[i];
		const float labelWidth = labelWidths[i] * textScale;
		const float valueWidth = entry.icons.empty() ? valueWidths[i] * textScale : valueWidths[i];

		entry.label.setScale({ textScale, textScale });
		AlignLeft(entry.label, { cursorX, lineY });

		if (entry.icons.empty())
		{
			entry.value.setScale({ textScale, textScale });
			AlignLeft(entry.value, { cursorX + labelWidth, lineY });
		}
		else
		{
			float iconX = cursorX + labelWidth;
			for (sf::Sprite& icon : entry.icons)
			{
				const sf::FloatRect bounds = icon.getLocalBounds();
				icon.setOrigin({ bounds.position.x, bounds.position.y + bounds.size.y * 0.5f });
				icon.setPosition({ iconX, lineY });
				iconX += bounds.size.x * IconScale + IconGap;
			}
		}

		cursorX += labelWidth + valueWidth + gap;
	}
}

bool GameplayHUD::HoldVisible() const
{
	return isHoldVisible;
}

sf::FloatRect GameplayHUD::HoldPreviewArea() const
{
	return holdBoxBounds;
}

bool GameplayHUD::NextVisible() const
{
	return isNextVisible;
}

sf::FloatRect GameplayHUD::NextPreviewArea() const
{
	return nextBoxBounds;
}

void GameplayHUD::Set(int score, int level, int lines, float seconds)
{
	scoreRow.value.setString(std::to_string(score));
	levelRow.value.setString(std::to_string(level));
	linesRow.value.setString(std::to_string(lines));
	timeRow.value.setString(TimeFormat::GetTimeString(seconds));

	CenterText(scoreRow.value, { RightColumnCenterX, ScoreRowTop + StatValueOffset });
	CenterText(levelRow.value, { LeftColumnCenterX, LevelRowTop + StatValueOffset });
	CenterText(linesRow.value, { RightColumnCenterX, LinesRowTop + StatValueOffset });
	CenterText(timeRow.value, { LeftColumnCenterX, TimeRowTop + StatValueOffset });
}

void GameplayHUD::Update(float deltaTime)
{
	levelRow.flash = std::max(0.f, levelRow.flash - deltaTime / FlashDuration);
	timeRow.flash = std::max(0.f, timeRow.flash - deltaTime / FlashDuration);
	scoreRow.flash = std::max(0.f, scoreRow.flash - deltaTime / FlashDuration);
	linesRow.flash = std::max(0.f, linesRow.flash - deltaTime / FlashDuration);

	leftPanelPulse = std::max(0.f, leftPanelPulse - deltaTime / PanelPulseDuration);
	rightPanelPulse = std::max(0.f, rightPanelPulse - deltaTime / PanelPulseDuration);
}

void GameplayHUD::SetVisible(Element element, bool visible)
{
	switch (element)
	{
	case Element::Hold:
		isHoldVisible = visible;
		break;
	case Element::Next:
		isNextVisible = visible;
		break;
	case Element::Score:
		scoreRow.isVisible = visible;
		break;
	case Element::Lines:
		linesRow.isVisible = visible;
		break;
	case Element::Level:
		levelRow.isVisible = visible;
		break;
	case Element::Time:
		timeRow.isVisible = visible;
		break;
	case Element::ControlsLegend:
		areControlsShown = visible;
		break;
	}
}

void GameplayHUD::OnRowsCleared(int rank)
{
	scoreRow.flash = 1.f;
	linesRow.flash = 1.f;
	rightPanelPulse = 1.f;
	rightPanelPulseRank = rank;
}

void GameplayHUD::OnLevelUp()
{
	levelRow.flash = 1.f;
	leftPanelPulse = 1.f;
}

void GameplayHUD::DrawValue(sf::RenderTarget& target, const sf::Text& value, float flash) const
{
	if (flash <= 0.f)
	{
		target.draw(value);
		return;
	}

	const float ease = Easing::EaseOutCubic(flash);

	sf::Text lit = value;
	lit.setScale({ 1.f + FlashScaleBoost * ease, 1.f + FlashScaleBoost * ease });
	lit.setFillColor(MixColor(ValueColor, FlashColor, ease));
	target.draw(lit);
}

void GameplayHUD::DrawStatRow(sf::RenderTarget& target, const StatRow& row) const
{
	target.draw(row.label);
	DrawValue(target, row.value, row.flash);
}

void GameplayHUD::DrawPanelFrame(sf::RenderTarget& target, NineSliceFrame& frame, float pulse, int pulseRank) const
{
	if (pulse <= 0.f)
	{
		frame.SetColor(sf::Color::White);
		frame.Draw(target);
		return;
	}

	const float ease = Easing::EaseOutCubic(pulse);
	const float shakeMagnitude = (PanelShakeBase + PanelShakePerRank * static_cast<float>(pulseRank)) * ease;
	const sf::Vector2f jitter
	{
		shakeMagnitude * std::sin(context.totalTime * PanelShakeFrequencyX),
		shakeMagnitude * std::cos(context.totalTime * PanelShakeFrequencyY)
	};

	sf::RenderStates states;
	states.transform.translate(jitter);

	frame.SetColor(MixColor(sf::Color::White, PanelPulseColor, ease));
	frame.Draw(target, states);
}

void GameplayHUD::Render(sf::RenderTarget& target) const
{
	const bool isLeftPanelVisible = isHoldVisible || levelRow.isVisible || timeRow.isVisible;

	if (isLeftPanelVisible)
	{
		target.draw(leftFill);
		DrawPanelFrame(target, leftFrame, leftPanelPulse, 0);

		if (isHoldVisible)
		{
			target.draw(holdCaption);
			target.draw(holdPlaceholder);
		}

		if (levelRow.isVisible || timeRow.isVisible)
			target.draw(leftDivider);

		if (levelRow.isVisible)
			DrawStatRow(target, levelRow);
		if (timeRow.isVisible)
			DrawStatRow(target, timeRow);
	}

	const bool isRightPanelVisible = isNextVisible || scoreRow.isVisible || linesRow.isVisible;

	if (isRightPanelVisible)
	{
		target.draw(rightFill);
		DrawPanelFrame(target, rightFrame, rightPanelPulse, rightPanelPulseRank);

		// The queued pieces themselves are drawn by BoardRenderer, via
		// NextPreviewArea() -- this only frames the panel and its caption.
		if (isNextVisible)
			target.draw(nextCaption);

		if (scoreRow.isVisible || linesRow.isVisible)
			target.draw(rightDivider);

		if (scoreRow.isVisible)
			DrawStatRow(target, scoreRow);
		if (linesRow.isVisible)
			DrawStatRow(target, linesRow);
	}

	if (areControlsShown)
	{
		target.draw(controlsFill);
		controlsFrame.Draw(target);

		for (const ControlEntry& entry : controlsEntries)
		{
			target.draw(entry.label);
			if (entry.icons.empty())
			{
				target.draw(entry.value);
			}
			else
			{
				for (const sf::Sprite& icon : entry.icons)
					target.draw(icon);
			}
		}
	}
}
