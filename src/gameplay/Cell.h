#pragma once

#include "Tetromino.h"

struct Cell
{
	// Normal: colored by tetrominoType, same as ever. Golden: an escalation
	// bonus piece locked here -- doubles the score of whatever clear takes it.
	// Garbage: a row Board::PushGarbageRow() raised from below.
	enum class Kind
	{
		Normal,
		Golden,
		Garbage
	};

	bool isOccupied{};
	Tetromino::Type tetrominoType{};
	Kind kind{};
};
