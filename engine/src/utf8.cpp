#include <vs/engine/utf8.hpp>

namespace vs {

namespace {
bool is_continuation(unsigned char c) { return (c & 0xC0) == 0x80; }
}  // namespace

char32_t utf8_next(std::string_view text, std::size_t& pos) {
    const auto lead = static_cast<unsigned char>(text[pos]);
    if (lead < 0x80) {
        ++pos;
        return lead;
    }
    std::size_t len = 0;
    char32_t cp = 0;
    char32_t min = 0;
    if ((lead & 0xE0) == 0xC0) {
        len = 2;
        cp = lead & 0x1F;
        min = 0x80;
    } else if ((lead & 0xF0) == 0xE0) {
        len = 3;
        cp = lead & 0x0F;
        min = 0x800;
    } else if ((lead & 0xF8) == 0xF0) {
        len = 4;
        cp = lead & 0x07;
        min = 0x10000;
    } else {
        ++pos;
        return kReplacementChar;
    }
    if (pos + len > text.size()) {
        ++pos;
        return kReplacementChar;
    }
    for (std::size_t i = 1; i < len; ++i) {
        const auto c = static_cast<unsigned char>(text[pos + i]);
        if (!is_continuation(c)) {
            ++pos;
            return kReplacementChar;
        }
        cp = (cp << 6) | (c & 0x3F);
    }
    if (cp < min || cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) {
        ++pos;
        return kReplacementChar;
    }
    pos += len;
    return cp;
}

std::u32string utf8_decode(std::string_view text) {
    std::u32string out;
    out.reserve(text.size());
    std::size_t pos = 0;
    while (pos < text.size()) out.push_back(utf8_next(text, pos));
    return out;
}

void utf8_append(std::string& out, char32_t cp) {
    if (cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) cp = kReplacementChar;
    if (cp < 0x80) {
        out.push_back(static_cast<char>(cp));
    } else if (cp < 0x800) {
        out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else if (cp < 0x10000) {
        out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else {
        out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    }
}

std::string utf8_encode(std::u32string_view text) {
    std::string out;
    out.reserve(text.size());
    for (char32_t cp : text) utf8_append(out, cp);
    return out;
}

std::size_t utf8_length(std::string_view text) {
    std::size_t n = 0;
    std::size_t pos = 0;
    while (pos < text.size()) {
        utf8_next(text, pos);
        ++n;
    }
    return n;
}

void utf8_pop_back(std::string& text) {
    if (text.empty()) return;
    // Find the start of the last code point the same way utf8_next splits the string.
    std::size_t pos = 0;
    std::size_t last = 0;
    while (pos < text.size()) {
        last = pos;
        utf8_next(text, pos);
    }
    text.erase(last);
}

std::size_t utf8_offset(std::string_view text, std::size_t index) {
    std::size_t pos = 0;
    for (std::size_t i = 0; i < index && pos < text.size(); ++i) utf8_next(text, pos);
    return pos;
}

char32_t to_upper(char32_t cp) {
    if (cp >= U'a' && cp <= U'z') return cp - 32;
    if (cp >= 0x0430 && cp <= 0x044F) return cp - 0x20;  // а..я -> А..Я
    if (cp >= 0x0450 && cp <= 0x045F) return cp - 0x50;  // ѐ..џ (incl. ё) -> Ѐ..Џ
    return cp;
}

}  // namespace vs
