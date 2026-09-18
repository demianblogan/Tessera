#include "HighScoreManager.h"

#include <algorithm>
#include <fstream>
#include <string>
#include <system_error>

#include <nlohmann/json.hpp>

#include "../utils/SafeFileWrite.h"

namespace
{
	using Json = nlohmann::json;
}

HighScoreManager::HighScoreManager(const std::filesystem::path& filepath)
	: filepath(filepath)
{}

// File layout is JSON:
//   {
//     "formatVersion": <int>,
//     "records": [ { "score": <int>, "lines": <int>, "level": <int>, "playerName": <UTF-8 string> }, ... ]
//   }
void HighScoreManager::Load()
{
	records.clear();

	std::ifstream file(filepath);
	if (!file.is_open())
		return;

	Json data;
	try
	{
		data = Json::parse(file);
	}
	catch (const Json::exception&)
	{
		// Not a file this build can read -- keep it for inspection, start empty.
		file.close();
		static_cast<void>(SafeFileWrite::PreserveCorruptFile(filepath));
		return;
	}

	const auto formatVersion = data.find("formatVersion");
	const auto recordsField = data.find("records");

	if (formatVersion == data.end() || !formatVersion->is_number_integer() ||
		*formatVersion != FormatVersion || recordsField == data.end() || !recordsField->is_array())
	{
		file.close();
		static_cast<void>(SafeFileWrite::PreserveCorruptFile(filepath));
		return;
	}

	for (const Json& entryData : *recordsField)
	{
		if (!entryData.is_object())
			continue;

		const auto score = entryData.find("score");
		const auto lines = entryData.find("lines");
		const auto level = entryData.find("level");
		const auto playerName = entryData.find("playerName");

		if (score == entryData.end() || !score->is_number_integer() ||
			lines == entryData.end() || !lines->is_number_integer() ||
			level == entryData.end() || !level->is_number_integer() ||
			playerName == entryData.end() || !playerName->is_string())
		{
			continue;
		}

		HighScoreEntry entry;
		entry.score = score->get<int>();
		entry.lines = lines->get<int>();
		entry.level = level->get<int>();

		const std::string utf8Name = playerName->get<std::string>();
		entry.playerName = sf::String::fromUtf8(utf8Name.begin(), utf8Name.end());

		records.push_back(entry);
	}
}

// Mirrors Load's layout:
// {"formatVersion": ..., "records": [{"score": ..., "lines": ..., "level": ..., "playerName": ...}, ...]}
void HighScoreManager::Save() const
{
	std::error_code error;
	std::filesystem::create_directories(filepath.parent_path(), error);

	std::filesystem::path temporaryPath(filepath);
	temporaryPath += ".tmp";

	std::ofstream file(temporaryPath, std::ios::trunc);
	if (!file.is_open())
		return;

	Json data;
	data["formatVersion"] = FormatVersion;
	data["records"] = Json::array();

	for (const HighScoreEntry& entry : records)
	{
		const sf::U8String utf8Name = entry.playerName.toUtf8();

		Json entryData;
		entryData["score"] = entry.score;
		entryData["lines"] = entry.lines;
		entryData["level"] = entry.level;
		entryData["playerName"] = std::string(utf8Name.begin(), utf8Name.end());
		data["records"].push_back(entryData);
	}

	file << data.dump(1, '\t');
	file.close();

	static_cast<void>(SafeFileWrite::ReplaceFileAtomically(temporaryPath, filepath));
}

void HighScoreManager::AddRecord(const HighScoreEntry& entry)
{
	records.push_back(entry);

	auto predicate =
		[](const HighScoreEntry& a, const HighScoreEntry& b)
		{
			return a.score > b.score;
		};

	std::ranges::sort(records, predicate);

	if (records.size() > MaxRecords)
		records.resize(MaxRecords);
}

void HighScoreManager::Clear()
{
	records.clear();
}

bool HighScoreManager::IsHighScore(int score) const
{
	if (score <= 0)
		return false;

	if (records.size() < MaxRecords)
		return true;

	return score > records.back().score;
}

const std::vector<HighScoreEntry>& HighScoreManager::GetRecords() const
{
	return records;
}
