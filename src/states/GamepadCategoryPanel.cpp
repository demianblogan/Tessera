#include "GamepadCategoryPanel.h"

#include <algorithm>
#include <array>
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
#include "../input/MenuInput.h"
#include "../localization/LocalizationManager.h"
#include "../localization/TextKeys.h"
#include "../resources/Assets.h"
#include "../ui/ColourUtils.h"

namespace
{
	constexpr sf::FloatRect PanelBounds{ { 560.f, 190.f }, { 1320.f, 812.f } };
	constexpr unsigned int PanelSourceBorder = 28u;
	constexpr sf::Vector2f PanelTargetBorder{ 44.f, 44.f };

	constexpr float LabelInset = 118.f;
	constexpr float HeaderY = PanelBounds.position.y + 84.f;
	constexpr float RowsTop = PanelBounds.position.y + 146.f;
	constexpr float RowHeight = 74.f;
	constexpr float XboxColumnX = PanelBounds.position.x + PanelBounds.size.x - 470.f;
	constexpr float PlayStationColumnX = PanelBounds.position.x + PanelBounds.size.x - 190.f;
	constexpr float IconScale = 3.4f;

	constexpr unsigned int LabelSize = 34u;
	constexpr unsigned int HeaderSize = 30u;
	constexpr unsigned int BackSize = 46u;
	constexpr sf::Vector2f BackBoxPadding{ 84.f, 52.f };

	constexpr float FadeSpeed = 9.f;

	// Sprite rects, identical in both atlases.
	constexpr sf::IntRect FaceButtonA{ { 49, 48 }, { 13, 15 } };
	constexpr sf::IntRect OptionsButton{ { 50, 114 }, { 11, 13 } };
	constexpr sf::IntRect DpadLeft{ { 130, 99 }, { 11, 7 } };
	constexpr sf::IntRect DpadRight{ { 146, 99 }, { 11, 7 } };
	constexpr sf::IntRect DpadDown{ { 148, 82 }, { 7, 11 } };
	constexpr sf::IntRect LeftBumper{ { 336, 49 }, { 15, 13 } };
	constexpr sf::IntRect RightBumper{ { 336, 65 }, { 15, 13 } };

	const sf::Color LabelColour{ 214, 220, 230 };

	[[nodiscard]] std::uint8_t Fade(float alpha, float extra = 1.f)
	{
		return static_cast<std::uint8_t>(std::clamp(alpha * extra, 0.f, 1.f) * 255.f);
	}

	void CentreText(sf::Text& text, sf::Vector2f centre)
	{
		const sf::FloatRect bounds = text.getLocalBounds();
		text.setOrigin({ bounds.position.x + bounds.size.x * 0.5f, bounds.position.y + bounds.size.y * 0.5f });
		text.setPosition(centre);
	}
}

GamepadCategoryPanel::GamepadCategoryPanel(Context& context, sf::Color accent)
	: context(context)
	, accent(accent)
	, panelBounds(PanelBounds)
	, frame(context.textures.Get(Assets::TextureID::UiFrame), PanelBounds, PanelSourceBorder, PanelTargetBorder)
	, xboxHeader(context.fonts.Get(Assets::FontID::Main),
		context.localization.GetText(TextKey::Options::GamepadXbox), HeaderSize)
	, playStationHeader(context.fonts.Get(Assets::FontID::Main),
		context.localization.GetText(TextKey::Options::GamepadPlayStation), HeaderSize)
	, backButton(context.fonts.Get(Assets::FontID::Main), BackSize)
{
	const LocalizationManager& text = context.localization;
	const sf::Font& font = context.fonts.Get(Assets::FontID::Main);

	struct Def { std::string_view key; sf::IntRect sprite; };
	const std::array<Def, 7> defs{ {
		{ TextKey::Options::KeyMoveLeft,  DpadLeft },
		{ TextKey::Options::KeyMoveRight, DpadRight },
		{ TextKey::Options::KeySoftDrop,  DpadDown },
		{ TextKey::Options::KeyHardDrop,  FaceButtonA },
		{ TextKey::Options::KeyRotateCw,  RightBumper },
		{ TextKey::Options::KeyRotateCcw, LeftBumper },
		{ TextKey::Options::KeyPause,     OptionsButton } } };

	rows.reserve(defs.size());
	for (const Def& def : defs)
	{
		sf::Text label(font, text.GetText(def.key), LabelSize);
		label.setFillColor(LabelColour);
		rows.push_back(Row{ std::move(label), def.sprite, def.sprite });
	}

	backButton.SetText(text.GetText(TextKey::Options::BackButton));
	backButton.SetWaveEnabled(false);

	LayOut();
}

void GamepadCategoryPanel::LayOut()
{
	for (std::size_t i = 0; i < rows.size(); ++i)
	{
		const float centreY = RowsTop + static_cast<float>(i) * RowHeight + RowHeight * 0.5f;
		const sf::FloatRect bounds = rows[i].label.getLocalBounds();
		rows[i].label.setOrigin({ bounds.position.x, bounds.position.y + bounds.size.y * 0.5f });
		rows[i].label.setPosition({ panelBounds.position.x + LabelInset, centreY });
	}

	CentreText(xboxHeader, { XboxColumnX, HeaderY });
	CentreText(playStationHeader, { PlayStationColumnX, HeaderY });

	backCentre = { panelBounds.position.x + panelBounds.size.x * 0.5f,
		panelBounds.position.y + panelBounds.size.y - 64.f };
	const sf::Vector2f ink = backButton.InkSize();
	const sf::Vector2f box{ ink.x + BackBoxPadding.x, ink.y + BackBoxPadding.y };
	backBox = { { backCentre.x - box.x * 0.5f, backCentre.y - box.y * 0.5f }, box };
}

void GamepadCategoryPanel::Open()
{
	closeRequested = false;
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

void GamepadCategoryPanel::Update(float deltaTime)
{
	alpha += (targetAlpha - alpha) * std::min(1.f, deltaTime * FadeSpeed);
	backButton.Update(deltaTime);
}

bool GamepadCategoryPanel::HandleEvent(const sf::Event& event)
{
	if (!active)
	{
		return false;
	}

	switch (MenuInput::Resolve(event, context.gamepad))
	{
	case MenuInput::Action::Confirm:
	case MenuInput::Action::Back:
		closeRequested = true;
		context.audioPlayer.Play(Assets::SoundID::MenuItemSelected, 0.78f);
		return true;
	default:
		break;
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
	xboxHeader.setFillColor(sf::Color(headerColour.r, headerColour.g, headerColour.b, a));
	playStationHeader.setFillColor(sf::Color(headerColour.r, headerColour.g, headerColour.b, a));
	target.draw(xboxHeader);
	target.draw(playStationHeader);

	// A faint divider between the two icon columns.
	{
		const float dividerX = (XboxColumnX + PlayStationColumnX) * 0.5f;
		const float top = HeaderY - 6.f;
		const float bottom = RowsTop + static_cast<float>(rows.size()) * RowHeight - RowHeight * 0.25f;
		sf::RectangleShape divider({ 2.f, bottom - top });
		divider.setPosition({ dividerX, top });
		divider.setFillColor(sf::Color(accent.r, accent.g, accent.b, Fade(alpha, 0.28f)));
		target.draw(divider);
	}

	const sf::Texture& xboxAtlas = context.textures.Get(Assets::TextureID::XboxGamepadLayout);
	const sf::Texture& psAtlas = context.textures.Get(Assets::TextureID::PlayStationGamepadLayout);

	const auto drawIcon = [&](const sf::Texture& atlas, sf::IntRect rect, sf::Vector2f centre)
	{
		sf::Sprite sprite(atlas);
		sprite.setTextureRect(rect);
		sprite.setOrigin(sf::Vector2f(rect.size) * 0.5f);
		sprite.setScale({ IconScale, IconScale });
		sprite.setPosition(centre);
		sprite.setColor(sf::Color(255, 255, 255, a));
		target.draw(sprite);
	};

	for (std::size_t i = 0; i < rows.size(); ++i)
	{
		const float centreY = RowsTop + static_cast<float>(i) * RowHeight + RowHeight * 0.5f;

		rows[i].label.setFillColor(sf::Color(LabelColour.r, LabelColour.g, LabelColour.b, a));
		target.draw(rows[i].label);

		drawIcon(xboxAtlas, rows[i].xbox, { XboxColumnX, centreY });
		drawIcon(psAtlas, rows[i].playStation, { PlayStationColumnX, centreY });
	}

	// The lone Back button, focused look borrowed from the settings panels.
	for (int band = 3; band >= 1; --band)
	{
		const float inflate = static_cast<float>(band) * 5.f;
		sf::RectangleShape halo({ backBox.size.x + 2.f * inflate, backBox.size.y + 2.f * inflate });
		halo.setOrigin(halo.getSize() * 0.5f);
		halo.setPosition(backCentre);
		halo.setFillColor(sf::Color::Transparent);
		halo.setOutlineThickness(3.f);
		halo.setOutlineColor(sf::Color(accent.r, accent.g, accent.b,
			static_cast<std::uint8_t>(frac * (70.f - static_cast<float>(band) * 16.f))));
		target.draw(halo);
	}

	backButton.Draw(target, backCentre, 1.04f, UI::MixToWhite(sf::Color(236, 240, 246), 0.15f), alpha);
}
