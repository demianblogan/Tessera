#include "CreditsScreen.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <cstdint>
#include <string_view>

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Mouse.hpp>

#include "../../audio/AudioPlayer.h"
#include "../../core/Context.h"
#include "../../input/MenuInput.h"
#include "../../localization/LocalizationManager.h"
#include "../../localization/TextKeys.h"
#include "../../resources/Assets.h"
#include "../ColorUtils.h"
#include "../TextLayout.h"
#include "../../states/ScreenHost.h"

namespace
{
	constexpr sf::FloatRect PanelBounds{ { 260.f, 210.f }, { 1400.f, 710.f } };
	constexpr unsigned int PanelSourceBorder = MenuFrameSourceBorder;
	constexpr sf::Vector2f PanelTargetBorder{ 44.f, 44.f };

	constexpr float LineSidePadding = 140.f;
	constexpr float LineMaxWidth = PanelBounds.size.x - LineSidePadding;
	constexpr unsigned int LineMinFontSize = 18u;
	constexpr float CenterX = PanelBounds.position.x + PanelBounds.size.x * 0.5f;

	constexpr unsigned int ButtonTextSize = 40;
	constexpr float ButtonCenterY = 968.f;   // below the frame

	constexpr float IntroDuration = 0.24f;
	constexpr float ExitDuration = 0.18f;

	constexpr float Pi = std::numbers::pi_v<float>;
	constexpr float PressDuration = 0.18f;
	constexpr float PressPunch = 0.12f;
	constexpr float PressFlash = 0.55f;
	constexpr float ButtonGlowIntensity = 0.5f;

	// PaletteColor()'s "accent-bright" tint: how far the accent color is mixed
	// toward white.
	constexpr float AccentBrightMixFactor = 0.35f;

	constexpr sf::Color DimTextColor{ 160, 170, 184 };
	constexpr sf::Color BodyTextColor{ 228, 234, 242 };

	enum class Palette { Body = 0, Dim = 1, Accent = 2, AccentBright = 3 };

	struct LineSpec
	{
		std::string_view key;
		unsigned int size;
		float y;
		Palette palette;
	};

	// The blurb is one multi-line key (\n in en.txt); every other row is a single
	// line. y is the vertical center of the row, palette: 0 body, 1 dim, 2 accent,
	// 3 accent-bright.
	constexpr std::array<LineSpec, 9> Lines{ {
		{ TextKey::Credits::Intro,       42u, 282.f, Palette::AccentBright },
		{ TextKey::Credits::Blurb,       32u, 378.f, Palette::Body },
		{ TextKey::Credits::Email,       32u, 496.f, Palette::Accent },
		{ TextKey::Credits::LinkedIn,    32u, 542.f, Palette::Accent },
		{ TextKey::Credits::Instagram,   32u, 588.f, Palette::Accent },
		{ TextKey::Credits::Code,        32u, 634.f, Palette::Accent },
		{ TextKey::Credits::Portfolio,   32u, 680.f, Palette::Accent },
		{ TextKey::Credits::Programming, 32u, 726.f, Palette::Accent },
		{ TextKey::Credits::Gaming,      32u, 772.f, Palette::Accent },
	} };

	[[nodiscard]] sf::Color PaletteColor(Palette palette, sf::Color accent)
	{
		switch (palette)
		{
		case Palette::Dim:          return DimTextColor;
		case Palette::Accent:       return accent;
		case Palette::AccentBright: return UI::MixToWhite(accent, AccentBrightMixFactor);
		default:                    return BodyTextColor;
		}
	}
}

CreditsScreen::CreditsScreen(ScreenHost& host, sf::Color accent)
	: MenuScreen(host)
	, accent(accent)
	, panel(context.textures.Get(Assets::TextureID::UiFrameBrown), PanelBounds, PanelSourceBorder, PanelTargetBorder)
	, backLabel(context.fonts.Get(Assets::FontID::Menu), ButtonTextSize)
	, backGlow(context.shaders.Get(Assets::ShaderID::NeonDilate), context.shaders.Get(Assets::ShaderID::NeonBlur))
{
	const sf::Font& bodyFont = context.fonts.Get(Assets::FontID::Main);

	lines.reserve(Lines.size());
	for (const LineSpec& spec : Lines)
	{
		sf::Text text(bodyFont, context.localization.GetText(spec.key), spec.size);
		UI::TextLayout::FitWidth(text, LineMaxWidth, LineMinFontSize);
		UI::TextLayout::CenterOrigin(text);
		text.setPosition({ CenterX, spec.y });

		lines.push_back({ std::move(text), PaletteColor(spec.palette, accent) });
	}

	backLabel.SetText(context.localization.GetText(TextKey::Credits::Back));
}

void CreditsScreen::PlayIntro()
{
	introTime = 0.f;
}

void CreditsScreen::StartExit()
{
	if (exitTime < 0.f)
	{
		exitTime = 0.f;
	}
}

bool CreditsScreen::IsExitFinished() const
{
	return exitTime >= ExitDuration;
}

std::optional<sf::Color> CreditsScreen::GetLightbarColor() const
{
	return accent;
}

float CreditsScreen::GetPanelAlpha() const
{
	if (exitTime >= 0.f)
	{
		return std::clamp(1.f - exitTime / ExitDuration, 0.f, 1.f);
	}
	return std::clamp(introTime / IntroDuration, 0.f, 1.f);
}

void CreditsScreen::Leave()
{
	if (isLeaving)
	{
		return;
	}

	isLeaving = true;
	pressTime = 0.f;
	context.audioPlayer.Play(Assets::SoundID::MenuItemPressed);
	host.BeginBack();
}

void CreditsScreen::HandleEvent(const sf::Event& event)
{
	if (isLeaving)
	{
		return;
	}

	switch (MenuInput::ResolveAction(event, context.gamepad))
	{
	case MenuInput::Action::Back:
	case MenuInput::Action::Confirm:
		Leave();
		return;
	default:
		break;
	}

	if (const auto* clicked = event.getIf<sf::Event::MouseButtonPressed>())
	{
		if (clicked->button == sf::Mouse::Button::Left
			&& backLabel.GetBounds({ CenterX, ButtonCenterY }, 1.f).contains(context.window.mapPixelToCoords(clicked->position)))
		{
			Leave();
		}
	}
}

void CreditsScreen::Update(float deltaTime)
{
	introTime += deltaTime;
	pressTime += deltaTime;
	if (exitTime >= 0.f)
	{
		exitTime += deltaTime;
	}

	backLabel.Update(deltaTime);
	backGlow.Update(deltaTime);
}

void CreditsScreen::Render(sf::RenderTarget& target)
{
	const float alphaFraction = GetPanelAlpha();
	const auto alpha = static_cast<std::uint8_t>(alphaFraction * 255.f);

	panel.SetColor(sf::Color(255, 255, 255, alpha));
	panel.Draw(target);

	for (Line& line : lines)
	{
		line.text.setFillColor(sf::Color(line.color.r, line.color.g, line.color.b, alpha));
		target.draw(line.text);
	}

	// The "Back to Main Menu" button: main-menu-entry styling, always lit, a
	// punch and flash when pressed.
	const float press = pressTime < PressDuration
		? std::sin((1.f - pressTime / PressDuration) * Pi)
		: 0.f;
	const float buttonScale = 1.f + PressPunch * press;
	const sf::Color glowTint = UI::ScaleRgb(sf::Color::White, ButtonGlowIntensity * alphaFraction);

	backLabel.DrawGlow(target, backGlow, { CenterX, ButtonCenterY }, buttonScale, glowTint);
	backLabel.Draw(target, { CenterX, ButtonCenterY }, buttonScale, sf::Color::White, alphaFraction, PressFlash * press);
}
