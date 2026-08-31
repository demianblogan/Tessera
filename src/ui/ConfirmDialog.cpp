#include "ConfirmDialog.h"

#include <algorithm>
#include <cstdint>

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Texture.hpp>

#include "NineSliceFrame.h"
#include "TextLayout.h"

namespace
{
	constexpr sf::Vector2f Centre{ 960.f, 540.f };
	constexpr sf::Vector2f BoxSize{ 760.f, 320.f };
	constexpr unsigned int MessageSize = 42;

	constexpr float ButtonBox = 78.f;         // framed button, on-screen
	constexpr float ButtonSpacing = 120.f;    // from centre to each button
	constexpr float ButtonY = Centre.y + 82.f;
	constexpr float IconSize = 40.f;

	constexpr sf::IntRect TickRect{ { 0, 0 }, { 26, 26 } };
	constexpr sf::IntRect CrossRect{ { 28, 0 }, { 26, 26 } };

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
	ConfirmDialog::ConfirmDialog(const sf::Font& messageFont, const sf::Texture& iconTexture,
		const sf::Texture& frameTexture)
		: iconTexture(iconTexture)
		, frameTexture(frameTexture)
		, messageText(messageFont, "", MessageSize)
	{
	}

	void ConfirmDialog::Show(const sf::String& message)
	{
		messageText.setString(message);
		UI::TextLayout::CentreOrigin(messageText);
		messageText.setPosition({ Centre.x, Centre.y - 48.f });

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

	void ConfirmDialog::DrawButton(sf::RenderTarget& target, bool yesSide, bool chosen) const
	{
		const sf::Vector2f centre{ Centre.x + (yesSide ? ButtonSpacing : -ButtonSpacing), ButtonY };
		const float box = ButtonBox * (chosen ? 1.12f : 1.f);

		const sf::FloatRect bounds{ { centre.x - box * 0.5f, centre.y - box * 0.5f }, { box, box } };

		if (chosen)
		{
			sf::RectangleShape halo({ box + 26.f, box + 26.f });
			halo.setOrigin(halo.getSize() * 0.5f);
			halo.setPosition(centre);
			halo.setFillColor(sf::Color(BoxOutline.r, BoxOutline.g, BoxOutline.b, 40));
			target.draw(halo);
		}

		NineSliceFrame frame(frameTexture, bounds, 16u, { 18.f, 18.f });
		frame.SetColor(sf::Color::White);
		frame.Draw(target);

		sf::Sprite icon(iconTexture);
		icon.setTextureRect(yesSide ? TickRect : CrossRect);
		icon.setOrigin(sf::Vector2f(TickRect.size) * 0.5f);
		const float iconScale = IconSize / static_cast<float>(TickRect.size.x) * (chosen ? 1.12f : 1.f);
		icon.setScale({ iconScale, iconScale });
		icon.setPosition(centre);
		icon.setColor(chosen
			? (yesSide ? sf::Color(120, 240, 150) : sf::Color(255, 120, 120))
			: sf::Color(150, 156, 166));
		target.draw(icon);
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

		DrawButton(target, false, !yesSelected);
		DrawButton(target, true, yesSelected);
	}
}
