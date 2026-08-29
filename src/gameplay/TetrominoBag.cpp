#include "TetrominoBag.h"

#include <algorithm>

#include "../utils/Random.h"

Tetromino::Type TetrominoBag::Next()
{
	if (bag.empty())
	{
		Refill();
	}

	const Tetromino::Type nextTetromino = bag.back();
	bag.pop_back();

	return nextTetromino;
}

void TetrominoBag::Refill()
{
	bag =
	{
		Tetromino::Type::I,
		Tetromino::Type::O,
		Tetromino::Type::T,
		Tetromino::Type::S,
		Tetromino::Type::Z,
		Tetromino::Type::J,
		Tetromino::Type::L
	};

	std::shuffle(bag.begin(), bag.end(), Random::Engine());
}