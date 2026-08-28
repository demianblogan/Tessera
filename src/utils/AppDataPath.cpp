#include "AppDataPath.h"

#include <cstdlib>

namespace AppDataPath
{
	std::filesystem::path Resolve(std::string_view fileName)
	{
		char* localAppData = nullptr;
		std::size_t valueLength = 0;

		if (_dupenv_s(&localAppData, &valueLength, "LOCALAPPDATA") == 0 &&
			localAppData != nullptr)
		{
			const std::filesystem::path directory =
				std::filesystem::path(localAppData) /
				"Alone Bull Company" /
				"Tessera";

			std::free(localAppData);

			return directory / fileName;
		}

		std::free(localAppData);

		return std::filesystem::current_path() / "user_data" / fileName;
	}
}
