// Level validation for the editor. SDL-free.
#pragma once

#include <string>
#include <vector>

#include <vs/editor/tile_document.hpp>

namespace soldier_editor {

struct CheckResult {
    std::vector<std::string> errors;    // block test-play: no player start, no exit
    std::vector<std::string> warnings;  // extra player tiles, unreachable exit, doors without buttons / keys
};

// Start tiles as the game scans them: player on floor / button / door / under a guillotine.
bool is_start_code(int code);
int count_starts(const vs::TileGrid& grid);
// The start as the game picks it: the last start tile in row-major order; {-1, -1} if none.
struct CellPos {
    int x = -1, y = -1;
    bool operator==(const CellPos&) const = default;
};
CellPos find_start(const vs::TileGrid& grid);

CheckResult check_level(const vs::TileDocument& doc);

}  // namespace soldier_editor
