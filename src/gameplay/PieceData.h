#pragma once

#include <array>
#include <optional>

#include <SFML/System/Vector2.hpp>

#include "Tetromino.h"
#include "TetrominoShapes.h"

// The block layout of every tetromino in each of its four rotation states, as
// offsets in a 4x4 local frame. Built from the canonical SRS shapes in
// TetrominoShapes.h; individual pieces can be replaced at startup with data
// from assets/data/pieces.json (see PieceDataFile.h).
//
// This is process-wide, authored, load-once-before-play data -- Tetromino is a
// bare value type and reads it directly, the same way it used to read the
// TetrominoShapes constants. It is not thread-safe; load overrides before the
// first piece is in play.
namespace PieceData
{
	inline constexpr int BlockCount = TetrominoShapes::BLOCK_COUNT;
	inline constexpr int RotationCount = TetrominoShapes::ROTATION_COUNT;

	// The BlockCount block offsets for one rotation state, in the 4x4 local frame.
	using BlockOffsets = std::array<sf::Vector2i, BlockCount>;

	// The four rotation states (Spawn / Right / Reverse / Left).
	using Rotations = std::array<BlockOffsets, RotationCount>;

	// Block offsets for one rotation state. `rotationIndex` is wrapped to 0..3.
	[[nodiscard]] const BlockOffsets& Blocks(Tetromino::Type type, int rotationIndex);

	// Replace one piece's rotation table (used by the JSON loader; also handy for
	// tests that want a known layout).
	void SetRotations(Tetromino::Type type, const Rotations& rotations);

	// Restore every piece to the built-in SRS layout.
	void ResetToDefaults();

	// Parse a 4x4 character grid ('X' = block, anything else = empty) into the
	// offsets for one rotation state, in row-major order. std::nullopt if the
	// grid does not hold exactly BlockCount blocks.
	[[nodiscard]] std::optional<BlockOffsets> ParseState(const TetrominoShapes::ShapeMatrix& rows);

	// Parse all four rotation states. std::nullopt if any state is malformed.
	[[nodiscard]] std::optional<Rotations> ParseShape(const TetrominoShapes::RotationSet& states);
}
