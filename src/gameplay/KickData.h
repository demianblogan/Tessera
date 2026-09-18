#pragma once

#include <array>
#include <optional>
#include <string_view>

#include <SFML/System/Vector2.hpp>

#include "Tetromino.h"

// Super Rotation System wall-kick data: when a rotation lands the piece on a
// wall, floor or the stack, the game tries a short ordered list of offsets and
// takes the first that fits. There are two tables -- one shared by J, L, S, T
// and Z, one for I -- keyed by which adjacent rotation state the piece is
// turning from and to. O never kicks.
//
// Offsets are in board space (+x right, +y down) and the first is always (0, 0)
// (rotate in place). Built-in values are the standard SRS tables; assets/data/
// srs_kicks.json can override a transition (see KickDataFile.h).
namespace KickData
{
	inline constexpr int TestCount = 5;

	using Tests = std::array<sf::Vector2i, TestCount>;

	// One transition slot per turn direction out of each rotation state --
	// see GetSlotFor for how a from->to step maps to one of these.
	inline constexpr int SlotsPerRotationState = 2;
	inline constexpr int SlotCount = TetrominoShapes::RotationCount * SlotsPerRotationState;

	// The offsets to try, in order, rotating `type` from `fromRotation` to
	// `toRotation` (both 0..3, and one step apart). O returns all zeroes.
	[[nodiscard]] const Tests& GetOffsets(Tetromino::Type type, int fromRotation, int toRotation);

	// Which of the two override tables a transition slot belongs to.
	enum class Table { JLSTZ, I };

	// Transition slot for a from->to step, 0..SlotCount-1 (from * SlotsPerRotationState,
	// +1 when turning counter-clockwise). std::nullopt if the states are not one step apart.
	[[nodiscard]] std::optional<int> GetSlotFor(int fromRotation, int toRotation);

	// Replace one transition slot in one table (used by the JSON loader).
	void SetSlot(Table table, int slot, const Tests& tests);

	// Restore both tables to the built-in SRS values.
	void ResetToDefaults();
}
