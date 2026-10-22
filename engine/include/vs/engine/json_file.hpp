#pragma once
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

namespace vs {

// Whole-file reads. Throw vs::Error if the file cannot be read.
std::string read_text_file(const std::filesystem::path& path);
std::vector<unsigned char> read_binary_file(const std::filesystem::path& path);

// Atomic write: writes `<path>.tmp` next to the target, flushes it to disk, then renames it over
// the target (Windows: MoveFileExW REPLACE_EXISTING | WRITE_THROUGH, retried a few times on
// sharing violations, e.g. from antivirus scanners), so a crash never leaves a half-written
// file. Creates missing parent directories. Throws vs::Error (with the system's reason) on
// failure; the `.tmp` file is removed on every failure path and the target stays untouched.
void write_file_atomic(const std::filesystem::path& path, std::string_view data);

// Returns std::nullopt if the file does not exist; throws vs::Error if it exists but cannot
// be read or is not valid JSON.
std::optional<nlohmann::json> read_json_file(const std::filesystem::path& path);
// Pretty-printed (indent spaces, UTF-8, trailing newline) atomic write. Throws vs::Error.
void write_json_file(const std::filesystem::path& path, const nlohmann::json& value, int indent = 2);

// UTF-8 string of a path (for SDL functions and messages).
std::string path_to_utf8(const std::filesystem::path& path);
// Path from a UTF-8 string.
std::filesystem::path utf8_to_path(std::string_view utf8);

}  // namespace vs
