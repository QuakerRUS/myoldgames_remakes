#include <kbsim/utf8.hpp>

#include <fstream>
#include <iterator>

namespace kbsim {

std::u32string utf8_to_u32(std::string_view text) {
    if (text.starts_with("\xEF\xBB\xBF")) text.remove_prefix(3);
    std::u32string out;
    out.reserve(text.size());
    std::size_t i = 0;
    while (i < text.size()) {
        const auto b0 = static_cast<unsigned char>(text[i]);
        int extra = 0;
        char32_t cp = 0;
        char32_t min = 0;
        if (b0 < 0x80) {
            cp = b0;
        } else if ((b0 & 0xE0) == 0xC0) {
            extra = 1, cp = b0 & 0x1F, min = 0x80;
        } else if ((b0 & 0xF0) == 0xE0) {
            extra = 2, cp = b0 & 0x0F, min = 0x800;
        } else if ((b0 & 0xF8) == 0xF0) {
            extra = 3, cp = b0 & 0x07, min = 0x10000;
        } else {
            throw std::invalid_argument("invalid UTF-8 lead byte at offset " + std::to_string(i));
        }
        if (i + static_cast<std::size_t>(extra) >= text.size() && extra > 0) {
            throw std::invalid_argument("truncated UTF-8 sequence at offset " + std::to_string(i));
        }
        for (int k = 1; k <= extra; ++k) {
            const auto b = static_cast<unsigned char>(text[i + k]);
            if ((b & 0xC0) != 0x80) {
                throw std::invalid_argument("invalid UTF-8 continuation at offset " + std::to_string(i + k));
            }
            cp = (cp << 6) | (b & 0x3F);
        }
        if (extra > 0 && cp < min) throw std::invalid_argument("overlong UTF-8 at offset " + std::to_string(i));
        if (cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) {
            throw std::invalid_argument("invalid code point at offset " + std::to_string(i));
        }
        out.push_back(cp);
        i += static_cast<std::size_t>(extra) + 1;
    }
    return out;
}

std::string u32_to_utf8(char32_t cp) {
    std::string out;
    if (cp < 0x80) {
        out.push_back(static_cast<char>(cp));
    } else if (cp < 0x800) {
        out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else if (cp < 0x10000) {
        if (cp >= 0xD800 && cp <= 0xDFFF) throw std::invalid_argument("surrogate code point");
        out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else if (cp <= 0x10FFFF) {
        out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else {
        throw std::invalid_argument("code point above U+10FFFF");
    }
    return out;
}

std::string u32_to_utf8(std::u32string_view text) {
    std::string out;
    out.reserve(text.size() * 2);
    for (char32_t cp : text) out += u32_to_utf8(cp);
    return out;
}

std::string read_file(const std::filesystem::path& file) {
    std::ifstream in(file, std::ios::binary);
    if (!in) throw LoadError("cannot open " + file.string());
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

}  // namespace kbsim
