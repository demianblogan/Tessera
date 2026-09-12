#pragma once

#include <array>
#include <string_view>

// =====================================================
// The canonical Super Rotation System (SRS) block layout of every tetromino in
// each of its four rotation states, as a 4x4 character grid:
//
//   . = empty cell
//   X = tetromino block
//
// Rotation states, in index order: 0 = Spawn, 1 = Right (clockwise from spawn),
// 2 = Reverse, 3 = Left. These are the built-in defaults; assets/data/pieces.json
// can override individual pieces at startup (see PieceData / PieceDataFile).
// =====================================================

namespace TetrominoShapes
{
	inline constexpr int BLOCK_COUNT = 4;
	inline constexpr int MATRIX_SIZE = 4;
	inline constexpr int ROTATION_COUNT = 4;

	using ShapeMatrix = std::array<std::string_view, MATRIX_SIZE>;
	using RotationSet = std::array<ShapeMatrix, ROTATION_COUNT>;

	inline constexpr RotationSet I
	{
		ShapeMatrix{ "....", "XXXX", "....", "...." },
		ShapeMatrix{ "..X.", "..X.", "..X.", "..X." },
		ShapeMatrix{ "....", "....", "XXXX", "...." },
		ShapeMatrix{ ".X..", ".X..", ".X..", ".X.." }
	};

	inline constexpr RotationSet O
	{
		ShapeMatrix{ ".XX.", ".XX.", "....", "...." },
		ShapeMatrix{ ".XX.", ".XX.", "....", "...." },
		ShapeMatrix{ ".XX.", ".XX.", "....", "...." },
		ShapeMatrix{ ".XX.", ".XX.", "....", "...." }
	};

	inline constexpr RotationSet T
	{
		ShapeMatrix{ ".X..", "XXX.", "....", "...." },
		ShapeMatrix{ ".X..", ".XX.", ".X..", "...." },
		ShapeMatrix{ "....", "XXX.", ".X..", "...." },
		ShapeMatrix{ ".X..", "XX..", ".X..", "...." }
	};

	inline constexpr RotationSet S
	{
		ShapeMatrix{ ".XX.", "XX..", "....", "...." },
		ShapeMatrix{ ".X..", ".XX.", "..X.", "...." },
		ShapeMatrix{ "....", ".XX.", "XX..", "...." },
		ShapeMatrix{ "X...", "XX..", ".X..", "...." }
	};

	inline constexpr RotationSet Z
	{
		ShapeMatrix{ "XX..", ".XX.", "....", "...." },
		ShapeMatrix{ "..X.", ".XX.", ".X..", "...." },
		ShapeMatrix{ "....", "XX..", ".XX.", "...." },
		ShapeMatrix{ ".X..", "XX..", "X...", "...." }
	};

	inline constexpr RotationSet J
	{
		ShapeMatrix{ "X...", "XXX.", "....", "...." },
		ShapeMatrix{ ".XX.", ".X..", ".X..", "...." },
		ShapeMatrix{ "....", "XXX.", "..X.", "...." },
		ShapeMatrix{ ".X..", ".X..", "XX..", "...." }
	};

	inline constexpr RotationSet L
	{
		ShapeMatrix{ "..X.", "XXX.", "....", "...." },
		ShapeMatrix{ ".X..", ".X..", ".XX.", "...." },
		ShapeMatrix{ "....", "XXX.", "X...", "...." },
		ShapeMatrix{ "XX..", ".X..", ".X..", "...." }
	};
}
