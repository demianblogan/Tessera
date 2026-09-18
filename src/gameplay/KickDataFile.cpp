#include "KickDataFile.h"

#include <array>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include <nlohmann/json.hpp>

#include "KickData.h"

namespace
{
	using Json = nlohmann::json;

	constexpr std::array<std::string_view, KickData::SlotCount> SlotNames = {
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

	void ReadTable(const Json& tables, std::string_view name, KickData::Table table)
	{
		const auto object = tables.find(name);
		if (object == tables.end() || !object->is_object())
		{
			return;
		}

		for (int slot = 0; slot < KickData::SlotCount; ++slot)
		{
			const auto transition = object->find(SlotNames[slot]);
			if (transition == object->end())
			{
				continue;
			}

			const std::optional<KickData::Tests> tests = ReadTests(*transition);
			if (!tests.has_value())
			{
				continue;
			}

			KickData::SetSlot(table, slot, *tests);
		}
	}
}

// A missing/invalid file, a missing "tables" object, or any one table/slot
// being malformed just leaves that slot (or all of them) at its built-in SRS
// kick offsets -- srs_kicks.json is an optional authoring override, not a
// requirement.
void KickDataFile::Load(const std::filesystem::path& path)
{
	std::ifstream file(path);
	if (!file.is_open())
	{
		return;
	}

	Json data;
	try
	{
		data = Json::parse(file);
	}
	catch (const Json::exception&)
	{
		return;
	}

	const auto tables = data.find("tables");
	if (tables == data.end() || !tables->is_object())
	{
		return;
	}

	ReadTable(*tables, "JLSTZ", KickData::Table::JLSTZ);
	ReadTable(*tables, "I", KickData::Table::I);
}
