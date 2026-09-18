#include "KickData.h"

namespace
{
	using Table8 = std::array<KickData::Tests, KickData::SlotCount>;

	// Slot order: 0->R, 0->L, R->2, R->0, 2->L, 2->R, L->0, L->2
	// (slot = fromState * 2, plus 1 for a counter-clockwise turn).
	//
	// The classic SRS tables are written with +y up; these are already flipped to
	// board space (+y down).

	constexpr Table8 DefaultJLSTZ =
	{ {
		{ { { 0, 0 }, { -1, 0 }, { -1, -1 }, { 0, 2 }, { -1, 2 } } },   // 0->R
		{ { { 0, 0 }, { 1, 0 }, { 1, -1 }, { 0, 2 }, { 1, 2 } } },      // 0->L
		{ { { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, -2 }, { 1, -2 } } },     // R->2
		{ { { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, -2 }, { 1, -2 } } },     // R->0
		{ { { 0, 0 }, { 1, 0 }, { 1, -1 }, { 0, 2 }, { 1, 2 } } },      // 2->L
		{ { { 0, 0 }, { -1, 0 }, { -1, -1 }, { 0, 2 }, { -1, 2 } } },   // 2->R
		{ { { 0, 0 }, { -1, 0 }, { -1, 1 }, { 0, -2 }, { -1, -2 } } },  // L->0
		{ { { 0, 0 }, { -1, 0 }, { -1, 1 }, { 0, -2 }, { -1, -2 } } },  // L->2
	} };

	constexpr Table8 DefaultI =
	{ {
		{ { { 0, 0 }, { -2, 0 }, { 1, 0 }, { -2, 1 }, { 1, -2 } } },    // 0->R
		{ { { 0, 0 }, { -1, 0 }, { 2, 0 }, { -1, -2 }, { 2, 1 } } },    // 0->L
		{ { { 0, 0 }, { -1, 0 }, { 2, 0 }, { -1, -2 }, { 2, 1 } } },    // R->2
		{ { { 0, 0 }, { 2, 0 }, { -1, 0 }, { 2, -1 }, { -1, 2 } } },    // R->0
		{ { { 0, 0 }, { 2, 0 }, { -1, 0 }, { 2, -1 }, { -1, 2 } } },    // 2->L
		{ { { 0, 0 }, { 1, 0 }, { -2, 0 }, { 1, 2 }, { -2, -1 } } },    // 2->R
		{ { { 0, 0 }, { 1, 0 }, { -2, 0 }, { 1, 2 }, { -2, -1 } } },    // L->0
		{ { { 0, 0 }, { -2, 0 }, { 1, 0 }, { -2, 1 }, { 1, -2 } } },    // L->2
	} };

	constexpr KickData::Tests NoKick =
	{ {
		{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }
	} };

	struct Tables
	{
		Table8 jlstz = DefaultJLSTZ;
		Table8 i = DefaultI;
	};

	// Mutable so SetSlot() (the JSON loader) and ResetToDefaults() can override
	// it at runtime; everything else in this file only reads it.
	Tables liveTables;

	[[nodiscard]] bool UsesITable(Tetromino::Type type)
	{
		return type == Tetromino::Type::I;
	}
}

std::optional<int> KickData::GetSlotFor(int fromRotation, int toRotation)
{
	using TetrominoShapes::RotationCount;

	const int from = ((fromRotation % RotationCount) + RotationCount) % RotationCount;
	const int to = ((toRotation % RotationCount) + RotationCount) % RotationCount;

	if (to == (from + 1) % RotationCount)
		return from * SlotsPerRotationState;         // clockwise
	if (to == (from + RotationCount - 1) % RotationCount)
		return from * SlotsPerRotationState + 1;     // counter-clockwise

	return std::nullopt;
}

const KickData::Tests& KickData::GetOffsets(Tetromino::Type type, int fromRotation, int toRotation)
{
	if (type == Tetromino::Type::O)
		return NoKick;

	const std::optional<int> slot = GetSlotFor(fromRotation, toRotation);
	if (!slot.has_value())
		return NoKick;

	return UsesITable(type) ? liveTables.i[*slot] : liveTables.jlstz[*slot];
}

void KickData::SetSlot(Table table, int slot, const Tests& tests)
{
	if (slot < 0 || slot >= SlotCount)
		return;

	if (table == Table::I)
		liveTables.i[slot] = tests;
	else
		liveTables.jlstz[slot] = tests;
}

void KickData::ResetToDefaults()
{
	liveTables = Tables{};
}
