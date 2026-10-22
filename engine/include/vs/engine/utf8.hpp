#pragma once
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace vs {

// Replacement character returned for malformed UTF-8 sequences.
inline constexpr char32_t kReplacementChar = U'�';

// Decodes the code point that starts at byte offset `pos` and advances `pos` past it.
// Malformed or truncated sequences yield kReplacementChar and advance by one byte.
// Precondition: pos < text.size().
char32_t utf8_next(std::string_view text, std::size_t& pos);

// Decodes a whole string into code points.
std::u32string utf8_decode(std::string_view text);

// Appends the UTF-8 encoding of `cp` to `out` (invalid code points become U+FFFD).
void utf8_append(std::string& out, char32_t cp);

// Encodes code points into a UTF-8 string.
std::string utf8_encode(std::u32string_view text);

// Number of code points in `text`.
std::size_t utf8_length(std::string_view text);

// Removes the last code point (for Backspace in text fields). No-op on an empty string.
void utf8_pop_back(std::string& text);

// Byte offset of the code point with index `index` (or text.size() if index >= length).
std::size_t utf8_offset(std::string_view text, std::size_t index);

// Upper-cases ASCII a-z and Cyrillic а-я, ё (U+0430..U+044F, U+0450..U+045F); others unchanged.
char32_t to_upper(char32_t cp);

}  // namespace vs
