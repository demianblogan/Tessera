#pragma once

#include "Tetromino.h"

struct Cell
{
	// Normal: coloured by tetrominoType, same as ever. Golden: an escalation
	// bonus piece locked here -- doubles the score of whatever clear takes it.
	// Garbage: a row Board::PushGarbageRow() raised from below.
	enum class Kind { Normal, Golden, Garbage };

	bool occupied = false;
	Tetromino::Type tetrominoType = Tetromino::Type::I;
	Kind kind = Kind::Normal;
};