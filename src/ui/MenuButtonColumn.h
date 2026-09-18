#pragma once

#include <cstddef>
#include <functional>
#include <optional>
#include <vector>

#include <SFML/Graphics/Color.hpp>
#include <SFML/System/String.hpp>
#include <SFML/System/Vector2.hpp>

#include "MenuLabel.h"
#include "../primitives/NeonGlow.h"

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
	// center of the screen and settle into their left column; PlayExit() drops
	// them back down. The selected button carries a neon bloom.
	//
	// SetCompact() shrinks and dims every button except one -- the state the
	// screen uses once a category's content panel is open.
	class MenuButtonColumn
	{
	public:
		MenuButtonColumn(const sf::Font& font, unsigned int characterSize,
			sf::Shader& dilateShader, sf::Shader& blurShader);

		// `color` overrides the default hue of an enabled button (white).
		void AddButton(const sf::String& text, std::function<void()> onActivate, bool isEnabled = true,
			std::optional<sf::Color> color = std::nullopt);
		void SetLayout(sf::Vector2f topLeft, float rowGap);

		// Re-labels an existing button in place (a language switch) without
		// touching its callback, enabled state or position.
		void SetButtonText(std::size_t index, const sf::String& text);

		// Fired with the new index whenever the selection moves (keyboard, pad or
		// hover) -- the screen uses it to swap the preview panel. `direction` is
		// -1 / +1 (up-to-down convention: down/right is +1, up/left is -1), so a
		// listener can pitch its nav sound the same way the main-menu ring does.
		void SetSelectionChangedCallback(std::function<void(std::size_t, int)> callback);

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
		void SetRenderShift(sf::Vector2f shift);
		void SetRenderDim(float dim);

		// When false, no button draws its selected state (glow / brightening /
		// idle wave). The Options screen turns it off for the Controls column
		// while it is only a hover preview, so nothing looks focused until the
		// player actually steps into that sub-menu.
		void SetSelectionHighlight(bool isSelectionHighlightEnabled);
		[[nodiscard]] bool IsIntroDone() const;
		[[nodiscard]] bool IsExitDone() const;

		void SelectPrevious();
		void SelectNext();
		void Activate();

		void PointerMoved(sf::Vector2f point);
		enum class PointerHit { None, Hovered, Activated };
		PointerHit PointerPressed(sf::Vector2f point);

		[[nodiscard]] std::size_t GetSelectedIndex() const;
		[[nodiscard]] std::size_t GetButtonCount() const;

		// Resting on-screen center / ink height of one entry (render shift
		// included). For a header that rises from a specific entry.
		[[nodiscard]] sf::Vector2f GetEntryCenter(std::size_t index) const;
		[[nodiscard]] float GetEntryHeight(std::size_t index) const;

		void SetCompact(bool isCompact, std::size_t activeIndex);

		void Update(float deltaTime);
		void Render(sf::RenderTarget& target) const;

	private:
		struct Button
		{
			MenuLabel label;
			std::function<void()> activate;
			bool isEnabled = true;
			sf::Color color{ sf::Color::White };   // used only when enabled
			sf::Vector2f restCenter;
		};

		struct Pose { sf::Vector2f center; float scale = 1.f; float alpha = 1.f; };
		[[nodiscard]] Pose GetPose(std::size_t index) const;

		void MoveSelection(int direction);
		[[nodiscard]] bool IsAnyEnabled() const;

		mutable NeonGlow glow;

		const sf::Font& font;
		unsigned int characterSize;

		static constexpr sf::Vector2f DefaultTopLeft{ 240.f, 300.f };
		static constexpr float DefaultRowGap = 96.f;

		std::vector<Button> buttons;
		sf::Vector2f topLeft = DefaultTopLeft;
		float rowGap = DefaultRowGap;

		sf::Vector2f renderShift{ 0.f, 0.f };
		float renderDim = 1.f;
		bool isSelectionHighlightEnabled = true;

		std::size_t selectedIndex = 0;
		std::function<void(std::size_t, int)> onSelectionChanged;
		std::function<void(std::size_t)> onSwoosh;
		std::vector<char> swooshFired;

		bool hasStarted = false;
		float introTime = 0.f;
		float exitTime = -1.f;

		// Seconds since Activate() was last called; kept large until then, which
		// Render() reads as "no press flash in progress".
		static constexpr float NoPressSentinel = 1000.f;
		float pressTime = NoPressSentinel;

		float animTime = 0.f;      // glow breath

		bool isCompact = false;
		std::size_t compactActive = 0;
		float compactFraction = 0.f;      // 0 full, 1 compact
	};
}
