#include "KickData.h"

namespace
{
	using Table8 = std::array<KickData::Tests, 8>;

	// Slot order: 0->R, 0->L, R->2, R->0, 2->L, 2->R, L->0, L->2
	// (slot = fromState * 2, plus 1 for a counter-clockwise turn).
	//
	// The classic SRS tables are written with +y up; these are already flipped to
	// board space (+y down).

	constexpr Table8 DefaultJLSTZ = { {
		{ { { 0, 0 }, { -1, 0 }, { -1, -1 }, { 0, 2 }, { -1, 2 } } },   // 0->R
		{ { { 0, 0 }, { 1, 0 }, { 1, -1 }, { 0, 2 }, { 1, 2 } } },      // 0->L
		{ { { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, -2 }, { 1, -2 } } },     // R->2
		{ { { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, -2 }, { 1, -2 } } },     // R->0
		{ { { 0, 0 }, { 1, 0 }, { 1, -1 }, { 0, 2 }, { 1, 2 } } },      // 2->L
		{ { { 0, 0 }, { -1, 0 }, { -1, -1 }, { 0, 2 }, { -1, 2 } } },   // 2->R
		{ { { 0, 0 }, { -1, 0 }, { -1, 1 }, { 0, -2 }, { -1, -2 } } },  // L->0
		{ { { 0, 0 }, { -1, 0 }, { -1, 1 }, { 0, -2 }, { -1, -2 } } },  // L->2
	} };

	constexpr Table8 DefaultI = { {
		{ { { 0, 0 }, { -2, 0 }, { 1, 0 }, { -2, 1 }, { 1, -2 } } },    // 0->R
		{ { { 0, 0 }, { -1, 0 }, { 2, 0 }, { -1, -2 }, { 2, 1 } } },    // 0->L
		{ { { 0, 0 }, { -1, 0 }, { 2, 0 }, { -1, -2 }, { 2, 1 } } },    // R->2
		{ { { 0, 0 }, { 2, 0 }, { -1, 0 }, { 2, -1 }, { -1, 2 } } },    // R->0
		{ { { 0, 0 }, { 2, 0 }, { -1, 0 }, { 2, -1 }, { -1, 2 } } },    // 2->L
		{ { { 0, 0 }, { 1, 0 }, { -2, 0 }, { 1, 2 }, { -2, -1 } } },    // 2->R
		{ { { 0, 0 }, { 1, 0 }, { -2, 0 }, { 1, 2 }, { -2, -1 } } },    // L->0
		{ { { 0, 0 }, { -2, 0 }, { 1, 0 }, { -2, 1 }, { 1, -2 } } },    // L->2
	} };

	constexpr KickData::Tests NoKick = { { { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } } };

	struct Tables
	{
		Table8 jlstz = DefaultJLSTZ;
		Table8 i = DefaultI;
	};

	Tables& Live()
	{
		static Tables tables;
		return tables;
	}

	[[nodiscard]] bool UsesITable(Tetromino::Type type)
	{
		return type == Tetromino::Type::I;
	}
}

std::optional<int> KickData::SlotFor(int fromRotation, int toRotation)
{
	const int from = ((fromRotation % 4) + 4) % 4;
	const int to = ((toRotation % 4) + 4) % 4;

	if (to == (from + 1) % 4)
	{
		return from * 2;         // clockwise
	}
	if (to == (from + 3) % 4)
	{
		return from * 2 + 1;     // counter-clockwise
	}

	return std::nullopt;
}

const KickData::Tests& KickData::Offsets(Tetromino::Type type, int fromRotation, int toRotation)
{
	if (type == Tetromino::Type::O)
	{
		return NoKick;
	}

	const std::optional<int> slot = SlotFor(fromRotation, toRotation);
	if (!slot)
	{
		return NoKick;
	}

	return UsesITable(type) ? Live().i[*slot] : Live().jlstz[*slot];
}

void KickData::SetSlot(Table table, int slot, const Tests& tests)
{
	if (slot < 0 || slot >= 8)
	{
		return;
	}

	if (table == Table::I)
	{
		Live().i[slot] = tests;
	}
	else
	{
		Live().jlstz[slot] = tests;
	}
}

void KickData::ResetToDefaults()
{
	Live() = Tables{};
}
