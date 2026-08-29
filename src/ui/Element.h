#pragma once

#include <SFML/System/Vector2.hpp>

namespace sf
{
	class RenderTarget;
}

class NeonGlow;

namespace UI
{
	class Element
	{
	public:
		enum class SizeMode
		{
			Auto,
			Pixels,
			Percent,
			Fill
		};

		struct SizeRule
		{
			SizeMode mode = SizeMode::Auto;

			float value = 0.f;
		};

	protected:
		sf::Vector2f position{ 0.f, 0.f };
		sf::Vector2f size{ 0.f, 0.f };

		SizeRule widthRule;
		SizeRule heightRule;

	public:
		virtual ~Element() = default;

		[[nodiscard]] virtual sf::Vector2f Measure() const = 0;
		virtual void Arrange(sf::Vector2f position, sf::Vector2f size);

		// Render with an optional neon edge-glow for the selected element.
		virtual void Render(sf::RenderTarget& target, NeonGlow* glow) const;
		virtual void Render(sf::RenderTarget& target) const = 0;

		[[nodiscard]] sf::Vector2f GetPosition() const;
		[[nodiscard]] sf::Vector2f GetSize() const;

		void SetWidthAuto();
		void SetHeightAuto();

		void SetWidthPixels(float width);
		void SetHeightPixels(float height);

		void SetWidthPercent(float percent);
		void SetHeightPercent(float percent);

		void SetWidthFill();
		void SetHeightFill();

		[[nodiscard]] const SizeRule& GetWidthRule() const;
		[[nodiscard]] const SizeRule& GetHeightRule() const;
	};
}