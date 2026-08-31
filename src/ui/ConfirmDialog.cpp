#include "ConfirmDialog.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

#include "ColourUtils.h"
#include "TextLayout.h"

namespace
{
	constexpr sf::Vector2f Centre{ 960.f, 540.f };
	constexpr sf::Vector2f BoxSize{ 780.f, 300.f };
	constexpr unsigned int MessageSize = 34;
	constexpr unsigned int ButtonSize = 44;
	constexpr float ButtonSpacing = 170.f;
	constexpr float ButtonY = Centre.y + 78.f;

	const sf::Color BoxFill{ 14, 16, 22 };
	const sf::Color BoxOutline{ 120, 210, 255 };

	[[nodiscard]] float EaseOutBack(float t) noexcept
	{
		t = std::clamp(t, 0.f, 1.f);
		constexpr float c = 1.70158f;
		const float inv = t - 1.f;
		return 1.f + (c + 1.f) * inv * inv * inv + c * inv * inv;
	}
}

namespace UI
{
	ConfirmDialog::ConfirmDialog(const sf::Font& messageFont, const sf::Font& buttonFont,
		sf::Shader& dilateShader, sf::Shader& blurShader)
		: glow(dilateShader, blurShader)
		, messageText(messageFont, "", MessageSize)
		, yesButton(buttonFont, ButtonSize)
		, noButton(buttonFont, ButtonSize)
	{
	}

	void ConfirmDialog::Show(const sf::String& message, const sf::String& yesLabel, const sf::String& noLabel)
	{
		messageText.setString(message);
		UI::TextLayout::CentreOrigin(messageText);
		messageText.setPosition({ Centre.x, Centre.y - 40.f });

		yesButton.SetText(yesLabel);
		noButton.SetText(noLabel);

		open = true;
		yesSelected = false;   // default to "no" -- the safe answer
		result.reset();
		appear = 0.f;
	}

	std::optional<bool> ConfirmDialog::TakeResult()
	{
		const std::optional<bool> value = result;
		result.reset();
		return value;
	}

	void ConfirmDialog::Resolve(bool answer)
	{
		result = answer;
		open = false;
	}

	void ConfirmDialog::Navigate(MenuInput::Action action)
	{
		if (!open)
		{
			return;
		}

		switch (action)
		{
		case MenuInput::Action::Left:
		case MenuInput::Action::Right:
			yesSelected = !yesSelected;
			break;
		case MenuInput::Action::Confirm:
			Resolve(yesSelected);
			break;
		case MenuInput::Action::Back:
			Resolve(false);
			break;
		default:
			break;
		}
	}

	void ConfirmDialog::Update(float deltaTime)
	{
		glow.Update(deltaTime);
		yesButton.Update(deltaTime);
		noButton.Update(deltaTime);

		if (open)
		{
			appear = std::min(1.f, appear + deltaTime / 0.18f);
		}
	}

	void ConfirmDialog::Render(sf::RenderTarget& target) const
	{
		if (!open)
		{
			return;
		}

		const float pop = EaseOutBack(appear);

		sf::RectangleShape dim({ 1920.f, 1080.f });
		dim.setFillColor(sf::Color(0, 0, 0, static_cast<std::uint8_t>(std::clamp(appear, 0.f, 1.f) * 170.f)));
		target.draw(dim);

		sf::RectangleShape box(BoxSize * pop);
		box.setOrigin(box.getSize() * 0.5f);
		box.setPosition(Centre);
		box.setFillColor(BoxFill);
		box.setOutlineThickness(2.5f);
		box.setOutlineColor(BoxOutline);
		target.draw(box);

		if (appear < 0.6f)
		{
			return;
		}

		target.draw(messageText);

		const sf::Vector2f yesPos{ Centre.x + ButtonSpacing, ButtonY };
		const sf::Vector2f noPos{ Centre.x - ButtonSpacing, ButtonY };

		const auto drawButton = [&](const MenuLabel& label, sf::Vector2f pos, bool chosen)
		{
			if (chosen)
			{
				label.DrawGlow(target, glow, pos, 1.f, UI::ScaleRgb(sf::Color::White, 0.5f));
			}
			label.Draw(target, pos, chosen ? 1.06f : 1.f,
				chosen ? sf::Color::White : sf::Color(150, 156, 166));
		};

		drawButton(noButton, noPos, !yesSelected);
		drawButton(yesButton, yesPos, yesSelected);
	}
}
