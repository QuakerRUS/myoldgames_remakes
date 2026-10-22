// A Soldier level: a grid of tile codes plus the time limit.
#pragma once

#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "soldier/tile.hpp"

namespace soldier {

// Thrown by every loader on malformed input.
class FormatError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

struct Point {
    int x = 0, y = 0;
    friend bool operator==(Point, Point) = default;
};

inline constexpr int kMaxLevelSize = 100;  // the original's ml(100, 100)

struct Level {
    int number = 0;
    int width = 0;
    int height = 0;
    int time_limit = 0;       // timer units; 0 = no limit
    std::vector<Tile> cells;  // row-major, cells[y * width + x], x/y are 0-based

    bool in_bounds(Point p) const { return p.x >= 0 && p.y >= 0 && p.x < width && p.y < height; }
    Tile at(Point p) const { return cells[static_cast<std::size_t>(p.y * width + p.x)]; }
    Tile& at(Point p) { return cells[static_cast<std::size_t>(p.y * width + p.x)]; }

    // Start position: the LAST player tile in row-major order (codes 3..6, 11..14, 16..19, 49..52),
    // exactly as the original scan did. Player-looking tiles elsewhere (levels 14 and 27) and
    // tiles 57..60 stay as inert decorations. Throws FormatError if there is no start.
    Point start() const;
};

// Level JSON (docs/formats/soldier-level.md). Throws FormatError.
Level parse_level_json(std::string_view text);
Level load_level_file(const std::string& path);
std::string level_to_json(const Level& level);

// Reads a whole file into a string; throws FormatError if it cannot be opened.
std::string read_text_file(const std::string& path);

}  // namespace soldier
