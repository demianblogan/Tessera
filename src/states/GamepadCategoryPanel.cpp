#include "GamepadCategoryPanel.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string_view>
#include <utility>

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Mouse.hpp>

#include "../audio/AudioPlayer.h"
#include "../core/Context.h"
#include "../input/GamepadPrompts.h"
#include "../input/MenuInput.h"
#include "../localization/LocalizationManager.h"
#include "../localization/TextKeys.h"
#include "../resources/Assets.h"
#include "../ui/ColourUtils.h"
#include "ControlsPanelMetrics.h"

namespace
{
	constexpr sf::FloatRect PanelBounds = ControlsPanel::Bounds;
	constexpr unsigned int PanelSourceBorder = UI::MenuFrameSourceBorder;
	constexpr sf::Vector2f PanelTargetBorder{ 44.f, 44.f };

	constexpr float LabelX = ControlsPanel::LabelX;
	constexpr float RowsTop = ControlsPanel::RowsTop;
	constexpr float RowHeight = ControlsPanel::RowHeight;   // same list layout as the Keyboard panel
	constexpr float RowPitch = ControlsPanel::RowPitch;
	constexpr float HeaderY = RowsTop - 30.f;
	constexpr float PlayStationColumnX = PanelBounds.position.x + PanelBounds.size.x - 220.f;
	constexpr float XboxColumnX = PlayStationColumnX - 310.f;
	constexpr float IconScale = 3.0f;
	constexpr sf::Vector2f IconBox{ 76.f, 62.f };   // taller and narrower, so no icon overflows it

	constexpr unsigned int LabelSize = ControlsPanel::LabelSize;   // match the Keyboard screen
	constexpr unsigned int HeaderSize = ControlsPanel::LabelSize;  // same size -> crisp pixel font
	constexpr unsigned int BackSize = 46u;
	constexpr sf::Vector2f BackBoxPadding{ 84.f, 52.f };

	constexpr float FadeSpeed = 9.f;
	constexpr float HighlightSpeed = 12.f;

	const sf::Color LabelIdle{ 206, 213, 224 };
	const sf::Color LabelHot{ 255, 255, 255 };
	const sf::Color KeycapFill{ 28, 32, 40 };
	const sf::Color KeycapEdge{ 120, 130, 145 };

	[[nodiscard]] std::uint8_t Fade(float alpha, float extra = 1.f)
	{
		return static_cast<std::uint8_t>(std::clamp(alpha * extra, 0.f, 1.f) * 255.f);
	}

	[[nodiscard]] sf::Color WithAlpha(sf::Color colour, std::uint8_t a)
	{
		return { colour.r, colour.g, colour.b, a };
	}

	void CentreText(sf::Text& text, sf::Vector2f centre)
	{
		const sf::FloatRect bounds = text.getLocalBounds();
		text.setOrigin({ bounds.position.x + bounds.size.x * 0.5f, bounds.position.y + bounds.size.y * 0.5f });
		text.setPosition(centre);
	}

	using Prompt = GamepadPrompts::Action;
	struct ActionDef { std::string_view key; Prompt action; };
	const std::array<ActionDef, 7> ActionDefs{ {
		{ TextKey::Options::KeyMoveLeft,  Prompt::MoveLeft },
		{ TextKey::Options::KeyMoveRight, Prompt::MoveRight },
		{ TextKey::Options::KeySoftDrop,  Prompt::SoftDrop },
		{ TextKey::Options::KeyHardDrop,  Prompt::HardDrop },
		{ TextKey::Options::KeyRotateCw,  Prompt::RotateClockwise },
		{ TextKey::Options::KeyRotateCcw, Prompt::RotateCounterClockwise },
		{ TextKey::Options::KeyHold,      Prompt::Hold } } };
}

GamepadCategoryPanel::GamepadCategoryPanel(Context& context, sf::Color accent)
	: context(context)
	, accent(accent)
	, panelBounds(PanelBounds)
	, frame(context.textures.Get(Assets::TextureID::UiFramePurple), PanelBounds, PanelSourceBorder, PanelTargetBorder)
	, xboxHeader(context.fonts.Get(Assets::FontID::Main),
		context.localization.GetText(TextKey::Options::GamepadXbox), HeaderSize)
	, playStationHeader(context.fonts.Get(Assets::FontID::Main),
		context.localization.GetText(TextKey::Options::GamepadPlayStation), HeaderSize)
	, backButton(context.fonts.Get(Assets::FontID::Main), BackSize)
{
	const LocalizationManager& text = context.localization;
	const sf::Font& font = context.fonts.Get(Assets::FontID::Main);

	rows.reserve(ActionDefs.size());
	for (const ActionDef& def : ActionDefs)
	{
		sf::Text label(font, text.GetText(def.key), LabelSize);
		const sf::IntRect xbox = GamepadPrompts::IconFor(GamepadManager::Layout::Xbox, def.action);
		const sf::IntRect playStation = GamepadPrompts::IconFor(GamepadManager::Layout::PlayStation, def.action);
		rows.push_back(Row{ std::move(label), xbox, playStation, 0.f });
	}

	backButton.SetText(text.GetText(TextKey::Options::BackButton));
	backButton.SetWaveEnabled(false);

	LayOut();
}

void GamepadCategoryPanel::LayOut()
{
	for (std::size_t i = 0; i < rows.size(); ++i)
	{
		const float centreY = RowsTop + static_cast<float>(i) * RowPitch + RowHeight * 0.5f;
		const sf::FloatRect bounds = rows[i].label.getLocalBounds();
		rows[i].label.setOrigin({ bounds.position.x, bounds.position.y + bounds.size.y * 0.5f });
		rows[i].label.setPosition({ LabelX, centreY });
	}

	CentreText(xboxHeader, { XboxColumnX, HeaderY });
	CentreText(playStationHeader, { PlayStationColumnX, HeaderY });

	// Keep the headers inside the frame if the pixel font renders them wide.
	const float leftLimit = panelBounds.position.x + 54.f;
	const float rightLimit = panelBounds.position.x + panelBounds.size.x - 54.f;
	const auto clampInside = [&](sf::Text& text)
	{
		const sf::FloatRect box = text.getGlobalBounds();
		if (box.position.x < leftLimit)
		{
			text.move({ leftLimit - box.position.x, 0.f });
		}
		else if (box.position.x + box.size.x > rightLimit)
		{
			text.move({ rightLimit - (box.position.x + box.size.x), 0.f });
		}
	};
	clampInside(xboxHeader);
	clampInside(playStationHeader);

	// Same height as the Keyboard panel's Apply/Reset/Back row.
	backCentre = { panelBounds.position.x + panelBounds.size.x * 0.5f,
		panelBounds.position.y + panelBounds.size.y - 86.f };
	const sf::Vector2f ink = backButton.InkSize();
	const sf::Vector2f box{ ink.x + BackBoxPadding.x, ink.y + BackBoxPadding.y };
	backBox = { { backCentre.x - box.x * 0.5f, backCentre.y - box.y * 0.5f }, box };
}

void GamepadCategoryPanel::Open()
{
	closeRequested = false;
	focus = Focus::Rows;
	selectedRow = 0;
}

void GamepadCategoryPanel::RefreshText()
{
	const LocalizationManager& text = context.localization;

	xboxHeader.setString(text.GetText(TextKey::Options::GamepadXbox));
	playStationHeader.setString(text.GetText(TextKey::Options::GamepadPlayStation));

	for (std::size_t i = 0; i < rows.size() && i < ActionDefs.size(); ++i)
	{
		rows[i].label.setString(text.GetText(ActionDefs[i].key));
	}

	backButton.SetText(text.GetText(TextKey::Options::BackButton));

	LayOut();
}

void GamepadCategoryPanel::Close()
{
	active = false;
	closeRequested = false;
}

void GamepadCategoryPanel::SetVisibility(Visibility visibility, float previewFade)
{
	switch (visibility)
	{
	case Visibility::Open:    targetAlpha = 1.f; break;
	case Visibility::Preview: targetAlpha = 0.55f * std::clamp(previewFade, 0.f, 1.f); break;
	case Visibility::Hidden:  targetAlpha = 0.f; break;
	}

	active = visibility == Visibility::Open;
}

void GamepadCategoryPanel::MoveSelection(int direction)
{
	if (focus == Focus::Rows)
	{
		const int next = static_cast<int>(selectedRow) + direction;
		if (next < 0)
		{
			return;
		}
		if (next >= static_cast<int>(rows.size()))
		{
			focus = Focus::Back;
			context.audioPlayer.Restart(Assets::SoundID::MenuItemSelected, 1.14f);
			return;
		}
		selectedRow = static_cast<std::size_t>(next);
	}
	else if (direction < 0)
	{
		focus = Focus::Rows;
		selectedRow = rows.empty() ? 0 : rows.size() - 1;
	}
	else
	{
		return;
	}

	context.audioPlayer.Restart(Assets::SoundID::MenuItemSelected, direction >= 0 ? 1.14f : 0.9f);
}

void GamepadCategoryPanel::Update(float deltaTime)
{
	alpha += (targetAlpha - alpha) * std::min(1.f, deltaTime * FadeSpeed);
	backButton.Update(deltaTime);

	for (std::size_t i = 0; i < rows.size(); ++i)
	{
		const float target = (active && focus == Focus::Rows && i == selectedRow) ? 1.f : 0.f;
		rows[i].highlight += (target - rows[i].highlight) * std::min(1.f, deltaTime * HighlightSpeed);
	}
}

bool GamepadCategoryPanel::HandleEvent(const sf::Event& event)
{
	if (!active)
	{
		return false;
	}

	switch (MenuInput::Resolve(event, context.gamepad))
	{
	case MenuInput::Action::Up:   MoveSelection(-1); return true;
	case MenuInput::Action::Down: MoveSelection(1);  return true;
	case MenuInput::Action::Confirm:
		if (focus == Focus::Back)
		{
			closeRequested = true;
			context.audioPlayer.Play(Assets::SoundID::MenuItemSelected, 0.78f);
		}
		return true;
	case MenuInput::Action::Back:
		closeRequested = true;
		context.audioPlayer.Play(Assets::SoundID::MenuItemSelected, 0.78f);
		return true;
	default:
		break;
	}

	if (const auto* moved = event.getIf<sf::Event::MouseMoved>())
	{
		const sf::Vector2f point = context.window.mapPixelToCoords(moved->position);
		for (std::size_t i = 0; i < rows.size(); ++i)
		{
			const float centreY = RowsTop + static_cast<float>(i) * RowPitch + RowHeight * 0.5f;
			const sf::FloatRect rowRect{ { panelBounds.position.x + 40.f, centreY - RowHeight * 0.5f },
				{ panelBounds.size.x - 80.f, RowHeight } };
			if (rowRect.contains(point))
			{
				focus = Focus::Rows;
				selectedRow = i;
			}
		}
		if (backBox.contains(point))
		{
			focus = Focus::Back;
		}
		return true;
	}

	if (const auto* clicked = event.getIf<sf::Event::MouseButtonPressed>())
	{
		if (clicked->button == sf::Mouse::Button::Left
			&& backBox.contains(context.window.mapPixelToCoords(clicked->position)))
		{
			closeRequested = true;
			context.audioPlayer.Play(Assets::SoundID::MenuItemSelected, 0.78f);
			return true;
		}
	}

	return false;
}

void GamepadCategoryPanel::Render(sf::RenderTarget& target)
{
	if (alpha <= 0.01f)
	{
		return;
	}

	const std::uint8_t a = Fade(alpha);
	const float frac = std::clamp(alpha, 0.f, 1.f);

	frame.SetColor(sf::Color(255, 255, 255, a));
	frame.Draw(target);

	const sf::Color headerColour = UI::MixToWhite(accent, 0.55f);
	xboxHeader.setFillColor(WithAlpha(headerColour, a));
	playStationHeader.setFillColor(WithAlpha(headerColour, a));
	target.draw(xboxHeader);
	target.draw(playStationHeader);

	const sf::Texture& xboxAtlas = context.textures.Get(Assets::TextureID::XboxGamepadLayout);
	const sf::Texture& psAtlas = context.textures.Get(Assets::TextureID::PlayStationGamepadLayout);

	const auto drawKeycap = [&](sf::Vector2f centre, bool hot)
	{
		sf::RectangleShape box(IconBox);
		box.setOrigin(IconBox * 0.5f);
		box.setPosition(centre);
		box.setFillColor(WithAlpha(KeycapFill, a));
		box.setOutlineThickness(2.f);
		box.setOutlineColor(WithAlpha(hot ? accent : KeycapEdge, a));
		target.draw(box);
	};

	const auto drawIcon = [&](const sf::Texture& atlas, sf::IntRect rect, sf::Vector2f centre)
	{
		sf::Sprite sprite(atlas);
		sprite.setTextureRect(rect);
		sprite.setOrigin(sf::Vector2f(rect.size) * 0.5f);
		sprite.setScale({ IconScale, IconScale });
		sprite.setPosition({ std::round(centre.x), std::round(centre.y) });
		sprite.setColor(sf::Color(255, 255, 255, a));
		target.draw(sprite);
	};

	for (std::size_t i = 0; i < rows.size(); ++i)
	{
		Row& row = rows[i];
		const float centreY = RowsTop + static_cast<float>(i) * RowPitch + RowHeight * 0.5f;
		const float hi = row.highlight;

		if (hi > 0.01f)
		{
			sf::RectangleShape wash({ panelBounds.size.x - 80.f, RowHeight });
			wash.setPosition({ panelBounds.position.x + 40.f, centreY - RowHeight * 0.5f });
			wash.setFillColor(sf::Color(255, 255, 255, Fade(alpha, 0.08f * hi)));
			target.draw(wash);

			sf::RectangleShape bar({ 5.f, RowHeight });
			bar.setPosition({ panelBounds.position.x + 40.f, centreY - RowHeight * 0.5f });
			bar.setFillColor(WithAlpha(accent, Fade(alpha, hi)));
			target.draw(bar);
		}

		const sf::Color labelColour{
			static_cast<std::uint8_t>(LabelIdle.r + (LabelHot.r - LabelIdle.r) * hi),
			static_cast<std::uint8_t>(LabelIdle.g + (LabelHot.g - LabelIdle.g) * hi),
			static_cast<std::uint8_t>(LabelIdle.b + (LabelHot.b - LabelIdle.b) * hi) };
		row.label.setFillColor(WithAlpha(labelColour, a));
		target.draw(row.label);

		const bool hot = hi > 0.5f;
		drawKeycap({ XboxColumnX, centreY }, hot);
		drawKeycap({ PlayStationColumnX, centreY }, hot);
		drawIcon(xboxAtlas, row.xbox, { XboxColumnX, centreY });
		drawIcon(psAtlas, row.playStation, { PlayStationColumnX, centreY });
	}

	// The lone Back button, focused look borrowed from the settings panels.
	const bool backFocused = focus == Focus::Back;
	if (backFocused)
	{
		for (int band = 3; band >= 1; --band)
		{
			const float inflate = static_cast<float>(band) * 5.f;
			sf::RectangleShape halo({ backBox.size.x + 2.f * inflate, backBox.size.y + 2.f * inflate });
			halo.setOrigin(halo.getSize() * 0.5f);
			halo.setPosition(backCentre);
			halo.setFillColor(sf::Color::Transparent);
			halo.setOutlineThickness(3.f);
			halo.setOutlineColor(sf::Color(236, 240, 246,
				static_cast<std::uint8_t>(frac * (70.f - static_cast<float>(band) * 16.f))));
			target.draw(halo);
		}
	}

	backButton.Draw(target, backCentre, backFocused ? 1.04f : 1.f,
		backFocused ? sf::Color(255, 255, 255) : sf::Color(214, 220, 230), alpha);
}
