#include "doctest/doctest.h"

#include <algorithm>

#include "gameplay/Tetromino.h"

namespace
{
	bool HasBlock(const Tetromino& piece, sf::Vector2i cell)
	{
		const auto blocks = piece.GetBlockPositions();
		return std::find(blocks.begin(), blocks.end(), cell) != blocks.end();
	}
}

TEST_CASE("a piece always has four blocks")
{
	for (int type = 0; type < 7; type++)
	{
		const Tetromino piece(static_cast<Tetromino::Type>(type), { 0, 0 });
		CHECK(piece.GetBlockPositions().size() == 4);
	}
}

TEST_CASE("the T piece spawns in its documented cells")
{
	const Tetromino piece(Tetromino::Type::T, { 0, 0 });

	CHECK(HasBlock(piece, { 1, 0 }));
	CHECK(HasBlock(piece, { 0, 1 }));
	CHECK(HasBlock(piece, { 1, 1 }));
	CHECK(HasBlock(piece, { 2, 1 }));
}

TEST_CASE("Move offsets every block")
{
	Tetromino piece(Tetromino::Type::T, { 0, 0 });
	piece.Move(3, 5);

	CHECK(HasBlock(piece, { 4, 5 }));
	CHECK(HasBlock(piece, { 3, 6 }));
	CHECK(HasBlock(piece, { 4, 6 }));
	CHECK(HasBlock(piece, { 5, 6 }));
}

TEST_CASE("four clockwise rotations return to the spawn state")
{
	Tetromino piece(Tetromino::Type::T, { 0, 0 });
	const auto spawn = piece.GetBlockPositions();

	CHECK(piece.GetRotationIndex() == 0);

	for (int i = 0; i < 4; i++)
	{
		piece.RotateClockwise();
	}

	CHECK(piece.GetRotationIndex() == 0);
	CHECK(piece.GetBlockPositions() == spawn);
}

TEST_CASE("counter-clockwise is the inverse of clockwise")
{
	Tetromino piece(Tetromino::Type::J, { 2, 2 });
	const auto spawn = piece.GetBlockPositions();

	piece.RotateClockwise();
	piece.RotateCounterClockwise();

	CHECK(piece.GetBlockPositions() == spawn);

	piece.RotateCounterClockwise();
	CHECK(piece.GetRotationIndex() == 3);
}

TEST_CASE("the O piece is unchanged by rotation")
{
	Tetromino piece(Tetromino::Type::O, { 0, 0 });
	const auto spawn = piece.GetBlockPositions();

	piece.RotateClockwise();

	CHECK(piece.GetBlockPositions() == spawn);
}
