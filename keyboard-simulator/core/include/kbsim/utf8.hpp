#pragma once
// UTF-8 <-> UTF-32 conversion and file loading helpers of kbsim_core.

#include <filesystem>
#include <stdexcept>
#include <string>
#include <string_view>

namespace kbsim {

// Thrown when a data file is missing or malformed.
struct LoadError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

// Decodes UTF-8 (a leading BOM is skipped). Throws std::invalid_argument on malformed input
// (bad sequences, overlong forms, surrogates, code points above U+10FFFF).
std::u32string utf8_to_u32(std::string_view text);

// Encodes UTF-32 as UTF-8. Throws std::invalid_argument on surrogates or values above U+10FFFF.
std::string u32_to_utf8(std::u32string_view text);
std::string u32_to_utf8(char32_t ch);

// Reads a whole file as bytes. Throws LoadError if it cannot be opened.
std::string read_file(const std::filesystem::path& file);

}  // namespace kbsim
