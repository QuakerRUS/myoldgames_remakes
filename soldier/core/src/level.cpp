#include "soldier/level.hpp"

#include <fstream>
#include <sstream>

#include <nlohmann/json.hpp>

namespace soldier {
namespace {

bool is_start_tile(Tile t) {
    const auto g = player_ground(t);
    return g && *g != Ground::SunkBox;  // the original scan ignored 57..60
}

}  // namespace

Point Level::start() const {
    Point found{-1, -1};
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x)
            if (is_start_tile(at({x, y}))) found = {x, y};
    if (found.x < 0) throw FormatError("level " + std::to_string(number) + " has no player start");
    return found;
}

Level parse_level_json(std::string_view text) {
    Level lv;
    try {
        const nlohmann::json doc = nlohmann::json::parse(text);
        lv.number = doc.at("number").get<int>();
        lv.width = doc.at("width").get<int>();
        lv.height = doc.at("height").get<int>();
        lv.time_limit = doc.at("time_limit").get<int>();
        if (lv.width < 1 || lv.height < 1 || lv.width > kMaxLevelSize || lv.height > kMaxLevelSize)
            throw FormatError("level size out of range");
        if (lv.time_limit < 0) throw FormatError("negative time_limit");
        const auto& rows = doc.at("rows");
        if (!rows.is_array() || static_cast<int>(rows.size()) != lv.height)
            throw FormatError("rows count differs from height");
        lv.cells.reserve(static_cast<std::size_t>(lv.width * lv.height));
        for (const auto& row : rows) {
            const auto s = row.get<std::string>();
            if (static_cast<int>(s.size()) != lv.width) throw FormatError("row length differs from width");
            for (char c : s) {
                const auto t = tile_from_char(c);
                if (!t) throw FormatError(std::string("unknown tile character '") + c + "'");
                lv.cells.push_back(*t);
            }
        }
    } catch (const nlohmann::json::exception& e) {
        throw FormatError(std::string("level JSON: ") + e.what());
    }
    (void)lv.start();  // validate
    return lv;
}

std::string read_text_file(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw FormatError("cannot open " + path);
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

Level load_level_file(const std::string& path) { return parse_level_json(read_text_file(path)); }

std::string level_to_json(const Level& lv) {
    std::string out = "{\n";
    out += "  \"number\": " + std::to_string(lv.number) + ",\n";
    out += "  \"width\": " + std::to_string(lv.width) + ",\n";
    out += "  \"height\": " + std::to_string(lv.height) + ",\n";
    out += "  \"time_limit\": " + std::to_string(lv.time_limit) + ",\n";
    out += "  \"rows\": [\n";
    for (int y = 0; y < lv.height; ++y) {
        std::string row;
        for (int x = 0; x < lv.width; ++x) row += tile_char(lv.at({x, y}));
        out += "    " + nlohmann::json(row).dump() + (y + 1 < lv.height ? ",\n" : "\n");
    }
    out += "  ]\n}\n";
    return out;
}

}  // namespace soldier
