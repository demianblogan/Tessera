#include "PieceData.h"

#include <array>

namespace
{
	// Piece order matches Tetromino::Type (I, O, T, S, Z, J, L).
	constexpr int PieceCount = 7;

	[[nodiscard]] const TetrominoShapes::RotationSet& DefaultShape(int pieceIndex)
	{
		switch (static_cast<Tetromino::Type>(pieceIndex))
		{
		case Tetromino::Type::I: return TetrominoShapes::I;
		case Tetromino::Type::O: return TetrominoShapes::O;
		case Tetromino::Type::T: return TetrominoShapes::T;
		case Tetromino::Type::S: return TetrominoShapes::S;
		case Tetromino::Type::Z: return TetrominoShapes::Z;
		case Tetromino::Type::J: return TetrominoShapes::J;
		case Tetromino::Type::L: return TetrominoShapes::L;
		}

		return TetrominoShapes::I;
	}

	[[nodiscard]] std::array<PieceData::Rotations, PieceCount> MakeDefaults()
	{
		std::array<PieceData::Rotations, PieceCount> table{};

		for (int piece = 0; piece < PieceCount; ++piece)
		{
			// The built-in shapes are known-good, so the parse cannot fail.
			table[piece] = PieceData::ParseShape(DefaultShape(piece)).value();
		}

		return table;
	}

	std::array<PieceData::Rotations, PieceCount>& Table()
	{
		static std::array<PieceData::Rotations, PieceCount> table = MakeDefaults();
		return table;
	}
}

const PieceData::BlockOffsets& PieceData::Blocks(Tetromino::Type type, int rotationIndex)
{
	const int wrapped = ((rotationIndex % RotationCount) + RotationCount) % RotationCount;
	return Table()[static_cast<int>(type)][wrapped];
}

void PieceData::SetRotations(Tetromino::Type type, const Rotations& rotations)
{
	Table()[static_cast<int>(type)] = rotations;
}

void PieceData::ResetToDefaults()
{
	Table() = MakeDefaults();
}

std::optional<PieceData::BlockOffsets> PieceData::ParseState(const TetrominoShapes::ShapeMatrix& rows)
{
	BlockOffsets offsets{};
	int count = 0;

	for (int y = 0; y < TetrominoShapes::MATRIX_SIZE; ++y)
	{
		const std::string_view row = rows[y];

		if (row.size() != TetrominoShapes::MATRIX_SIZE)
		{
			return std::nullopt;
		}

		for (int x = 0; x < TetrominoShapes::MATRIX_SIZE; ++x)
		{
			if (row[x] != 'X')
			{
				continue;
			}

			if (count >= BlockCount)
			{
				return std::nullopt;
			}

			offsets[count] = { x, y };
			++count;
		}
	}

	if (count != BlockCount)
	{
		return std::nullopt;
	}

	return offsets;
}

std::optional<PieceData::Rotations> PieceData::ParseShape(const TetrominoShapes::RotationSet& states)
{
	Rotations rotations{};

	for (int i = 0; i < RotationCount; ++i)
	{
		const std::optional<BlockOffsets> parsed = ParseState(states[i]);
		if (!parsed)
		{
			return std::nullopt;
		}

		rotations[i] = *parsed;
	}

	return rotations;
}
