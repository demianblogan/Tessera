#include "ConfirmDialog.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/Angle.hpp>

#include "TextLayout.h"

namespace
{
	constexpr sf::Vector2f Centre{ 960.f, 540.f };
	constexpr sf::Vector2f BoxSize{ 820.f, 340.f };
	constexpr unsigned int MessageSize = 42;

	constexpr float ButtonBox = 96.f;
	constexpr float ButtonSpacing = 140.f;
	constexpr float ButtonY = Centre.y + 88.f;

	constexpr sf::IntRect TickBg{ { 0, 0 }, { 26, 26 } };
	constexpr sf::IntRect CrossBg{ { 28, 0 }, { 26, 26 } };

	const sf::Color BoxFill{ 14, 16, 22 };
	const sf::Color BoxOutline{ 120, 210, 255 };
	const sf::Color TickColour{ 110, 235, 145 };
	const sf::Color CrossColour{ 255, 110, 110 };

	[[nodiscard]] float EaseOutBack(float t) noexcept
	{
		t = std::clamp(t, 0.f, 1.f);
		constexpr float c = 1.70158f;
		const float inv = t - 1.f;
		return 1.f + (c + 1.f) * inv * inv * inv + c * inv * inv;
	}

	void ThickLine(sf::RenderTarget& target, sf::Vector2f a, sf::Vector2f b, float thickness, sf::Color colour)
	{
		const sf::Vector2f delta = b - a;
		const float length = std::sqrt(delta.x * delta.x + delta.y * delta.y);

		sf::RectangleShape segment({ length, thickness });
		segment.setOrigin({ 0.f, thickness * 0.5f });
		segment.setPosition(a);
		segment.setRotation(sf::radians(std::atan2(delta.y, delta.x)));
		segment.setFillColor(colour);
		target.draw(segment);
	}
}

namespace UI
{
	ConfirmDialog::ConfirmDialog(const sf::Font& messageFont, const sf::Texture& checkboxTexture)
		: checkboxTexture(checkboxTexture)
		, messageText(messageFont, "", MessageSize)
	{
	}

	void ConfirmDialog::Show(const sf::String& message)
	{
		messageText.setString(message);
		UI::TextLayout::CentreOrigin(messageText);
		messageText.setPosition({ Centre.x, Centre.y - 50.f });

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
		const float size = ButtonBox * (chosen ? 1.14f : 1.f);

		if (chosen)
		{
			for (int band = 3; band >= 1; --band)
			{
				const float inflate = static_cast<float>(band) * 5.f;
				sf::RectangleShape halo({ size + 2.f * inflate, size + 2.f * inflate });
				halo.setOrigin(halo.getSize() * 0.5f);
				halo.setPosition(centre);
				halo.setFillColor(sf::Color::Transparent);
				halo.setOutlineThickness(3.f);
				halo.setOutlineColor(sf::Color(BoxOutline.r, BoxOutline.g, BoxOutline.b,
					static_cast<std::uint8_t>(72.f - static_cast<float>(band) * 16.f)));
				target.draw(halo);
			}
		}

		// The checkbox sprite is the button background.
		sf::Sprite background(checkboxTexture);
		background.setTextureRect(yesSide ? TickBg : CrossBg);
		background.setOrigin(sf::Vector2f(TickBg.size) * 0.5f);
		background.setScale({ size / static_cast<float>(TickBg.size.x), size / static_cast<float>(TickBg.size.y) });
		background.setPosition(centre);
		target.draw(background);

		// The tick / cross drawn on top.
		const float r = size * 0.26f;
		const float thickness = size * 0.11f;
		if (yesSide)
		{
			ThickLine(target, { centre.x - r, centre.y + r * 0.1f }, { centre.x - r * 0.25f, centre.y + r * 0.75f },
				thickness, TickColour);
			ThickLine(target, { centre.x - r * 0.25f, centre.y + r * 0.75f }, { centre.x + r, centre.y - r * 0.7f },
				thickness, TickColour);
		}
		else
		{
			ThickLine(target, { centre.x - r * 0.75f, centre.y - r * 0.75f }, { centre.x + r * 0.75f, centre.y + r * 0.75f },
				thickness, CrossColour);
			ThickLine(target, { centre.x + r * 0.75f, centre.y - r * 0.75f }, { centre.x - r * 0.75f, centre.y + r * 0.75f },
				thickness, CrossColour);
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

		DrawButton(target, false, !yesSelected);
		DrawButton(target, true, yesSelected);
	}
}
