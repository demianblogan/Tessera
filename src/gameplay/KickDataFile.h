#pragma once

#include <filesystem>

// Loads assets/data/srs_kicks.json into KickData, overriding any wall-kick
// transition the file defines. A missing file, table, or malformed transition
// leaves that slot on its built-in SRS value (with a warning to stderr). Call
// once at startup, before the first piece is in play.
//
// File shape (offsets are [x, y], +y down, exactly 5 per transition):
//   {
//     "tables": {
//       "JLSTZ": {
//         "0->R": [[0,0], [-1,0], [-1,-1], [0,2], [-1,2]],
//         "0->L": [ ... ], "R->2": [ ... ], "R->0": [ ... ],
//         "2->L": [ ... ], "2->R": [ ... ], "L->0": [ ... ], "L->2": [ ... ]
//       },
//       "I": { ...same eight keys... }
//     }
//   }
namespace KickDataFile
{
	void Load(const std::filesystem::path& path);
}
