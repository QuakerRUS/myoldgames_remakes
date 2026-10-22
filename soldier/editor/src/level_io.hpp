// Conversion between the editor document and the Soldier level JSON (docs/formats/soldier-level.md).
// The editor's TileDocument holds the tile codes in its grid and {"number", "time_limit"} in its
// properties. Saving uses soldier::level_to_json, so files are written exactly as the importer
// and soldier_core write them. SDL-free.
#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include <vs/editor/tile_document.hpp>

#include "soldier/level.hpp"

namespace soldier_editor {

inline constexpr int kDefaultWidth = 20;
inline constexpr int kDefaultHeight = 15;

int doc_number(const vs::TileDocument& doc);
int doc_time_limit(const vs::TileDocument& doc);

// A 20x15 level with a wall border, number 1, no time limit.
vs::TileDocument make_new_document(int width = kDefaultWidth, int height = kDefaultHeight);

soldier::Level document_to_level(const vs::TileDocument& doc);
vs::TileDocument level_to_document(const soldier::Level& level);

// Parses level JSON with the same checks as soldier::parse_level_json except that a missing
// player start is allowed (an unfinished level can be saved and reopened). Throws soldier::FormatError.
soldier::Level parse_level_lenient(std::string_view json_text);

// "NN.json" for a level number (01.json .. 99.json, 100.json ...): how the game names levels.
std::string level_file_name(int number);
// A warning when `path` is named like a game level (digits only, e.g. 07.json) but `number`
// differs from that NN (the game then uses the file name's number); empty otherwise.
std::string file_number_warning(const std::filesystem::path& path, int number);

std::string document_to_json(const vs::TileDocument& doc);
vs::TileDocument load_document(const std::filesystem::path& path);         // throws vs::Error
void save_document(const std::filesystem::path& path, const vs::TileDocument& doc);  // atomic

// Loads and re-saves (in memory) every *.json in `dir`, comparing the bytes with the file.
struct RoundTripResult {
    int files = 0;
    int identical = 0;
    std::vector<std::string> failures;  // "07.json: differs at byte 120"
};
RoundTripResult round_trip_directory(const std::filesystem::path& dir);

}  // namespace soldier_editor
