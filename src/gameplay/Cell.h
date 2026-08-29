#pragma once

#include "Tetromino.h"

struct Cell
{
	bool occupied = false;
	Tetromino::Type tetrominoType = Tetromino::Type::I;
};