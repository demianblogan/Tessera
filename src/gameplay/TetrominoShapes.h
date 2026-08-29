#pragma once

#include <array>
#include <string_view>

// =====================================================
// Shape legend:
//
// . = Empty cell
// X = Tetromino block
// P = Rotation pivot
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
		ShapeMatrix
		{
			"....",
			"XPXX",
			"....",
			"...."
		},
		ShapeMatrix
		{
			"..X.",
			"..P.",
			"..X.",
			"..X."
		},
		ShapeMatrix
		{
			"....",
			"....",
			"XPXX",
			"...."
		},
		ShapeMatrix
		{
			".X..",
			".P..",
			".X..",
			".X.."
		}
	};

	inline constexpr RotationSet O
	{
		ShapeMatrix
		{
			".PX.",
			".XX.",
			"....",
			"...."
		},
		ShapeMatrix
		{
			".PX.",
			".XX.",
			"....",
			"...."
		},
		ShapeMatrix
		{
			".PX.",
			".XX.",
			"....",
			"...."
		},
		ShapeMatrix
		{
			".PX.",
			".XX.",
			"....",
			"...."
		}	};

	inline constexpr RotationSet T
	{
		ShapeMatrix
		{
			".X..",
			"XPX.",
			"....",
			"...."
		},
		ShapeMatrix
		{
			".X..",
			".PX.",
			".X..",
			"...."
		},
		ShapeMatrix
		{
			"....",
			"XPX.",
			".X..",
			"...."
		},
		ShapeMatrix
		{
			".X..",
			"XP..",
			".X..",
			"...."
		}
	};

	inline constexpr RotationSet S
	{
		ShapeMatrix
		{
			".XX.",
			"XP..",
			"....",
			"...."
		},
		ShapeMatrix
		{
			".X..",
			".PX.",
			"..X.",
			"...."
		},
		ShapeMatrix
		{
			".XX.",
			"XP..",
			"....",
			"...."
		},
		ShapeMatrix
		{
			".X..",
			".PX.",
			"..X.",
			"...."
		}
	};

	inline constexpr RotationSet Z
	{
		ShapeMatrix
		{
			"XX..",
			".PX.",
			"....",
			"...."
		},
		ShapeMatrix
		{
			"..X.",
			".PX.",
			".X..",
			"...."
		},
		ShapeMatrix
		{
			"XX..",
			".PX.",
			"....",
			"...."
		},
		ShapeMatrix
		{
			"..X.",
			".PX.",
			".X..",
			"...."
		}
	};

	inline constexpr RotationSet J
	{
		ShapeMatrix
		{
			"X...",
			"XPX.",
			"....",
			"...."
		},
		ShapeMatrix
		{
			".PX.",
			".X..",
			".X..",
			"...."
		},
		ShapeMatrix
		{
			"....",
			"XPX.",
			"..X.",
			"...."
		},
		ShapeMatrix
		{
			".X..",
			".X..",
			"XP..",
			"...."
		}
	};

	inline constexpr RotationSet L
	{
		ShapeMatrix
		{
			"..X.",
			"XPX.",
			"....",
			"...."
		},
		ShapeMatrix
		{
			".X..",
			".X..",
			".PX.",
			"...."
		},
		ShapeMatrix
		{
			"....",
			"XPX.",
			"X...",
			"...."
		},
		ShapeMatrix
		{
			"XP..",
			".X..",
			".X..",
			"...."
		}
	};
}