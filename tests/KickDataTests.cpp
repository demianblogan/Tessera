#include "doctest/doctest.h"

#include "gameplay/KickData.h"
#include "gameplay/Tetromino.h"

TEST_CASE("SlotFor maps adjacent rotation steps and rejects the rest")
{
	CHECK(KickData::SlotFor(0, 1) == 0);
	CHECK(KickData::SlotFor(0, 3) == 1);
	CHECK(KickData::SlotFor(1, 2) == 2);
	CHECK(KickData::SlotFor(1, 0) == 3);
	CHECK(KickData::SlotFor(2, 3) == 4);
	CHECK(KickData::SlotFor(2, 1) == 5);
	CHECK(KickData::SlotFor(3, 0) == 6);
	CHECK(KickData::SlotFor(3, 2) == 7);

	CHECK_FALSE(KickData::SlotFor(0, 2).has_value());
	CHECK_FALSE(KickData::SlotFor(1, 1).has_value());
}

TEST_CASE("every kick list starts by rotating in place")
{
	KickData::ResetToDefaults();

	for (int type = 0; type < 7; type++)
	{
		for (int from = 0; from < 4; from++)
		{
			for (const int to : { (from + 1) % 4, (from + 3) % 4 })
			{
				const KickData::Tests& tests =
					KickData::Offsets(static_cast<Tetromino::Type>(type), from, to);
				CHECK(tests[0] == sf::Vector2i{ 0, 0 });
			}
		}
	}
}

TEST_CASE("the O piece never kicks")
{
	KickData::ResetToDefaults();

	for (int from = 0; from < 4; from++)
	{
		for (const sf::Vector2i& offset : KickData::Offsets(Tetromino::Type::O, from, (from + 1) % 4))
		{
			CHECK(offset == sf::Vector2i{ 0, 0 });
		}
	}
}

TEST_CASE("the I piece uses a different kick table from J/L/S/T/Z")
{
	KickData::ResetToDefaults();

	bool anyDifference = false;
	for (int from = 0; from < 4 && !anyDifference; from++)
	{
		const int to = (from + 1) % 4;
		if (KickData::Offsets(Tetromino::Type::I, from, to) != KickData::Offsets(Tetromino::Type::T, from, to))
		{
			anyDifference = true;
		}
	}
	CHECK(anyDifference);
}

TEST_CASE("SetSlot overrides one transition until ResetToDefaults")
{
	KickData::ResetToDefaults();
	const KickData::Tests original = KickData::Offsets(Tetromino::Type::T, 0, 1);

	const KickData::Tests replacement = { {
		{ 0, 0 }, { 5, 5 }, { 5, 5 }, { 5, 5 }, { 5, 5 } } };
	KickData::SetSlot(KickData::Table::JLSTZ, 0, replacement);

	CHECK(KickData::Offsets(Tetromino::Type::T, 0, 1) == replacement);

	KickData::ResetToDefaults();
	CHECK(KickData::Offsets(Tetromino::Type::T, 0, 1) == original);
}
