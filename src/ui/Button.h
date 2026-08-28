#pragma once

#include <memory>
#include <optional>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Shader.hpp>
#include <SFML/Graphics/Sprite.hpp>

#include "Label.h"

namespace UI
{
	class Button final : public Element
	{
	public:
		enum class TextAlignment
		{
			Left,
			Center,
			Right
		};

		struct Padding
		{
			float left = 0.f;
			float top = 0.f;
			float right = 0.f;
			float bottom = 0.f;
		};

		struct Style
		{
			std::optional<sf::Color> backgroundColor;
			std::optional<sf::Color> textColor;
			std::optional<sf::Color> outlineColor;
			std::optional<float> outlineThickness;
		};

	private:
		std::unique_ptr<Label> label;

		Padding padding;
		TextAlignment textAlignment = TextAlignment::Center;
		bool selected = false;
		bool enabled = true;

		Style normalStyle;
		Style selectedStyle;
		Style disabledStyle;

		sf::Color backgroundColor = sf::Color::White;
		sf::Color outlineColor = sf::Color::Transparent;
		float outlineThickness = 0.f;

		std::optional<sf::Sprite> backgroundSprite;

		sf::Vector2f preferredSize = { 0.f, 0.f };

		void ApplyStyle(const Style& style);
		void UpdateVisualState();

	public:
		Button() = default;
		explicit Button(sf::Vector2f size);
		Button(sf::Vector2f size, std::unique_ptr<Label> label);
		Button(const sf::Sprite& sprite);
		Button(const sf::Sprite& sprite, std::unique_ptr<Label> label);

		void SetLabel(std::unique_ptr<Label> label);
		[[nodiscard]] Label* GetLabel();
		[[nodiscard]] const Label* GetLabel() const;

		void SetTextAlignment(TextAlignment alignment);
		void SetPadding(Padding padding);

		void SetSelected(bool selected);
		[[nodiscard]] bool IsSelected() const;

		// A disabled button can't be selected and draws with disabledStyle.
		void SetEnabled(bool enabled);
		[[nodiscard]] bool IsEnabled() const;

		void SetNormalStyle(Style style);
		void SetSelectedStyle(Style style);
		void SetDisabledStyle(Style style);

		void SetPreferredSize(sf::Vector2f size);

		[[nodiscard]] sf::Vector2f Measure() const override;
		void Arrange(sf::Vector2f position, sf::Vector2f size) override;

		[[nodiscard]] bool Contains(sf::Vector2f point) const;

		void Render(sf::RenderTarget& target, sf::Shader* glowShader, float time) const override;
		void Render(sf::RenderTarget& target) const override;
	};
}