#include "PieceDataFile.h"

#include <array>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include <nlohmann/json.hpp>

#include "PieceData.h"
#include "Tetromino.h"
#include "TetrominoShapes.h"

namespace
{
	using Json = nlohmann::json;

	using PieceEntry = std::pair<std::string_view, Tetromino::Type>;
	constexpr std::size_t PieceTypeCount = static_cast<std::size_t>(Tetromino::Type::Count);

	constexpr std::array<PieceEntry, PieceTypeCount> Pieces =
	{ {
		{ "I", Tetromino::Type::I },
		{ "O", Tetromino::Type::O },
		{ "T", Tetromino::Type::T },
		{ "S", Tetromino::Type::S },
		{ "Z", Tetromino::Type::Z },
		{ "J", Tetromino::Type::J },
		{ "L", Tetromino::Type::L },
	} };

	// One rotation state as owned strings, plus the string_view grid that
	// TetrominoShapes / PieceData parse. The views point into `storage`.
	struct StateRows
	{
		std::array<std::string, TetrominoShapes::MatrixSize> storage;

		TetrominoShapes::ShapeMatrix Views() const
		{
			TetrominoShapes::ShapeMatrix views{};
			for (std::size_t i = 0; i < storage.size(); i++)
				views[i] = storage[i];
			return views;
		}
	};

	[[nodiscard]] std::optional<PieceData::Rotations> ReadRotations(const Json& rotations)
	{
		if (!rotations.is_array() || rotations.size() != TetrominoShapes::RotationCount)
			return std::nullopt;

		std::array<StateRows, TetrominoShapes::RotationCount> owned{};

		for (std::size_t state = 0; state < TetrominoShapes::RotationCount; state++)
		{
			const Json& grid = rotations[state];
			if (!grid.is_array() || grid.size() != TetrominoShapes::MatrixSize)
				return std::nullopt;

			for (std::size_t row = 0; row < TetrominoShapes::MatrixSize; row++)
			{
				if (!grid[row].is_string())
					return std::nullopt;

				owned[state].storage[row] = grid[row].get<std::string>();
			}
		}

		TetrominoShapes::RotationSet views{};
		for (std::size_t state = 0; state < TetrominoShapes::RotationCount; state++)
			views[state] = owned[state].Views();

		return PieceData::ParseShape(views);
	}
}

// A missing/invalid file, a missing "pieces" object, or any one piece being
// malformed just leaves that piece (or all of them) at its built-in SRS
// shape -- pieces.json is an optional authoring override, not a requirement.
void PieceDataFile::Load(const std::filesystem::path& path)
{
	std::ifstream file(path);
	if (!file.is_open())
		return;

	Json data;

	try
	{
		data = Json::parse(file);
	}
	catch (const Json::exception&)
	{
		return;
	}

	const auto pieces = data.find("pieces");
	if (pieces == data.end() || !pieces->is_object())
		return;

	for (const auto& [name, type] : Pieces)
	{
		const auto piece = pieces->find(name);
		if (piece == pieces->end())
			continue;

		const auto rotations = piece->find("rotations");
		if (rotations == piece->end())
			continue;

		const std::optional<PieceData::Rotations> parsedRotations = ReadRotations(*rotations);
		if (!parsedRotations.has_value())
			continue;

		PieceData::SetRotations(type, *parsedRotations);
	}
}
