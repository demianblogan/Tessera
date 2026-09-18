#pragma once

#include <filesystem>

#include "GameSettings.h"

struct Context;

class SettingsManager
{
public:
	SettingsManager(const std::filesystem::path& filepath);

	void Load();
	void Save() const;
	void Apply(Context& context) const;

	GameSettings& GetSettings();
	[[nodiscard]] const GameSettings& GetSettings() const;

private:
	GameSettings settings;
	std::filesystem::path filepath;
};
