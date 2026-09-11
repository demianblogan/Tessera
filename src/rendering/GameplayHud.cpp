#include "GameplayHud.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

#include "BoardRenderer.h"
#include "../core/Context.h"
#include "../gameplay/Board.h"
#include "../input/KeyName.h"
#include "../localization/LocalizationManager.h"
#include "../localization/TextKeys.h"
#include "../resources/Assets.h"
#include "../settings/GameSettings.h"
#include "../settings/SettingsManager.h"
#include "../ui/Easing.h"
#include "../utils/TimeFormat.h"

namespace
{
	// The HUD hugs the well: a narrow gap off its *outer* wall (the board draws a
	// one-block wall around the playfield) keeps every panel close so the eye
	// barely has to travel off the stack.
	constexpr float ScreenHeight = 1080.f;
	constexpr float WellOuterLeft = BoardRenderer::BoardPosition.x - BoardRenderer::BlockSize;
	constexpr float WellOuterRight = BoardRenderer::BoardPosition.x
		+ static_cast<float>(Board::WIDTH + 1) * BoardRenderer::BlockSize;
	constexpr float WellOuterTop = BoardRenderer::BoardPosition.y - BoardRenderer::BlockSize;
	constexpr float WellOuterBottom = BoardRenderer::BoardPosition.y
		+ static_cast<float>(Board::VisibleHeight + 1) * BoardRenderer::BlockSize;

	constexpr float WellGap = 34.f;
	constexpr float PanelWidth = 220.f;
	constexpr float RowInset = 22.f;

	// Both side panels share the same internal shape: a caption, a square "hero"
	// area (the hold piece / the next queue), a divider, then two stacked stat
	// rows. They sit flush with the well's top edge -- the well is by far the
	// tallest thing on screen, so there's nothing to centre either panel against.
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

	constexpr float LeftColumnCentreX = LeftX + PanelWidth * 0.5f;
	constexpr float RightColumnCentreX = RightX + PanelWidth * 0.5f;

	constexpr float LeftRowsTop = PanelTop + HeroTop + HoldBoxSize + DividerGap + RowsGap;
	constexpr float RightRowsTop = PanelTop + HeroTop + NextBoxHeight + DividerGap + RowsGap;

	constexpr float LevelRowTop = LeftRowsTop;
	constexpr float TimeRowTop = LeftRowsTop + RowHeight;
	constexpr float ScoreRowTop = RightRowsTop;
	constexpr float LinesRowTop = RightRowsTop + RowHeight;

	constexpr float StatLabelOffset = 16.f;
	constexpr float StatValueOffset = 50.f;

	// The controls legend: one horizontal strip under the well, spanning the
	// full HUD width (both panels plus the well between them). It's a reference
	// the player rarely needs mid-game, so it sits apart from HOLD/NEXT instead
	// of competing with them for a side column.
	constexpr float ControlsGap = 24.f;
	constexpr float ControlsBottomMargin = 16.f;
	constexpr float ControlsTop = WellOuterBottom + ControlsGap;
	constexpr sf::FloatRect ControlsBounds{
		{ LeftX, ControlsTop },
		{ (RightX + PanelWidth) - LeftX, ScreenHeight - ControlsTop - ControlsBottomMargin } };

	constexpr float ControlsLabelOffset = 34.f;
	constexpr float ControlsValueOffset = 68.f;

	constexpr unsigned int CaptionSize = 34;
	constexpr unsigned int StatLabelSize = 22;
	constexpr unsigned int StatValueSize = 40;
	constexpr unsigned int ControlsLabelSize = 19;
	constexpr unsigned int ControlsValueSize = 25;

	constexpr sf::Vector2f FrameTargetBorder{ 32.f, 32.f };
	constexpr float FillInset = 16.f;

	constexpr float FlashDuration = 0.5f;

	const sf::Color FillColour{ 8, 11, 17, 214 };
	const sf::Color CaptionColour{ 150, 172, 196 };
	const sf::Color ValueColour{ 255, 255, 255 };
	const sf::Color FlashColour{ 120, 230, 255 };
	const sf::Color DividerColour{ 150, 172, 196, 55 };
	const sf::Color PlaceholderColour{ 90, 110, 130, 130 };
	const sf::Color ControlsLabelColour{ 146, 162, 178 };
	const sf::Color ControlsValueColour{ 236, 240, 246 };

	[[nodiscard]] sf::Vector2f Centre(const sf::FloatRect& rect)
	{
		return { rect.position.x + rect.size.x * 0.5f, rect.position.y + rect.size.y * 0.5f };
	}

	[[nodiscard]] sf::Color MixColour(sf::Color from, sf::Color to, float t)
	{
		return sf::Color(
			static_cast<std::uint8_t>(UI::Easing::Lerp(static_cast<float>(from.r), static_cast<float>(to.r), t)),
			static_cast<std::uint8_t>(UI::Easing::Lerp(static_cast<float>(from.g), static_cast<float>(to.g), t)),
			static_cast<std::uint8_t>(UI::Easing::Lerp(static_cast<float>(from.b), static_cast<float>(to.b), t)),
			static_cast<std::uint8_t>(UI::Easing::Lerp(static_cast<float>(from.a), static_cast<float>(to.a), t)));
	}

	void CentreText(sf::Text& text, sf::Vector2f centre)
	{
		const sf::FloatRect bounds = text.getLocalBounds();
		text.setOrigin({ bounds.position.x + bounds.size.x * 0.5f, bounds.position.y + bounds.size.y * 0.5f });
		text.setPosition(centre);
	}
}

GameplayHud::StatRow GameplayHud::MakeStatRow(std::string_view labelKey, std::string_view initialValue,
	float centreX, float rowTop) const
{
	const sf::Font& font = context.fonts.Get(Assets::FontID::Main);

	StatRow row{
		sf::Text(font, context.localization.GetText(labelKey), StatLabelSize),
		sf::Text(font, initialValue, StatValueSize)
	};

	row.label.setFillColor(CaptionColour);
	row.label.setLetterSpacing(1.2f);
	CentreText(row.label, { centreX, rowTop + StatLabelOffset });

	row.value.setFillColor(ValueColour);
	CentreText(row.value, { centreX, rowTop + StatValueOffset });

	return row;
}

GameplayHud::GameplayHud(Context& context)
	: context(context)
	, leftFill({ LeftPanelBounds.size.x - FillInset * 2.f, LeftPanelBounds.size.y - FillInset * 2.f })
	, leftFrame(context.textures.Get(Assets::TextureID::UiFrameBlue), LeftPanelBounds,
		UI::MenuFrameSourceBorder, FrameTargetBorder)
	, holdCaption(context.fonts.Get(Assets::FontID::Main), context.localization.GetText(TextKey::Hud::Hold), CaptionSize)
	, holdBoxBounds{ { LeftX + (PanelWidth - HoldBoxSize) * 0.5f, PanelTop + HeroTop }, { HoldBoxSize, HoldBoxSize } }
	, holdPlaceholder({ holdBoxBounds.size.x - 12.f, holdBoxBounds.size.y - 12.f })
	, leftDivider({ PanelWidth - RowInset * 2.f, 2.f })
	, levelRow(MakeStatRow(TextKey::Hud::Level, "1", LeftColumnCentreX, LevelRowTop))
	, timeRow(MakeStatRow(TextKey::Hud::Time, "0:00", LeftColumnCentreX, TimeRowTop))
	, rightFill({ RightPanelBounds.size.x - FillInset * 2.f, RightPanelBounds.size.y - FillInset * 2.f })
	, rightFrame(context.textures.Get(Assets::TextureID::UiFrameBlue), RightPanelBounds,
		UI::MenuFrameSourceBorder, FrameTargetBorder)
	, nextCaption(context.fonts.Get(Assets::FontID::Main), context.localization.GetText(TextKey::Hud::Next), CaptionSize)
	, nextBoxBounds{ { RightX, PanelTop + HeroTop }, { PanelWidth, NextBoxHeight } }
	, rightDivider({ PanelWidth - RowInset * 2.f, 2.f })
	, scoreRow(MakeStatRow(TextKey::Hud::Score, "0", RightColumnCentreX, ScoreRowTop))
	, linesRow(MakeStatRow(TextKey::Hud::Lines, "0", RightColumnCentreX, LinesRowTop))
	, controlsFill({ ControlsBounds.size.x - FillInset * 2.f, ControlsBounds.size.y - FillInset * 2.f })
	, controlsFrame(context.textures.Get(Assets::TextureID::UiFrameBlue), ControlsBounds,
		UI::MenuFrameSourceBorder, FrameTargetBorder)
{
	leftFill.setPosition({ LeftPanelBounds.position.x + FillInset, LeftPanelBounds.position.y + FillInset });
	leftFill.setFillColor(FillColour);

	CentreText(holdCaption, { Centre(LeftPanelBounds).x, LeftPanelBounds.position.y + CaptionOffset });
	holdCaption.setFillColor(CaptionColour);
	holdCaption.setLetterSpacing(1.4f);

	holdPlaceholder.setPosition({ holdBoxBounds.position.x + 6.f, holdBoxBounds.position.y + 6.f });
	holdPlaceholder.setFillColor(sf::Color::Transparent);
	holdPlaceholder.setOutlineColor(PlaceholderColour);
	holdPlaceholder.setOutlineThickness(2.f);

	leftDivider.setPosition({ LeftX + RowInset, PanelTop + HeroTop + HoldBoxSize + DividerGap });
	leftDivider.setFillColor(DividerColour);

	rightFill.setPosition({ RightPanelBounds.position.x + FillInset, RightPanelBounds.position.y + FillInset });
	rightFill.setFillColor(FillColour);

	CentreText(nextCaption, { Centre(RightPanelBounds).x, RightPanelBounds.position.y + CaptionOffset });
	nextCaption.setFillColor(CaptionColour);
	nextCaption.setLetterSpacing(1.4f);

	rightDivider.setPosition({ RightX + RowInset, PanelTop + HeroTop + NextBoxHeight + DividerGap });
	rightDivider.setFillColor(DividerColour);

	controlsFill.setPosition({ ControlsBounds.position.x + FillInset, ControlsBounds.position.y + FillInset });
	controlsFill.setFillColor(FillColour);

	// One entry per action, spread evenly across the strip; key names are read
	// once from the live bindings (layout-independent, via Input::KeyName).
	const sf::Font& font = context.fonts.Get(Assets::FontID::Main);
	const ControlSettings& controls = context.settings.GetSettings().controls;

	const auto twoKeys = [](sf::Keyboard::Scancode a, sf::Keyboard::Scancode b)
	{
		return Input::KeyName(a) + sf::String(" / ") + Input::KeyName(b);
	};

	const std::array<std::pair<std::string_view, sf::String>, 5> entries =
	{ {
		{ TextKey::Hud::Move,     twoKeys(controls.moveLeft, controls.moveRight) },
		{ TextKey::Hud::SoftDrop, Input::KeyName(controls.softDrop) },
		{ TextKey::Hud::HardDrop, Input::KeyName(controls.hardDrop) },
		{ TextKey::Hud::Rotate,   twoKeys(controls.rotateCounterClockwise, controls.rotateClockwise) },
		{ TextKey::Hud::Pause,    Input::KeyName(controls.pause) },
	} };

	const float columnStep = ControlsBounds.size.x / static_cast<float>(entries.size());

	for (std::size_t i = 0; i < entries.size(); i++)
	{
		const float x = ControlsBounds.position.x + columnStep * (static_cast<float>(i) + 0.5f);

		ControlEntry entry{
			sf::Text(font, context.localization.GetText(entries[i].first), ControlsLabelSize),
			sf::Text(font, entries[i].second, ControlsValueSize)
		};
		entry.label.setFillColor(ControlsLabelColour);
		entry.label.setLetterSpacing(1.2f);
		CentreText(entry.label, { x, ControlsBounds.position.y + ControlsLabelOffset });

		entry.value.setFillColor(ControlsValueColour);
		CentreText(entry.value, { x, ControlsBounds.position.y + ControlsValueOffset });

		controlsEntries.push_back(std::move(entry));
	}
}

void GameplayHud::Set(int score, int level, int lines, float seconds)
{
	scoreRow.value.setString(std::to_string(score));
	levelRow.value.setString(std::to_string(level));
	linesRow.value.setString(std::to_string(lines));
	timeRow.value.setString(TimeFormat::MinutesSeconds(seconds));

	CentreText(scoreRow.value, { RightColumnCentreX, ScoreRowTop + StatValueOffset });
	CentreText(levelRow.value, { LeftColumnCentreX, LevelRowTop + StatValueOffset });
	CentreText(linesRow.value, { RightColumnCentreX, LinesRowTop + StatValueOffset });
	CentreText(timeRow.value, { LeftColumnCentreX, TimeRowTop + StatValueOffset });
}

void GameplayHud::Update(float deltaTime)
{
	levelRow.flash = std::max(0.f, levelRow.flash - deltaTime / FlashDuration);
	timeRow.flash = std::max(0.f, timeRow.flash - deltaTime / FlashDuration);
	scoreRow.flash = std::max(0.f, scoreRow.flash - deltaTime / FlashDuration);
	linesRow.flash = std::max(0.f, linesRow.flash - deltaTime / FlashDuration);
}

void GameplayHud::SetVisible(Element element, bool visible)
{
	switch (element)
	{
	case Element::Hold:  holdVisible = visible; break;
	case Element::Next:  nextVisible = visible; break;
	case Element::Score: scoreRow.visible = visible; break;
	case Element::Lines: linesRow.visible = visible; break;
	case Element::Level: levelRow.visible = visible; break;
	case Element::Time:  timeRow.visible = visible; break;
	case Element::ControlsLegend: showControls = visible; break;
	}
}

void GameplayHud::OnRowsCleared()
{
	scoreRow.flash = 1.f;
	linesRow.flash = 1.f;
}

void GameplayHud::OnLevelUp()
{
	levelRow.flash = 1.f;
}

void GameplayHud::DrawValue(sf::RenderTarget& target, const sf::Text& value, float flash) const
{
	if (flash <= 0.f)
	{
		target.draw(value);
		return;
	}

	const float ease = UI::Easing::EaseOutCubic(flash);

	sf::Text lit = value;
	lit.setScale({ 1.f + 0.18f * ease, 1.f + 0.18f * ease });
	lit.setFillColor(MixColour(ValueColour, FlashColour, ease));
	target.draw(lit);
}

void GameplayHud::DrawStatRow(sf::RenderTarget& target, const StatRow& row) const
{
	target.draw(row.label);
	DrawValue(target, row.value, row.flash);
}

void GameplayHud::Render(sf::RenderTarget& target) const
{
	const bool leftPanelVisible = holdVisible || levelRow.visible || timeRow.visible;

	if (leftPanelVisible)
	{
		target.draw(leftFill);
		leftFrame.Draw(target);

		if (holdVisible)
		{
			target.draw(holdCaption);
			target.draw(holdPlaceholder);
		}

		if (levelRow.visible || timeRow.visible)
		{
			target.draw(leftDivider);
		}

		if (levelRow.visible) { DrawStatRow(target, levelRow); }
		if (timeRow.visible)  { DrawStatRow(target, timeRow); }
	}

	const bool rightPanelVisible = nextVisible || scoreRow.visible || linesRow.visible;

	if (rightPanelVisible)
	{
		target.draw(rightFill);
		rightFrame.Draw(target);

		// The queued pieces themselves are drawn by BoardRenderer, via
		// NextPreviewArea() -- this only frames the panel and its caption.
		if (nextVisible)
		{
			target.draw(nextCaption);
		}

		if (scoreRow.visible || linesRow.visible)
		{
			target.draw(rightDivider);
		}

		if (scoreRow.visible) { DrawStatRow(target, scoreRow); }
		if (linesRow.visible) { DrawStatRow(target, linesRow); }
	}

	if (showControls)
	{
		target.draw(controlsFill);
		controlsFrame.Draw(target);

		for (const ControlEntry& entry : controlsEntries)
		{
			target.draw(entry.label);
			target.draw(entry.value);
		}
	}
}
