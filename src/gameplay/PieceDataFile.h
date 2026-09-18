#pragma once

#include <filesystem>

// Loads assets/data/pieces.json into PieceData, overriding the built-in SRS
// layout for any piece the file defines. A missing file, a missing piece, or a
// malformed entry leaves that piece on its built-in default. Call once at
// startup, before the first piece is in play.
//
// File shape:
//   {
//     "pieces": {
//       "T": { "rotations": [
//         [".X..", "XXX.", "....", "...."],   // spawn
//         [".X..", ".XX.", ".X..", "...."],   // right
//         ["....", "XXX.", ".X..", "...."],   // reverse
//         [".X..", "XX..", ".X..", "...."]    // left
//       ] },
//       ...
//     }
//   }
namespace PieceDataFile
{
	void Load(const std::filesystem::path& path);
}
