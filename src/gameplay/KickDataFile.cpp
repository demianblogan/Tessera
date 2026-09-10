#include "KickDataFile.h"

#include <array>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include <nlohmann/json.hpp>

#include "KickData.h"

namespace
{
	using Json = nlohmann::json;

	constexpr std::array<std::string_view, 8> SlotNames = {
		"0->R", "0->L", "R->2", "R->0", "2->L", "2->R", "L->0", "L->2"
	};

	[[nodiscard]] std::optional<KickData::Tests> ReadTests(const Json& value)
	{
		if (!value.is_array() || value.size() != KickData::TestCount)
		{
			return std::nullopt;
		}

		KickData::Tests tests{};

		for (int i = 0; i < KickData::TestCount; ++i)
		{
			const Json& pair = value[i];
			if (!pair.is_array() || pair.size() != 2 || !pair[0].is_number_integer() || !pair[1].is_number_integer())
			{
				return std::nullopt;
			}

			tests[i] = { pair[0].get<int>(), pair[1].get<int>() };
		}

		return tests;
	}

	void ReadTable(const Json& tables, std::string_view name, KickData::Table table, const std::filesystem::path& path)
	{
		const auto object = tables.find(name);
		if (object == tables.end())
		{
			return;
		}

		if (!object->is_object())
		{
			std::cerr << "WARNING: kick table \"" << name << "\" in \"" << path.string()
				<< "\" is not an object -- keeping its built-in values.\n";
			return;
		}

		for (int slot = 0; slot < 8; ++slot)
		{
			const auto transition = object->find(SlotNames[slot]);
			if (transition == object->end())
			{
				continue;
			}

			const std::optional<KickData::Tests> tests = ReadTests(*transition);
			if (!tests)
			{
				std::cerr << "WARNING: kick \"" << name << "\" / \"" << SlotNames[slot] << "\" in \""
					<< path.string() << "\" needs 5 [x, y] pairs -- keeping its built-in value.\n";
				continue;
			}

			KickData::SetSlot(table, slot, *tests);
		}
	}
}

void KickDataFile::Load(const std::filesystem::path& path)
{
	std::ifstream file(path);
	if (!file.is_open())
	{
		std::cerr << "WARNING: kick data file not found at \"" << path.string()
			<< "\" -- using the built-in SRS kicks.\n";
		return;
	}

	Json data;
	try
	{
		data = Json::parse(file);
	}
	catch (const Json::exception& exception)
	{
		std::cerr << "WARNING: kick data file \"" << path.string()
			<< "\" is invalid (" << exception.what() << ") -- using the built-in SRS kicks.\n";
		return;
	}

	const auto tables = data.find("tables");
	if (tables == data.end() || !tables->is_object())
	{
		std::cerr << "WARNING: kick data file \"" << path.string()
			<< "\" has no \"tables\" object -- using the built-in SRS kicks.\n";
		return;
	}

	ReadTable(*tables, "JLSTZ", KickData::Table::JLSTZ, path);
	ReadTable(*tables, "I", KickData::Table::I, path);
}
