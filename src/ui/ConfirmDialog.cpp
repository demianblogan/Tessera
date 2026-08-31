#include "ConfirmDialog.h"

#include <algorithm>
#include <cstdint>

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

#include "TextLayout.h"

namespace
{
	constexpr sf::Vector2f Centre{ 960.f, 540.f };
	constexpr sf::Vector2f BoxSize{ 820.f, 340.f };
	constexpr unsigned int MessageSize = 42;
	constexpr unsigned int ButtonSize = 36;

	constexpr float ButtonSpacing = 170.f;
	constexpr float ButtonY = Centre.y + 88.f;
	constexpr sf::Vector2f ButtonPadding{ 78.f, 30.f };

	const sf::Color BoxFill{ 14, 16, 22 };
	const sf::Color BoxOutline{ 120, 210, 255 };
	const sf::Color ChosenText{ 255, 255, 255 };
	const sf::Color IdleText{ 150, 156, 166 };

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
	ConfirmDialog::ConfirmDialog(const sf::Font& font)
		: messageText(font, "", MessageSize)
		, yesText(font, "", ButtonSize)
		, noText(font, "", ButtonSize)
	{
	}

	void ConfirmDialog::Show(const sf::String& message, const sf::String& yesLabel, const sf::String& noLabel)
	{
		messageText.setString(message);
		UI::TextLayout::CentreOrigin(messageText);
		messageText.setPosition({ Centre.x, Centre.y - 50.f });

		yesText.setString(yesLabel);
		noText.setString(noLabel);

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
		if (open)
		{
			appear = std::min(1.f, appear + deltaTime / 0.18f);
		}
	}

	void ConfirmDialog::DrawButton(sf::RenderTarget& target, sf::Text& text, sf::Vector2f centre, bool chosen) const
	{
		const sf::FloatRect bounds = text.getLocalBounds();
		text.setOrigin({ bounds.position.x + bounds.size.x * 0.5f, bounds.position.y + bounds.size.y * 0.5f });
		text.setPosition(centre);

		sf::RectangleShape frame({ bounds.size.x + ButtonPadding.x, static_cast<float>(ButtonSize) + ButtonPadding.y });
		frame.setOrigin(frame.getSize() * 0.5f);
		frame.setPosition(centre);
		frame.setFillColor(chosen ? sf::Color(BoxOutline.r, BoxOutline.g, BoxOutline.b, 45) : sf::Color(255, 255, 255, 12));
		frame.setOutlineThickness(2.f);
		frame.setOutlineColor(chosen ? BoxOutline : sf::Color(255, 255, 255, 60));
		target.draw(frame);

		text.setFillColor(chosen ? ChosenText : IdleText);
		target.draw(text);
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

		DrawButton(target, noText, { Centre.x - ButtonSpacing, ButtonY }, !yesSelected);
		DrawButton(target, yesText, { Centre.x + ButtonSpacing, ButtonY }, yesSelected);
	}
}
