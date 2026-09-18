#include "RecordsScreen.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <cstdint>
#include <string>

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Mouse.hpp>

#include "../../audio/AudioPlayer.h"
#include "../../core/Context.h"
#include "../../input/MenuInput.h"
#include "../../localization/LocalizationManager.h"
#include "../../localization/TextKeys.h"
#include "../../resources/Assets.h"
#include "../../statistics/HighScoreManager.h"
#include "../ColorUtils.h"
#include "../../states/ScreenHost.h"

namespace
{
	constexpr sf::FloatRect PanelBounds{ { 260.f, 210.f }, { 1400.f, 710.f } };
	constexpr sf::Vector2f PanelTargetBorder{ 44.f, 44.f };

	constexpr float HeaderY = 270.f;
	constexpr float RuleY = 304.f;
	constexpr float RowTopY = 344.f;
	constexpr float RowStep = 57.f;

	// Column anchors: rank / score / lines / level are right-aligned to their
	// value, the name is left-aligned.
	constexpr float ColRankRight = 440.f;
	constexpr float ColNameLeft = 490.f;
	constexpr float ColScoreRight = 1226.f;
	constexpr float ColLinesRight = 1436.f;
	constexpr float ColLevelRight = 1596.f;

	constexpr unsigned int RankSize = 32;
	constexpr unsigned int NameSize = 42;
	constexpr unsigned int ScoreSize = 46;
	constexpr unsigned int SubSize = 34;
	constexpr unsigned int HeaderSize = 38;

	constexpr unsigned int ButtonTextSize = 42;
	constexpr float ButtonRowY = 968.f;
	constexpr float ButtonRowCenterX = 960.f;
	constexpr float ButtonGap = 60.f;   // clear space between the two buttons' ink

	constexpr float IntroDuration = 0.24f;
	constexpr float ExitDuration = 0.18f;
	constexpr float PressDuration = 0.18f;
	constexpr float PressPunch = 0.12f;
	constexpr float PressFlash = 0.55f;
	constexpr float SelectedScale = 1.05f;
	constexpr float UnselectedAlpha = 0.5f;
	constexpr float ButtonGlowIntensity = 0.5f;
	constexpr float Pi = std::numbers::pi_v<float>;

	// The header/rows divider rule.
	constexpr float RuleWidth = 1240.f;
	constexpr float RuleThickness = 2.f;
	constexpr float RuleX = 360.f;

	constexpr float HeaderLetterSpacing = 1.2f;

	// DoReset()'s confirmation chime, pitched down from the default.
	constexpr float ResetConfirmPitch = 0.7f;

	// RefreshRows()'s score-column tint: how far toward white the accent mixes.
	constexpr float ScoreColorMixFraction = 0.5f;

	const sf::Color HeaderColor{ 162, 116, 202 };
	const sf::Color RankColor{ 150, 135, 165 };
	const sf::Color NameColor{ 232, 236, 244 };
	const sf::Color ChampionColor{ 255, 255, 255 };
	const sf::Color EmptyColor{ 120, 118, 130 };
	const sf::Color SubColor{ 158, 155, 172 };
	const sf::Color RuleColor{ 150, 90, 200, 150 };
	const sf::Color ResetHue{ 255, 162, 62 };   // the Options "Reset" orange
	const sf::Color BackHue{ 232, 236, 244 };

	[[nodiscard]] sf::Color Faded(sf::Color color, float alpha)
	{
		return sf::Color(color.r, color.g, color.b,
			static_cast<std::uint8_t>(static_cast<float>(color.a) * std::clamp(alpha, 0.f, 1.f)));
	}

	// Sets the text's origin so it sits at `x` (align -1 = left edge, +1 = right
	// edge) and is vertically centered on `y`.
	void PlaceCell(sf::Text& text, float x, float y, int align)
	{
		const sf::FloatRect bounds = text.getLocalBounds();
		const float originX = align < 0 ? bounds.position.x : bounds.position.x + bounds.size.x;
		text.setOrigin({ originX, bounds.position.y + bounds.size.y * 0.5f });
		text.setPosition({ x, y });
	}
}

RecordsScreen::RecordsScreen(ScreenHost& host, sf::Color accent)
	: MenuScreen(host)
	, accent(accent)
	, panel(context.textures.Get(Assets::TextureID::UiFramePurple), PanelBounds,
		MenuFrameSourceBorder, PanelTargetBorder)
	, resetLabel(context.fonts.Get(Assets::FontID::Menu), ButtonTextSize)
	, backLabel(context.fonts.Get(Assets::FontID::Menu), ButtonTextSize)
	, buttonGlow(context.shaders.Get(Assets::ShaderID::NeonDilate), context.shaders.Get(Assets::ShaderID::NeonBlur))
	, dialog(context.fonts.Get(Assets::FontID::Main), context.fonts.Get(Assets::FontID::Menu),
		context.textures.Get(Assets::TextureID::UiFrameWarning),
		context.shaders.Get(Assets::ShaderID::NeonDilate), context.shaders.Get(Assets::ShaderID::NeonBlur),
		context.audioPlayer)
{
	resetLabel.SetText(context.localization.GetText(TextKey::Records::Reset));
	backLabel.SetText(context.localization.GetText(TextKey::Records::Back));

	// Lay the two buttons out from their actual (localized) ink width instead
	// of a fixed distance apart -- "Reset"/"Back" and "Сбросить"/"Назад" don't
	// take up the same room.
	const float resetWidth = resetLabel.GetInkSize().x;
	const float backWidth = backLabel.GetInkSize().x;
	const float pairWidth = resetWidth + ButtonGap + backWidth;
	resetCenter = { ButtonRowCenterX - pairWidth * 0.5f + resetWidth * 0.5f, ButtonRowY };
	backCenter = { ButtonRowCenterX + pairWidth * 0.5f - backWidth * 0.5f, ButtonRowY };

	rule.setSize({ RuleWidth, RuleThickness });
	rule.setPosition({ RuleX, RuleY });

	BuildHeader();
	RefreshRows();
}

void RecordsScreen::BuildHeader()
{
	headerCells.clear();
	const sf::Font& bodyFont = context.fonts.Get(Assets::FontID::Main);

	const auto add = [&](const sf::String& string, float x, int align)
	{
		sf::Text text(bodyFont, string, HeaderSize);
		text.setLetterSpacing(HeaderLetterSpacing);
		text.setFillColor(HeaderColor);
		PlaceCell(text, x, HeaderY, align);
		headerCells.push_back({ std::move(text), HeaderColor });
	};

	add(sf::String("#"), ColRankRight, +1);
	add(context.localization.GetText(TextKey::Records::HeaderName), ColNameLeft, -1);
	add(context.localization.GetText(TextKey::Records::HeaderScore), ColScoreRight, +1);
	add(context.localization.GetText(TextKey::Records::HeaderLines), ColLinesRight, +1);
	add(context.localization.GetText(TextKey::Records::HeaderLevel), ColLevelRight, +1);
}

void RecordsScreen::RefreshRows()
{
	rowCells.clear();
	const sf::Font& bodyFont = context.fonts.Get(Assets::FontID::Main);
	const std::vector<HighScoreEntry>& records = context.highScores.GetRecords();
	const sf::Color scoreColor = UI::MixToWhite(accent, ScoreColorMixFraction);

	const auto add = [&](const sf::String& string, float x, float y, int align, unsigned int size, sf::Color color)
	{
		sf::Text text(bodyFont, string, size);
		text.setFillColor(color);
		PlaceCell(text, x, y, align);
		rowCells.push_back({ std::move(text), color });
	};

	for (std::size_t rank = 0; rank < HighScoreManager::MaxRecords; ++rank)
	{
		const float y = RowTopY + static_cast<float>(rank) * RowStep;
		const bool present = rank < records.size();

		add(std::to_string(rank + 1) + ".", ColRankRight, y, +1, RankSize, RankColor);

		if (!present)
		{
			add(sf::String("-"), ColNameLeft, y, -1, NameSize, EmptyColor);
			continue;
		}

		const HighScoreEntry& entry = records[rank];
		add(entry.playerName, ColNameLeft, y, -1, NameSize, rank == 0 ? ChampionColor : NameColor);
		add(std::to_string(entry.score), ColScoreRight, y, +1, ScoreSize, scoreColor);
		add(std::to_string(entry.lines), ColLinesRight, y, +1, SubSize, SubColor);
		add(std::to_string(entry.level), ColLevelRight, y, +1, SubSize, SubColor);
	}
}

void RecordsScreen::PlayIntro()
{
	introTime = 0.f;
}

void RecordsScreen::StartExit()
{
	if (exitTime < 0.f)
	{
		exitTime = 0.f;
	}
}

bool RecordsScreen::IsExitFinished() const
{
	return exitTime >= ExitDuration;
}

std::optional<sf::Color> RecordsScreen::GetLightbarColor() const
{
	return accent;
}

float RecordsScreen::GetPanelAlpha() const
{
	if (exitTime >= 0.f)
	{
		return std::clamp(1.f - exitTime / ExitDuration, 0.f, 1.f);
	}
	return std::clamp(introTime / IntroDuration, 0.f, 1.f);
}

void RecordsScreen::Leave()
{
	if (isLeaving)
	{
		return;
	}

	isLeaving = true;
	context.audioPlayer.Play(Assets::SoundID::MenuItemPressed);
	host.BeginBack();
}

void RecordsScreen::Activate()
{
	pressTime = 0.f;
	context.audioPlayer.Play(Assets::SoundID::MenuItemPressed);

	if (focus == Focus::Reset)
	{
		dialog.Show(context.localization.GetText(TextKey::Records::ConfirmReset),
			context.localization.GetText(TextKey::Common::Yes),
			context.localization.GetText(TextKey::Common::No));
	}
	else
	{
		Leave();
	}
}

void RecordsScreen::DoReset()
{
	context.highScores.Clear();
	context.highScores.Save();
	RefreshRows();
	context.audioPlayer.Play(Assets::SoundID::MenuItemSelected, ResetConfirmPitch);
}

void RecordsScreen::HandleEvent(const sf::Event& event)
{
	if (isLeaving)
	{
		return;
	}

	if (dialog.IsOpen())
	{
		dialog.Navigate(MenuInput::ResolveAction(event, context.gamepad));

		if (const auto* moved = event.getIf<sf::Event::MouseMoved>())
		{
			dialog.PointerMoved(context.window.mapPixelToCoords(moved->position));
		}
		else if (const auto* pressed = event.getIf<sf::Event::MouseButtonPressed>())
		{
			if (pressed->button == sf::Mouse::Button::Left)
			{
				dialog.PointerPressed(context.window.mapPixelToCoords(pressed->position));
			}
		}

		return;
	}

	switch (MenuInput::ResolveAction(event, context.gamepad))
	{
	case MenuInput::Action::Left:
	case MenuInput::Action::Right:
		focus = focus == Focus::Reset ? Focus::Back : Focus::Reset;
		context.audioPlayer.Restart(Assets::SoundID::MenuItemSelected);
		return;
	case MenuInput::Action::Confirm:
		Activate();
		return;
	case MenuInput::Action::Back:
		Leave();
		return;
	default:
		break;
	}

	if (const auto* moved = event.getIf<sf::Event::MouseMoved>())
	{
		const sf::Vector2f point = context.window.mapPixelToCoords(moved->position);
		if (resetLabel.GetBounds(resetCenter, 1.f).contains(point))
		{
			focus = Focus::Reset;
		}
		else if (backLabel.GetBounds(backCenter, 1.f).contains(point))
		{
			focus = Focus::Back;
		}
	}
	else if (const auto* clicked = event.getIf<sf::Event::MouseButtonPressed>())
	{
		if (clicked->button != sf::Mouse::Button::Left)
		{
			return;
		}

		const sf::Vector2f point = context.window.mapPixelToCoords(clicked->position);
		if (resetLabel.GetBounds(resetCenter, 1.f).contains(point))
		{
			focus = Focus::Reset;
			Activate();
		}
		else if (backLabel.GetBounds(backCenter, 1.f).contains(point))
		{
			focus = Focus::Back;
			Activate();
		}
	}
}

void RecordsScreen::Update(float deltaTime)
{
	introTime += deltaTime;
	pressTime += deltaTime;
	if (exitTime >= 0.f)
	{
		exitTime += deltaTime;
	}

	const bool interactive = !dialog.IsOpen() && !isLeaving;
	resetLabel.SetWaveEnabled(interactive && focus == Focus::Reset);
	backLabel.SetWaveEnabled(interactive && focus == Focus::Back);
	resetLabel.Update(deltaTime);
	backLabel.Update(deltaTime);
	buttonGlow.Update(deltaTime);

	dialog.Update(deltaTime);
	if (const std::optional<bool> answer = dialog.TakeResult(); answer.has_value())
	{
		if (*answer)
		{
			DoReset();
		}
	}
}

void RecordsScreen::DrawButton(sf::RenderTarget& target, UI::MenuLabel& label, sf::Vector2f center,
	sf::Color hue, bool isSelected, float alpha)
{
	const float press = (isSelected && pressTime < PressDuration)
		? std::sin(std::clamp(1.f - pressTime / PressDuration, 0.f, 1.f) * Pi)
		: 0.f;
	const float scale = (isSelected ? SelectedScale : 1.f) + PressPunch * press;
	const float drawAlpha = alpha * (isSelected ? 1.f : UnselectedAlpha);

	if (isSelected)
	{
		const auto glowAlpha = static_cast<std::uint8_t>(
			std::clamp(alpha, 0.f, 1.f) * 255.f * ButtonGlowIntensity);
		label.DrawGlow(target, buttonGlow, center, scale, sf::Color(hue.r, hue.g, hue.b, glowAlpha));
	}

	label.Draw(target, center, scale, hue, drawAlpha, PressFlash * press);
}

void RecordsScreen::Render(sf::RenderTarget& target)
{
	const float alpha = GetPanelAlpha();

	panel.SetColor(sf::Color(255, 255, 255, static_cast<std::uint8_t>(alpha * 255.f)));
	panel.Draw(target);

	if (alpha > 0.f)
	{
		for (Cell& cell : headerCells)
		{
			cell.text.setFillColor(Faded(cell.base, alpha));
			target.draw(cell.text);
		}

		rule.setFillColor(Faded(RuleColor, alpha));
		target.draw(rule);

		for (Cell& cell : rowCells)
		{
			cell.text.setFillColor(Faded(cell.base, alpha));
			target.draw(cell.text);
		}
	}

	DrawButton(target, resetLabel, resetCenter, ResetHue, !dialog.IsOpen() && focus == Focus::Reset, alpha);
	DrawButton(target, backLabel, backCenter, BackHue, !dialog.IsOpen() && focus == Focus::Back, alpha);

	dialog.Render(target);
}
