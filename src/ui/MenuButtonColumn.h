#pragma once

#include <cstddef>
#include <functional>
#include <optional>
#include <vector>

#include <SFML/Graphics/Color.hpp>
#include <SFML/System/String.hpp>
#include <SFML/System/Vector2.hpp>

#include "MenuLabel.h"
#include "../rendering/NeonGlow.h"

namespace sf
{
	class Font;
	class RenderTarget;
	class Shader;
}

namespace UI
{
	// A left-aligned vertical column of text buttons drawn like the main-menu
	// entries (via MenuLabel). On Begin() the buttons fly up from the bottom
	// centre of the screen and settle into their left column; PlayExit() drops
	// them back down. The selected button carries a neon bloom.
	//
	// SetCompact() shrinks and dims every button except one -- the state the
	// screen uses once a category's content panel is open.
	class MenuButtonColumn
	{
	public:
		MenuButtonColumn(const sf::Font& font, unsigned int characterSize,
			sf::Shader& dilateShader, sf::Shader& blurShader);

		// `colour` overrides the default hue of an enabled button (white).
		void AddButton(const sf::String& text, std::function<void()> onActivate, bool enabled = true,
			std::optional<sf::Color> colour = std::nullopt);
		void SetLayout(sf::Vector2f topLeft, float rowGap);

		// Fired with the new index whenever the selection moves (keyboard, pad or
		// hover) -- the screen uses it to swap the preview panel.
		void SetSelectionChangedCallback(std::function<void(std::size_t)> callback);

		// Fired once per button as it launches into the fly-in.
		void SetSwooshCallback(std::function<void(std::size_t)> callback);

		void Begin();
		// Settle the column immediately in its resting slots, with no fly-in.
		// Used for a secondary column that slides in horizontally instead.
		void AppearInstantly();
		void PlayExit();

		// A render-time translation applied to every button (and its hit box),
		// and an alpha/dim multiplier. The Options screen animates these to slide
		// one column off-screen while another slides in, and to show a dimmed
		// "flyout" preview of the Controls sub-menu on hover.
		void SetRenderShift(sf::Vector2f shift) { renderShift = shift; }
		void SetRenderDim(float dim) { renderDim = dim; }
		[[nodiscard]] bool IsIntroDone() const;
		[[nodiscard]] bool IsExitDone() const;

		void SelectPrevious();
		void SelectNext();
		void Activate();

		void PointerMoved(sf::Vector2f point);
		enum class PointerHit { None, Hovered, Activated };
		PointerHit PointerPressed(sf::Vector2f point);

		[[nodiscard]] std::size_t SelectedIndex() const { return selectedIndex; }

		void SetCompact(bool compact, std::size_t activeIndex);

		void Update(float deltaTime);
		void Render(sf::RenderTarget& target) const;

	private:
		struct Button
		{
			MenuLabel label;
			std::function<void()> activate;
			bool enabled = true;
			sf::Color colour{ sf::Color::White };   // used only when enabled
			sf::Vector2f restCentre;
		};

		struct Pose { sf::Vector2f centre; float scale = 1.f; float alpha = 1.f; };
		[[nodiscard]] Pose PoseOf(std::size_t index) const;

		void MoveSelection(int direction);
		[[nodiscard]] bool AnyEnabled() const;

		mutable NeonGlow glow;

		const sf::Font& font;
		unsigned int characterSize;

		std::vector<Button> buttons;
		sf::Vector2f topLeft{ 240.f, 300.f };
		float rowGap = 96.f;

		sf::Vector2f renderShift{ 0.f, 0.f };
		float renderDim = 1.f;

		std::size_t selectedIndex = 0;
		std::function<void(std::size_t)> onSelectionChanged;
		std::function<void(std::size_t)> onSwoosh;
		std::vector<char> swooshFired;

		bool started = false;
		float introTime = 0.f;
		float exitTime = -1.f;
		float pressTime = 1000.f;
		float animTime = 0.f;      // glow breath

		bool compact = false;
		std::size_t compactActive = 0;
		float compactT = 0.f;      // 0 full, 1 compact
	};
}
