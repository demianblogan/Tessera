#include "CreditsScreen.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string_view>

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Mouse.hpp>

#include "../audio/AudioPlayer.h"
#include "../core/Context.h"
#include "../input/MenuInput.h"
#include "../localization/LocalizationManager.h"
#include "../localization/TextKeys.h"
#include "../resources/Assets.h"
#include "../ui/ColourUtils.h"
#include "../ui/TextLayout.h"
#include "MenuShell.h"

namespace
{
	constexpr sf::FloatRect PanelBounds{ { 290.f, 175.f }, { 1340.f, 775.f } };
	constexpr unsigned int PanelSourceBorder = 28u;   // of the 96px frame texture
	constexpr sf::Vector2f PanelTargetBorder{ 46.f, 46.f };

	constexpr float LineMaxWidth = PanelBounds.size.x - 170.f;
	constexpr float CentreX = PanelBounds.position.x + PanelBounds.size.x * 0.5f;

	constexpr unsigned int ButtonTextSize = 52;
	constexpr float ButtonCentreY = 1002.f;   // below the frame

	constexpr float IntroDuration = 0.24f;
	constexpr float ExitDuration = 0.18f;

	constexpr float Pi = 3.14159265f;
	constexpr float PressDuration = 0.18f;
	constexpr float PressPunch = 0.12f;
	constexpr float PressFlash = 0.55f;
	constexpr float ButtonGlowIntensity = 0.5f;

	struct LineSpec
	{
		std::string_view key;
		unsigned int size;
		float y;
		int palette;   // 0 body, 1 dim, 2 accent, 3 accent-bright
	};

	constexpr std::array<LineSpec, 11> Lines{ {
		{ TextKey::Credits::Line1,      50u, 285.f, 3 },
		{ TextKey::Credits::Line2,      40u, 350.f, 0 },
		{ TextKey::Credits::Line3,      38u, 400.f, 0 },
		{ TextKey::Credits::Line4,      38u, 448.f, 0 },
		{ TextKey::Credits::Line5,      38u, 496.f, 0 },
		{ TextKey::Credits::Line6,      42u, 556.f, 0 },
		{ TextKey::Credits::Contact,    36u, 636.f, 1 },
		{ TextKey::Credits::Email,      42u, 686.f, 2 },
		{ TextKey::Credits::YouTube,    34u, 742.f, 1 },
		{ TextKey::Credits::Source,     34u, 794.f, 1 },
		{ TextKey::Credits::Repository, 40u, 836.f, 2 },
	} };

	[[nodiscard]] sf::Color PaletteColour(int palette, sf::Color accent)
	{
		switch (palette)
		{
		case 1:  return sf::Color(160, 170, 184);
		case 2:  return accent;
		case 3:  return UI::MixToWhite(accent, 0.35f);
		default: return sf::Color(228, 234, 242);
		}
	}
}

CreditsScreen::CreditsScreen(MenuShell& shell, sf::Color accent)
	: MenuScreen(shell)
	, accent(accent)
	, panel(context.textures.Get(Assets::TextureID::UiFrame), PanelBounds, PanelSourceBorder, PanelTargetBorder)
	, backLabel(context.fonts.Get(Assets::FontID::Menu), ButtonTextSize)
	, backGlow(context.shaders.Get(Assets::ShaderID::NeonDilate), context.shaders.Get(Assets::ShaderID::NeonBlur))
{
	const sf::Font& bodyFont = context.fonts.Get(Assets::FontID::Credits);

	lines.reserve(Lines.size());
	for (const LineSpec& spec : Lines)
	{
		sf::Text text(bodyFont, context.localization.GetText(spec.key), spec.size);
		UI::TextLayout::FitWidth(text, LineMaxWidth, 18u);
		UI::TextLayout::CentreOrigin(text);
		text.setPosition({ CentreX, spec.y });

		lines.push_back({ std::move(text), PaletteColour(spec.palette, accent) });
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

bool CreditsScreen::ExitFinished() const
{
	return exitTime >= ExitDuration;
}

float CreditsScreen::PanelAlpha() const
{
	if (exitTime >= 0.f)
	{
		return std::clamp(1.f - exitTime / ExitDuration, 0.f, 1.f);
	}
	return std::clamp(introTime / IntroDuration, 0.f, 1.f);
}

void CreditsScreen::Leave()
{
	if (leaving)
	{
		return;
	}

	leaving = true;
	pressTime = 0.f;
	context.audioPlayer.Play(Assets::SoundID::MenuItemPressed);
	shell.BeginBack();
}

void CreditsScreen::HandleEvent(const sf::Event& event)
{
	if (leaving)
	{
		return;
	}

	switch (MenuInput::Resolve(event, context.gamepad))
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
			&& backLabel.Bounds({ CentreX, ButtonCentreY }, 1.f).contains(context.window.mapPixelToCoords(clicked->position)))
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
	const float alphaFraction = PanelAlpha();
	const auto alpha = static_cast<std::uint8_t>(alphaFraction * 255.f);

	panel.SetColor(sf::Color(255, 255, 255, alpha));
	panel.Draw(target);

	for (Line& line : lines)
	{
		line.text.setFillColor(sf::Color(line.colour.r, line.colour.g, line.colour.b, alpha));
		target.draw(line.text);
	}

	// The "Back to Main Menu" button: main-menu-entry styling, always lit, a
	// punch and flash when pressed.
	const float press = pressTime < PressDuration
		? std::sin((1.f - pressTime / PressDuration) * Pi)
		: 0.f;
	const float buttonScale = 1.f + PressPunch * press;
	const sf::Color glowTint = UI::ScaleRgb(sf::Color::White, ButtonGlowIntensity * alphaFraction);

	backLabel.DrawGlow(target, backGlow, { CentreX, ButtonCentreY }, buttonScale, glowTint);
	backLabel.Draw(target, { CentreX, ButtonCentreY }, buttonScale, sf::Color::White, alphaFraction, PressFlash * press);
}
