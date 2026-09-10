#include "PieceDataFile.h"

#include <array>
#include <fstream>
#include <iostream>
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

	constexpr std::array<std::pair<std::string_view, Tetromino::Type>, 7> Pieces = { {
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
		std::array<std::string, TetrominoShapes::MATRIX_SIZE> storage;
		TetrominoShapes::ShapeMatrix Views() const
		{
			TetrominoShapes::ShapeMatrix views{};
			for (std::size_t i = 0; i < storage.size(); ++i)
			{
				views[i] = storage[i];
			}
			return views;
		}
	};

	[[nodiscard]] std::optional<PieceData::Rotations> ReadRotations(const Json& rotations)
	{
		if (!rotations.is_array() || rotations.size() != TetrominoShapes::ROTATION_COUNT)
		{
			return std::nullopt;
		}

		std::array<StateRows, TetrominoShapes::ROTATION_COUNT> owned{};

		for (std::size_t state = 0; state < TetrominoShapes::ROTATION_COUNT; ++state)
		{
			const Json& grid = rotations[state];
			if (!grid.is_array() || grid.size() != TetrominoShapes::MATRIX_SIZE)
			{
				return std::nullopt;
			}

			for (std::size_t row = 0; row < TetrominoShapes::MATRIX_SIZE; ++row)
			{
				if (!grid[row].is_string())
				{
					return std::nullopt;
				}
				owned[state].storage[row] = grid[row].get<std::string>();
			}
		}

		TetrominoShapes::RotationSet views{};
		for (std::size_t state = 0; state < TetrominoShapes::ROTATION_COUNT; ++state)
		{
			views[state] = owned[state].Views();
		}

		return PieceData::ParseShape(views);
	}
}

void PieceDataFile::Load(const std::filesystem::path& path)
{
	std::ifstream file(path);
	if (!file.is_open())
	{
		std::cerr << "WARNING: piece data file not found at \"" << path.string()
			<< "\" -- using the built-in SRS shapes.\n";
		return;
	}

	Json data;
	try
	{
		data = Json::parse(file);
	}
	catch (const Json::exception& exception)
	{
		std::cerr << "WARNING: piece data file \"" << path.string()
			<< "\" is invalid (" << exception.what() << ") -- using the built-in SRS shapes.\n";
		return;
	}

	const auto pieces = data.find("pieces");
	if (pieces == data.end() || !pieces->is_object())
	{
		std::cerr << "WARNING: piece data file \"" << path.string()
			<< "\" has no \"pieces\" object -- using the built-in SRS shapes.\n";
		return;
	}

	for (const auto& [name, type] : Pieces)
	{
		const auto piece = pieces->find(name);
		if (piece == pieces->end())
		{
			continue;
		}

		const auto rotations = piece->find("rotations");
		if (rotations == piece->end())
		{
			std::cerr << "WARNING: piece \"" << name << "\" in \"" << path.string()
				<< "\" has no \"rotations\" -- keeping its built-in shape.\n";
			continue;
		}

		const std::optional<PieceData::Rotations> parsed = ReadRotations(*rotations);
		if (!parsed)
		{
			std::cerr << "WARNING: piece \"" << name << "\" in \"" << path.string()
				<< "\" is malformed (need 4 states of 4 rows with 4 blocks each)"
				<< " -- keeping its built-in shape.\n";
			continue;
		}

		PieceData::SetRotations(type, *parsed);
	}
}
