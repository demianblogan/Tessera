#include "GamepadPrompts.h"

#include <array>

namespace
{
	// The measured top-left is exact; the size gets +1 in each axis so the icon
	// is not clipped on its right / bottom edge.
	[[nodiscard]] constexpr sf::IntRect Sprite(int x, int y, int w, int h)
	{
		return sf::IntRect{ { x, y }, { w + 1, h + 1 } };
	}

	// Xbox atlas.
	constexpr sf::IntRect XFaceButton = Sprite(49, 48, 13, 15);       // A -- hard drop
	constexpr sf::IntRect XTopFaceButton = Sprite(49, 64, 13, 15);    // Y -- hold
	constexpr sf::IntRect XMenuButton = Sprite(49, 112, 13, 14);      // Menu -- pause
	constexpr sf::IntRect XDpadLeft = Sprite(130, 99, 11, 7);
	constexpr sf::IntRect XDpadRight = Sprite(146, 99, 11, 7);
	constexpr sf::IntRect XDpadDown = Sprite(148, 82, 7, 11);
	constexpr sf::IntRect XLeftBumper = Sprite(336, 49, 15, 13);
	constexpr sf::IntRect XRightBumper = Sprite(336, 65, 15, 13);

	// PlayStation atlas (a different layout, its own coordinates).
	constexpr sf::IntRect PFaceButton = Sprite(49, 64, 13, 15);       // Cross -- hard drop
	constexpr sf::IntRect PTopFaceButton = Sprite(49, 32, 13, 15);    // Triangle -- hold
	constexpr sf::IntRect PStartButton = Sprite(340, 51, 7, 10);      // Options/Start glyph -- pause
	constexpr sf::IntRect PDpadLeft = Sprite(370, 198, 12, 10);
	constexpr sf::IntRect PDpadRight = Sprite(369, 166, 12, 10);
	constexpr sf::IntRect PDpadDown = Sprite(372, 177, 7, 14);
	constexpr sf::IntRect PLeftBumper = Sprite(96, 228, 15, 7);
	constexpr sf::IntRect PRightBumper = Sprite(96, 224, 15, 7);

	using Action = GamepadPrompts::Action;
	constexpr std::size_t ActionCount = 8;

	[[nodiscard]] constexpr std::size_t Index(Action action) noexcept
	{
		return static_cast<std::size_t>(action);
	}

	const std::array<sf::IntRect, ActionCount> XboxIcons = []
	{
		std::array<sf::IntRect, ActionCount> icons{};
		icons[Index(Action::MoveLeft)] = XDpadLeft;
		icons[Index(Action::MoveRight)] = XDpadRight;
		icons[Index(Action::SoftDrop)] = XDpadDown;
		icons[Index(Action::HardDrop)] = XFaceButton;
		icons[Index(Action::RotateClockwise)] = XRightBumper;
		icons[Index(Action::RotateCounterClockwise)] = XLeftBumper;
		icons[Index(Action::Hold)] = XTopFaceButton;
		icons[Index(Action::Pause)] = XMenuButton;
		return icons;
	}();

	const std::array<sf::IntRect, ActionCount> PlayStationIcons = []
	{
		std::array<sf::IntRect, ActionCount> icons{};
		icons[Index(Action::MoveLeft)] = PDpadLeft;
		icons[Index(Action::MoveRight)] = PDpadRight;
		icons[Index(Action::SoftDrop)] = PDpadDown;
		icons[Index(Action::HardDrop)] = PFaceButton;
		icons[Index(Action::RotateClockwise)] = PRightBumper;
		icons[Index(Action::RotateCounterClockwise)] = PLeftBumper;
		icons[Index(Action::Hold)] = PTopFaceButton;
		icons[Index(Action::Pause)] = PStartButton;
		return icons;
	}();
}

Assets::TextureID GamepadPrompts::AtlasFor(GamepadManager::Layout layout)
{
	return layout == GamepadManager::Layout::PlayStation
		? Assets::TextureID::PlayStationGamepadLayout
		: Assets::TextureID::XboxGamepadLayout;
}

sf::IntRect GamepadPrompts::IconFor(GamepadManager::Layout layout, Action action)
{
	const auto& table = layout == GamepadManager::Layout::PlayStation ? PlayStationIcons : XboxIcons;
	return table[Index(action)];
}
