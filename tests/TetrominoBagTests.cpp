#include "doctest/doctest.h"

#include <algorithm>
#include <array>
#include <vector>

#include "gameplay/TetrominoBag.h"

namespace
{
	std::array<int, 7> DrawSevenCounts(TetrominoBag& bag)
	{
		std::array<int, 7> counts{};

		for (int i = 0; i < 7; i++)
		{
			counts[static_cast<std::size_t>(bag.Next())]++;
		}

		return counts;
	}
}

TEST_CASE("each bag of seven draws contains every piece exactly once")
{
	TetrominoBag bag;

	const std::array<int, 7> counts = DrawSevenCounts(bag);

	for (int count : counts)
	{
		CHECK(count == 1);
	}
}

TEST_CASE("the bag keeps refilling: three bags, each a full set")
{
	TetrominoBag bag;

	for (int round = 0; round < 3; round++)
	{
		const std::array<int, 7> counts = DrawSevenCounts(bag);

		CAPTURE(round);
		for (int count : counts)
		{
			CHECK(count == 1);
		}
	}
}

TEST_CASE("a piece never repeats before the bag is exhausted")
{
	TetrominoBag bag;

	std::vector<Tetromino::Type> seen;

	for (int i = 0; i < 7; i++)
	{
		const Tetromino::Type piece = bag.Next();
		CHECK(std::find(seen.begin(), seen.end(), piece) == seen.end());
		seen.push_back(piece);
	}
}
