#include "GamepadPrompts.h"

#include <array>

namespace
{
	// The measured top-left is exact; the size gets +1 in each axis so the icon
	// is not clipped on its right / bottom edge.
	[[nodiscard]] constexpr sf::IntRect MakeIconRect(int x, int y, int w, int h)
	{
		return sf::IntRect{ { x, y }, { w + 1, h + 1 } };
	}

	// Xbox atlas.
	constexpr sf::IntRect XFaceButton = MakeIconRect(49, 48, 13, 15);     // A -- hard drop
	constexpr sf::IntRect XTopFaceButton = MakeIconRect(49, 64, 13, 15);  // Y -- hold
	constexpr sf::IntRect XMenuButton = MakeIconRect(49, 112, 13, 14);    // Menu -- pause
	constexpr sf::IntRect XDpadLeft = MakeIconRect(130, 99, 11, 7);
	constexpr sf::IntRect XDpadRight = MakeIconRect(146, 99, 11, 7);
	constexpr sf::IntRect XDpadDown = MakeIconRect(148, 82, 7, 11);
	constexpr sf::IntRect XLeftBumper = MakeIconRect(336, 85, 15, 7);
	constexpr sf::IntRect XRightBumper = MakeIconRect(336, 101, 15, 7);

	// PlayStation atlas (a different layout, its own coordinates).
	constexpr sf::IntRect PFaceButton = MakeIconRect(49, 64, 13, 15);     // Cross -- hard drop
	constexpr sf::IntRect PTopFaceButton = MakeIconRect(49, 32, 13, 15);  // Triangle -- hold
	constexpr sf::IntRect PStartButton = MakeIconRect(340, 51, 7, 10);    // Options/Start glyph -- pause
	constexpr sf::IntRect PDpadLeft = MakeIconRect(370, 198, 12, 10);
	constexpr sf::IntRect PDpadRight = MakeIconRect(369, 166, 12, 10);
	constexpr sf::IntRect PDpadDown = MakeIconRect(372, 177, 7, 14);
	constexpr sf::IntRect PLeftBumper = MakeIconRect(96, 228, 15, 7);
	constexpr sf::IntRect PRightBumper = MakeIconRect(96, 244, 15, 7);

	using Action = GamepadPrompts::Action;
	constexpr std::size_t ActionCount = static_cast<std::size_t>(Action::Count);

	[[nodiscard]] constexpr std::size_t ConvertToIndex(Action action) noexcept
	{
		return static_cast<std::size_t>(action);
	}

	const std::array<sf::IntRect, ActionCount> XboxIcons = []
	{
		std::array<sf::IntRect, ActionCount> icons{};
		icons[ConvertToIndex(Action::MoveLeft)] = XDpadLeft;
		icons[ConvertToIndex(Action::MoveRight)] = XDpadRight;
		icons[ConvertToIndex(Action::SoftDrop)] = XDpadDown;
		icons[ConvertToIndex(Action::HardDrop)] = XFaceButton;
		icons[ConvertToIndex(Action::RotateClockwise)] = XRightBumper;
		icons[ConvertToIndex(Action::RotateCounterClockwise)] = XLeftBumper;
		icons[ConvertToIndex(Action::Hold)] = XTopFaceButton;
		icons[ConvertToIndex(Action::Pause)] = XMenuButton;
		return icons;
	}();

	const std::array<sf::IntRect, ActionCount> PlayStationIcons = []
	{
		std::array<sf::IntRect, ActionCount> icons{};
		icons[ConvertToIndex(Action::MoveLeft)] = PDpadLeft;
		icons[ConvertToIndex(Action::MoveRight)] = PDpadRight;
		icons[ConvertToIndex(Action::SoftDrop)] = PDpadDown;
		icons[ConvertToIndex(Action::HardDrop)] = PFaceButton;
		icons[ConvertToIndex(Action::RotateClockwise)] = PRightBumper;
		icons[ConvertToIndex(Action::RotateCounterClockwise)] = PLeftBumper;
		icons[ConvertToIndex(Action::Hold)] = PTopFaceButton;
		icons[ConvertToIndex(Action::Pause)] = PStartButton;
		return icons;
	}();
}

Assets::TextureID GamepadPrompts::GetAtlasFor(GamepadManager::Layout layout)
{
	return layout == GamepadManager::Layout::PlayStation
		? Assets::TextureID::PlayStationGamepadLayout
		: Assets::TextureID::XboxGamepadLayout;
}

sf::IntRect GamepadPrompts::GetIconFor(GamepadManager::Layout layout, Action action)
{
	const auto& table = layout == GamepadManager::Layout::PlayStation ? PlayStationIcons : XboxIcons;
	return table[ConvertToIndex(action)];
}
