#include "palette.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <sstream>
#include <tuple>
#include <unordered_map>

#include <vs/engine/error.hpp>
#include <vs/engine/json_file.hpp>

#include "path_util.hpp"

namespace painter {

namespace {

std::uint32_t pack(vs::Color c) {
    return (static_cast<std::uint32_t>(c.r) << 24) | (static_cast<std::uint32_t>(c.g) << 16) |
           (static_cast<std::uint32_t>(c.b) << 8) | c.a;
}

vs::Color unpack(std::uint32_t v) {
    return {static_cast<std::uint8_t>(v >> 24), static_cast<std::uint8_t>(v >> 16), static_cast<std::uint8_t>(v >> 8),
            static_cast<std::uint8_t>(v)};
}

// Sort key: transparent < greys (by lightness) < colours (12 hue buckets, then lightness), alpha last.
std::tuple<int, int, int, int, std::uint32_t> sort_key(vs::Color c) {
    if (c.a == 0) return {0, 0, 0, 0, pack(c)};
    const int mx = std::max({c.r, c.g, c.b});
    const int mn = std::min({c.r, c.g, c.b});
    const int light = c.r * 299 + c.g * 587 + c.b * 114;
    if (mx - mn < 16) return {1, 0, light, 255 - c.a, pack(c)};
    float h = 0;
    const float d = static_cast<float>(mx - mn);
    if (mx == c.r)
        h = std::fmod((static_cast<float>(c.g) - c.b) / d, 6.0f);
    else if (mx == c.g)
        h = (static_cast<float>(c.b) - c.r) / d + 2.0f;
    else
        h = (static_cast<float>(c.r) - c.g) / d + 4.0f;
    if (h < 0) h += 6.0f;
    const int bucket = static_cast<int>(h * 2.0f) % 12;  // 30 degree buckets
    return {2, bucket, light, 255 - c.a, pack(c)};
}

std::string trim(std::string_view s) {
    std::size_t b = 0, e = s.size();
    while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
    return std::string(s.substr(b, e - b));
}

std::vector<std::string> lines_of(std::string_view text) {
    std::vector<std::string> out;
    std::size_t start = 0;
    while (start <= text.size()) {
        std::size_t end = text.find('\n', start);
        if (end == std::string_view::npos) end = text.size();
        out.push_back(trim(text.substr(start, end - start)));
        start = end + 1;
    }
    return out;
}

}  // namespace

void sort_palette(Palette& palette) {
    std::sort(palette.begin(), palette.end(), [](vs::Color a, vs::Color b) { return sort_key(a) < sort_key(b); });
}

Palette palette_from_image(const vs::Image& image, std::size_t max_colors) {
    std::unordered_map<std::uint32_t, std::size_t> counts;
    for (std::size_t i = 0; i + 3 < image.pixels.size(); i += 4) {
        vs::Color c{image.pixels[i], image.pixels[i + 1], image.pixels[i + 2], image.pixels[i + 3]};
        if (c.a == 0) c = {0, 0, 0, 0};
        ++counts[pack(c)];
    }
    std::vector<std::pair<std::uint32_t, std::size_t>> list(counts.begin(), counts.end());
    if (list.size() > max_colors) {
        std::partial_sort(list.begin(), list.begin() + static_cast<std::ptrdiff_t>(max_colors), list.end(),
                          [](const auto& a, const auto& b) { return a.second != b.second ? a.second > b.second : a.first < b.first; });
        list.resize(max_colors);
    }
    Palette out;
    out.reserve(list.size());
    for (const auto& [v, n] : list) out.push_back(unpack(v));
    sort_palette(out);
    return out;
}

std::string to_hex(vs::Color c, bool force_alpha) {
    static const char* digits = "0123456789ABCDEF";
    std::string s;
    auto put = [&](std::uint8_t v) {
        s += digits[v >> 4];
        s += digits[v & 15];
    };
    put(c.r);
    put(c.g);
    put(c.b);
    if (force_alpha || c.a != 255) put(c.a);
    return s;
}

std::optional<vs::Color> parse_hex(std::string_view text) {
    std::string s = trim(text);
    if (!s.empty() && s[0] == '#') s.erase(0, 1);
    if (s.size() != 6 && s.size() != 8) return std::nullopt;
    std::uint8_t v[4] = {0, 0, 0, 255};
    for (std::size_t i = 0; i < s.size(); i += 2) {
        int byte = 0;
        for (std::size_t k = i; k < i + 2; ++k) {
            const char ch = static_cast<char>(std::tolower(static_cast<unsigned char>(s[k])));
            int d = 0;
            if (ch >= '0' && ch <= '9')
                d = ch - '0';
            else if (ch >= 'a' && ch <= 'f')
                d = ch - 'a' + 10;
            else
                return std::nullopt;
            byte = byte * 16 + d;
        }
        v[i / 2] = static_cast<std::uint8_t>(byte);
    }
    return vs::Color{v[0], v[1], v[2], v[3]};
}

Palette parse_hex_palette(std::string_view text) {
    Palette out;
    int line_no = 0;
    for (const std::string& line : lines_of(text)) {
        ++line_no;
        if (line.empty() || line[0] == ';') continue;
        const std::optional<vs::Color> c = parse_hex(line);
        if (!c) throw vs::Error("palette: line " + std::to_string(line_no) + " is not a hex colour: \"" + line + "\"");
        out.push_back(*c);
    }
    return out;
}

Palette parse_gpl_palette(std::string_view text) {
    const std::vector<std::string> lines = lines_of(text);
    if (lines.empty() || lines[0].rfind("GIMP Palette", 0) != 0) throw vs::Error("palette: not a GIMP palette (no \"GIMP Palette\" header)");
    Palette out;
    for (std::size_t i = 1; i < lines.size(); ++i) {
        const std::string& line = lines[i];
        if (line.empty() || line[0] == '#' || line.rfind("Name:", 0) == 0 || line.rfind("Columns:", 0) == 0) continue;
        std::istringstream in(line);
        int r = 0, g = 0, b = 0;
        if (!(in >> r >> g >> b) || r < 0 || g < 0 || b < 0 || r > 255 || g > 255 || b > 255)
            throw vs::Error("palette: line " + std::to_string(i + 1) + " is not \"R G B\": \"" + line + "\"");
        out.push_back({static_cast<std::uint8_t>(r), static_cast<std::uint8_t>(g), static_cast<std::uint8_t>(b), 255});
    }
    return out;
}

std::string hex_palette_text(const Palette& palette) {
    std::string out;
    for (vs::Color c : palette) out += to_hex(c) + "\n";
    return out;
}

std::string gpl_palette_text(const Palette& palette, std::string_view name) {
    std::string out = "GIMP Palette\nName: " + std::string(name) + "\nColumns: 16\n#\n";
    for (vs::Color c : palette) {
        if (c.a == 0) continue;  // GPL has no alpha: the transparent entry is implicit
        char buf[64];
        std::snprintf(buf, sizeof buf, "%3d %3d %3d\t#%s\n", c.r, c.g, c.b, to_hex(c.with_alpha(255)).c_str());
        out += buf;
    }
    return out;
}

Palette load_palette(const std::filesystem::path& path) {
    const std::string text = vs::read_text_file(path);
    try {
        return lower_ext(path) == ".gpl" ? parse_gpl_palette(text) : parse_hex_palette(text);
    } catch (const vs::Error& e) {
        throw vs::Error(vs::path_to_utf8(path) + ": " + e.what());
    }
}

void save_palette(const std::filesystem::path& path, const Palette& palette) {
    const std::string text =
        lower_ext(path) == ".gpl" ? gpl_palette_text(palette, vs::path_to_utf8(path.stem())) : hex_palette_text(palette);
    vs::write_file_atomic(path, text);
}

}  // namespace painter
