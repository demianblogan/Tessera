#include "doctest/doctest.h"

#include <algorithm>
#include <set>

#include "gameplay/PieceData.h"
#include "gameplay/Tetromino.h"
#include "gameplay/TetrominoShapes.h"

namespace
{
	std::set<std::pair<int, int>> CellSet(const PieceData::BlockOffsets& offsets)
	{
		std::set<std::pair<int, int>> cells;
		for (const sf::Vector2i& offset : offsets)
		{
			cells.insert({ offset.x, offset.y });
		}
		return cells;
	}
}

TEST_CASE("every built-in piece has four rotation states of four in-range blocks")
{
	PieceData::ResetToDefaults();

	for (int type = 0; type < 7; type++)
	{
		for (int rotation = 0; rotation < PieceData::RotationCount; rotation++)
		{
			const PieceData::BlockOffsets& blocks =
				PieceData::GetBlocks(static_cast<Tetromino::Type>(type), rotation);

			CHECK(CellSet(blocks).size() == 4);

			for (const sf::Vector2i& offset : blocks)
			{
				CHECK(offset.x >= 0);
				CHECK(offset.x < TetrominoShapes::MatrixSize);
				CHECK(offset.y >= 0);
				CHECK(offset.y < TetrominoShapes::MatrixSize);
			}
		}
	}
}

TEST_CASE("Blocks wraps the rotation index into 0..3")
{
	PieceData::ResetToDefaults();

	CHECK(CellSet(PieceData::GetBlocks(Tetromino::Type::L, 4))
		== CellSet(PieceData::GetBlocks(Tetromino::Type::L, 0)));
	CHECK(CellSet(PieceData::GetBlocks(Tetromino::Type::L, -1))
		== CellSet(PieceData::GetBlocks(Tetromino::Type::L, 3)));
}

TEST_CASE("the O piece is identical in all four states")
{
	PieceData::ResetToDefaults();

	const auto spawn = CellSet(PieceData::GetBlocks(Tetromino::Type::O, 0));
	for (int rotation = 1; rotation < 4; rotation++)
	{
		CHECK(CellSet(PieceData::GetBlocks(Tetromino::Type::O, rotation)) == spawn);
	}
}

TEST_CASE("S and Z now have four distinct rotation states")
{
	PieceData::ResetToDefaults();

	for (const Tetromino::Type type : { Tetromino::Type::S, Tetromino::Type::Z })
	{
		std::set<std::set<std::pair<int, int>>> states;
		for (int rotation = 0; rotation < 4; rotation++)
		{
			states.insert(CellSet(PieceData::GetBlocks(type, rotation)));
		}
		CHECK(states.size() == 4);
	}
}

TEST_CASE("ParseState rejects a grid without exactly four blocks")
{
	CHECK_FALSE(PieceData::ParseState({ "....", "....", "....", "...." }).has_value());
	CHECK_FALSE(PieceData::ParseState({ "XXXXX", "....", "....", "...." }).has_value());
	CHECK(PieceData::ParseState({ "XX..", "XX..", "....", "...." }).has_value());
}

TEST_CASE("SetRotations overrides a piece until ResetToDefaults")
{
	PieceData::ResetToDefaults();
	const auto original = CellSet(PieceData::GetBlocks(Tetromino::Type::T, 0));

	PieceData::Rotations flat{};
	const PieceData::BlockOffsets row = { sf::Vector2i{ 0, 0 }, { 1, 0 }, { 2, 0 }, { 3, 0 } };
	flat.fill(row);
	PieceData::SetRotations(Tetromino::Type::T, flat);

	CHECK(CellSet(PieceData::GetBlocks(Tetromino::Type::T, 0))
		== std::set<std::pair<int, int>>{ { 0, 0 }, { 1, 0 }, { 2, 0 }, { 3, 0 } });

	PieceData::ResetToDefaults();
	CHECK(CellSet(PieceData::GetBlocks(Tetromino::Type::T, 0)) == original);
}
