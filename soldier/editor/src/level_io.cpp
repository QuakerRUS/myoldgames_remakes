#include "level_io.hpp"

#include <algorithm>
#include <cstdio>

#include <nlohmann/json.hpp>

#include <vs/engine/error.hpp>
#include <vs/engine/json_file.hpp>

namespace soldier_editor {

int doc_number(const vs::TileDocument& doc) { return doc.properties.value("number", 1); }
int doc_time_limit(const vs::TileDocument& doc) { return doc.properties.value("time_limit", 0); }

vs::TileDocument make_new_document(int width, int height) {
    vs::TileDocument doc{vs::TileGrid(width, height, static_cast<int>(soldier::Tile::Floor)),
                         {{"number", 1}, {"time_limit", 0}}};
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x)
            if (x == 0 || y == 0 || x == width - 1 || y == height - 1)
                doc.grid.set(x, y, static_cast<int>(soldier::Tile::Wall));
    return doc;
}

soldier::Level document_to_level(const vs::TileDocument& doc) {
    soldier::Level lv;
    lv.number = doc_number(doc);
    lv.time_limit = doc_time_limit(doc);
    lv.width = doc.grid.width();
    lv.height = doc.grid.height();
    lv.cells.reserve(doc.grid.cells().size());
    for (int c : doc.grid.cells())
        lv.cells.push_back(static_cast<soldier::Tile>(std::clamp(c, 0, soldier::kTileCount - 1)));
    return lv;
}

vs::TileDocument level_to_document(const soldier::Level& lv) {
    vs::TileDocument doc{vs::TileGrid(lv.width, lv.height, 0), {{"number", lv.number}, {"time_limit", lv.time_limit}}};
    for (int y = 0; y < lv.height; ++y)
        for (int x = 0; x < lv.width; ++x) doc.grid.set(x, y, static_cast<int>(lv.at({x, y})));
    return doc;
}

soldier::Level parse_level_lenient(std::string_view text) {
    using soldier::FormatError;
    soldier::Level lv;
    try {
        const nlohmann::json doc = nlohmann::json::parse(text);
        lv.number = doc.at("number").get<int>();
        lv.width = doc.at("width").get<int>();
        lv.height = doc.at("height").get<int>();
        lv.time_limit = doc.at("time_limit").get<int>();
        if (lv.width < 1 || lv.height < 1 || lv.width > soldier::kMaxLevelSize || lv.height > soldier::kMaxLevelSize)
            throw FormatError("level size out of range (1..100)");
        if (lv.time_limit < 0) throw FormatError("negative time_limit");
        const auto& rows = doc.at("rows");
        if (!rows.is_array() || static_cast<int>(rows.size()) != lv.height)
            throw FormatError("the number of rows differs from height");
        lv.cells.reserve(static_cast<std::size_t>(lv.width * lv.height));
        for (const auto& row : rows) {
            const auto s = row.get<std::string>();
            if (static_cast<int>(s.size()) != lv.width) throw FormatError("a row's length differs from width");
            for (char c : s) {
                const auto t = soldier::tile_from_char(c);
                if (!t) throw FormatError(std::string("unknown tile character '") + c + "'");
                lv.cells.push_back(*t);
            }
        }
    } catch (const nlohmann::json::exception& e) {
        throw FormatError(std::string("level JSON: ") + e.what());
    }
    return lv;
}

std::string level_file_name(int number) {
    char buf[32];
    std::snprintf(buf, sizeof buf, "%02d.json", number);
    return buf;
}

std::string file_number_warning(const std::filesystem::path& path, int number) {
    const std::string stem = vs::path_to_utf8(path.stem());
    if (stem.empty() || stem.size() > 4 || !std::all_of(stem.begin(), stem.end(), [](char c) { return c >= '0' && c <= '9'; }))
        return {};
    const int file_number = std::stoi(stem);
    if (file_number == number) return {};
    return "The level number is " + std::to_string(number) + " but the file is " + vs::path_to_utf8(path.filename()) +
           ": the game uses the file name (level " + std::to_string(file_number) + ") for progress and records. Save as " +
           level_file_name(number) + " or change the number.";
}

std::string document_to_json(const vs::TileDocument& doc) { return soldier::level_to_json(document_to_level(doc)); }

vs::TileDocument load_document(const std::filesystem::path& path) {
    const std::string text = vs::read_text_file(path);  // throws vs::Error
    try {
        return level_to_document(parse_level_lenient(text));
    } catch (const soldier::FormatError& e) {
        throw vs::Error(vs::path_to_utf8(path.filename()) + ": " + e.what());
    }
}

void save_document(const std::filesystem::path& path, const vs::TileDocument& doc) {
    vs::write_file_atomic(path, document_to_json(doc));
}

RoundTripResult round_trip_directory(const std::filesystem::path& dir) {
    RoundTripResult r;
    std::vector<std::filesystem::path> files;
    for (const auto& e : std::filesystem::directory_iterator(dir))
        if (e.is_regular_file() && e.path().extension() == ".json") files.push_back(e.path());
    std::sort(files.begin(), files.end());
    for (const auto& f : files) {
        ++r.files;
        const std::string name = vs::path_to_utf8(f.filename());
        try {
            const std::string original = vs::read_text_file(f);
            // Through the editor's own loader, and also check that soldier_core accepts the result.
            const std::string saved = document_to_json(level_to_document(parse_level_lenient(original)));
            (void)soldier::parse_level_json(saved);
            if (saved == original) {
                ++r.identical;
            } else {
                std::size_t i = 0;
                while (i < saved.size() && i < original.size() && saved[i] == original[i]) ++i;
                r.failures.push_back(name + ": differs at byte " + std::to_string(i));
            }
        } catch (const std::exception& e) {
            r.failures.push_back(name + ": " + e.what());
        }
    }
    return r;
}

}  // namespace soldier_editor
