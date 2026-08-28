#pragma once

#include <filesystem>
#include <system_error>

// Crash-safe file writing. Write new content to a temp file first, then call
// ReplaceFileAtomically to swap it into place -- a crash, power loss or a lock
// mid-write then never leaves a missing or half-written save; at worst the old
// (still valid) file survives.
namespace SafeFileWrite
{
	// Named check so a filesystem-error test at a call site reads as a
	// sentence ("if HasFailed(error)").
	[[nodiscard]] bool HasFailed(const std::error_code& error) noexcept;

	// Swaps a freshly written temp file into targetPath's place. Backs the
	// previous file up to <targetPath>.bak first and rolls back to it if the
	// final swap fails, so a mid-operation failure never leaves targetPath
	// missing.
	[[nodiscard]] bool ReplaceFileAtomically(
		const std::filesystem::path& temporaryPath,
		const std::filesystem::path& targetPath);

	// Renames a file that failed to load/parse to <path>.corrupt (or
	// .corrupt.1, .corrupt.2, ... if taken), rather than deleting or silently
	// overwriting it -- a corrupted save stays on disk for inspection while the
	// caller starts fresh.
	[[nodiscard]] bool PreserveCorruptFile(const std::filesystem::path& path);
}
