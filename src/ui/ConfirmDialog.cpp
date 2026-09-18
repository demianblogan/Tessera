#include "ConfirmDialog.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <cstdint>

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Transform.hpp>

#include "../audio/AudioPlayer.h"
#include "../display/DisplaySettings.h"
#include "../resources/Assets.h"
#include "../utils/Easing.h"
#include "TextLayout.h"

namespace
{
	constexpr sf::Vector2f Center{ 960.f, 540.f };
	constexpr sf::Vector2f BoxSize{ 880.f, 380.f };
	constexpr sf::FloatRect BoxBounds{
		{ Center.x - BoxSize.x * 0.5f, Center.y - BoxSize.y * 0.5f }, BoxSize };
	constexpr sf::Vector2f FrameTargetBorder{ 40.f, 40.f };

	constexpr unsigned int MessageSize = 40;
	constexpr unsigned int ButtonSize = 40;

	constexpr float MessageY = Center.y - 62.f;
	constexpr float ButtonY = Center.y + 100.f;
	constexpr float ButtonSpacing = 196.f;

	constexpr sf::Vector2f NoCenter{ Center.x - ButtonSpacing, ButtonY };
	constexpr sf::Vector2f YesCenter{ Center.x + ButtonSpacing, ButtonY };

	// The box slides in from above: fully off the top at appear 0, home at 1.
	constexpr float EntryDrop = 780.f;
	constexpr float AppearSpeed = 1.f / 0.16f;

	// A choice: the chosen button flashes, then the box slides back up and off
	// the top, and only then is the answer handed to the caller.
	constexpr float PressHold = 0.16f;
	constexpr float HideDuration = 0.16f;
	constexpr float PressPunch = 0.13f;
	constexpr float PressFlash = 0.6f;
	constexpr float SelectedScale = 1.06f;
	constexpr float UnselectedAlpha = 0.5f;
	constexpr float DimAlpha = 175.f;
	constexpr float Pi = std::numbers::pi_v<float>;

	const sf::Color BoxFillColor{ 12, 11, 16, 225 };
	const sf::Color MessageColor{ 244, 234, 210 };
	const sf::Color YesHue{ 70, 200, 110 };   // matches the Options "Apply" green
	const sf::Color NoHue{ 240, 70, 78 };     // the menu red

	using Easing::EaseInCubic;
	using Easing::EaseOutCubic;
	using Easing::Lerp;
	using Easing::SmoothStep;

	[[nodiscard]] std::uint8_t ToAlpha(float value)
	{
		return static_cast<std::uint8_t>(std::clamp(value, 0.f, 1.f) * 255.f);
	}
}

namespace UI
{
	ConfirmDialog::ConfirmDialog(const sf::Font& messageFont, const sf::Font& buttonFont,
		const sf::Texture& frameTexture, sf::Shader& neonDilate, sf::Shader& neonBlur, AudioPlayer& audio)
		: messageText(messageFont, "", MessageSize)
		, frame(frameTexture, BoxBounds, MenuFrameSourceBorder, FrameTargetBorder)
		, yesLabel(buttonFont, ButtonSize)
		, noLabel(buttonFont, ButtonSize)
		, glow(neonDilate, neonBlur)
		, audio(audio)
	{
		messageText.setFillColor(MessageColor);
	}

	void ConfirmDialog::Show(const sf::String& message, const sf::String& yesText, const sf::String& noText)
	{
		messageText.setString(message);
		UI::TextLayout::CenterOrigin(messageText);

		yesLabel.SetText(yesText);
		noLabel.SetText(noText);

		phase = Phase::Open;
		isYesSelected = false;   // default to "No" -- the safe answer
		result.reset();
		appear = 0.f;
		resolveTime = 0.f;
	}

	bool ConfirmDialog::IsOpen() const
	{
		return phase != Phase::Closed;
	}

	std::optional<bool> ConfirmDialog::TakeResult()
	{
		const std::optional<bool> value = result;
		result.reset();
		return value;
	}

	void ConfirmDialog::Choose(bool isYesAnswer)
	{
		isYesChosen = isYesAnswer;
		isYesSelected = isYesAnswer;   // show the chosen button as selected while it flashes
		phase = Phase::Resolving;
		resolveTime = 0.f;
		audio.Play(Assets::SoundID::MenuItemPressed);
	}

	void ConfirmDialog::Navigate(MenuInput::Action action)
	{
		if (phase != Phase::Open)
		{
			return;
		}

		switch (action)
		{
		case MenuInput::Action::Left:
		case MenuInput::Action::Right:
			isYesSelected = !isYesSelected;
			audio.Restart(Assets::SoundID::MenuItemSelected);
			break;
		case MenuInput::Action::Confirm:
			Choose(isYesSelected);
			break;
		case MenuInput::Action::Back:
			Choose(false);
			break;
		default:
			break;
		}
	}

	void ConfirmDialog::PointerMoved(sf::Vector2f point)
	{
		if (phase != Phase::Open)
		{
			return;
		}

		const bool overYes = yesLabel.GetBounds(YesCenter, 1.f).contains(point);
		const bool overNo = noLabel.GetBounds(NoCenter, 1.f).contains(point);

		if (overYes && !isYesSelected)
		{
			isYesSelected = true;
			audio.Restart(Assets::SoundID::MenuItemSelected);
		}
		else if (overNo && isYesSelected)
		{
			isYesSelected = false;
			audio.Restart(Assets::SoundID::MenuItemSelected);
		}
	}

	void ConfirmDialog::PointerPressed(sf::Vector2f point)
	{
		if (phase != Phase::Open)
		{
			return;
		}

		if (yesLabel.GetBounds(YesCenter, 1.f).contains(point))
		{
			Choose(true);
		}
		else if (noLabel.GetBounds(NoCenter, 1.f).contains(point))
		{
			Choose(false);
		}
	}

	void ConfirmDialog::Update(float deltaTime)
	{
		if (phase == Phase::Closed)
		{
			return;
		}

		appear = std::min(1.f, appear + deltaTime * AppearSpeed);

		yesLabel.SetWaveEnabled(isYesSelected);
		noLabel.SetWaveEnabled(!isYesSelected);
		yesLabel.Update(deltaTime);
		noLabel.Update(deltaTime);
		glow.Update(deltaTime);

		if (phase == Phase::Resolving)
		{
			resolveTime += deltaTime;
			if (resolveTime >= PressHold + HideDuration)
			{
				result = isYesChosen;
				phase = Phase::Closed;
			}
		}
	}

	void ConfirmDialog::DrawButton(sf::RenderTarget& target, MenuLabel& label, sf::Vector2f center,
		sf::Color hue, bool isSelected, float contentAlpha)
	{
		const float press = (phase == Phase::Resolving && isSelected)
			? std::sin(std::clamp(resolveTime / PressHold, 0.f, 1.f) * Pi)
			: 0.f;

		const float scale = (isSelected ? SelectedScale : 1.f) + PressPunch * press;
		const float alpha = contentAlpha * (isSelected ? 1.f : UnselectedAlpha);

		if (isSelected)
		{
			const sf::Color glowTint(hue.r, hue.g, hue.b, ToAlpha(contentAlpha));
			label.DrawGlow(target, glow, center, scale, glowTint);
		}

		label.Draw(target, center, scale, hue, alpha, PressFlash * press);
	}

	void ConfirmDialog::Render(sf::RenderTarget& target)
	{
		if (phase == Phase::Closed)
		{
			return;
		}

		const float hide = (phase == Phase::Resolving)
			? std::clamp((resolveTime - PressHold) / HideDuration, 0.f, 1.f)
			: 0.f;
		const float leaving = EaseInCubic(hide);

		const float slideY = hide > 0.f
			? Lerp(0.f, -EntryDrop, leaving)
			: Lerp(-EntryDrop, 0.f, EaseOutCubic(appear));

		sf::RectangleShape dim(Display::VirtualSize);
		dim.setFillColor(sf::Color(0, 0, 0,
			static_cast<std::uint8_t>(SmoothStep(appear) * (1.f - leaving) * DimAlpha)));
		target.draw(dim);

		// A dark fill behind the frame so the message stays legible whatever the
		// frame texture's center does.
		sf::RectangleShape fill(BoxSize);
		fill.setOrigin(BoxSize * 0.5f);
		fill.setPosition({ Center.x, Center.y + slideY });
		fill.setFillColor(BoxFillColor);
		target.draw(fill);

		sf::Transform slide;
		slide.translate({ 0.f, slideY });
		frame.SetColor(sf::Color::White);
		frame.Draw(target, sf::RenderStates(slide));

		messageText.setPosition({ Center.x, MessageY + slideY });
		target.draw(messageText);

		DrawButton(target, noLabel, { NoCenter.x, NoCenter.y + slideY }, NoHue, !isYesSelected, 1.f);
		DrawButton(target, yesLabel, { YesCenter.x, YesCenter.y + slideY }, YesHue, isYesSelected, 1.f);
	}
}
