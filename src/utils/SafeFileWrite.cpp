#include "SafeFileWrite.h"

#include <string>

namespace SafeFileWrite
{
	bool HasFailed(const std::error_code& error) noexcept
	{
		return static_cast<bool>(error);
	}

	bool ReplaceFileAtomically(
		const std::filesystem::path& temporaryPath,
		const std::filesystem::path& targetPath)
	{
		std::error_code error;
		if (!std::filesystem::exists(targetPath, error))
		{
			std::filesystem::rename(temporaryPath, targetPath, error);
			return !HasFailed(error);
		}

		std::filesystem::path backupPath(targetPath);
		backupPath += ".bak";

		std::filesystem::remove(backupPath, error);
		error.clear();

		std::filesystem::rename(targetPath, backupPath, error);
		if (HasFailed(error))
		{
			return false;
		}

		std::filesystem::rename(temporaryPath, targetPath, error);
		if (HasFailed(error))
		{
			std::error_code restoreError;
			std::filesystem::rename(backupPath, targetPath, restoreError);
			return false;
		}

		std::filesystem::remove(backupPath, error);

		return true;
	}

	bool PreserveCorruptFile(const std::filesystem::path& path)
	{
		std::error_code error;
		std::filesystem::path corruptPath(path);
		corruptPath += ".corrupt";

		unsigned int duplicateIndex = 1u;

		while (std::filesystem::exists(corruptPath, error) && !HasFailed(error))
		{
			corruptPath = path;
			corruptPath += ".corrupt." + std::to_string(duplicateIndex++);
		}

		if (HasFailed(error))
		{
			return false;
		}

		std::filesystem::rename(path, corruptPath, error);

		return !HasFailed(error);
	}
}
