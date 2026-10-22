#pragma once
// Basic Piton types: field size, cells, directions.

#include <cstdint>
#include <optional>
#include <string_view>

namespace piton {

inline constexpr int kFieldWidth = 38;        // cells, x = 0..37 (left to right)
inline constexpr int kFieldHeight = 23;       // cells, y = 0..22 (top to bottom)
inline constexpr int kStartLength = 3;        // snake segments at the start of a level
inline constexpr int kRabbitsPerLevel = 50;   // rabbits to eat to complete a level
inline constexpr int kLevelCount = 10;

// Directions use the original game's numbering: 1..4 = up, right, down, left.
enum class Dir : std::uint8_t { Up = 1, Right = 2, Down = 3, Left = 4 };

inline constexpr Dir kAllDirs[4] = {Dir::Up, Dir::Right, Dir::Down, Dir::Left};

struct Cell {
    int x = 0;
    int y = 0;
    friend constexpr bool operator==(Cell, Cell) = default;
};

constexpr Dir opposite(Dir d) {
    switch (d) {
    case Dir::Up: return Dir::Down;
    case Dir::Right: return Dir::Left;
    case Dir::Down: return Dir::Up;
    case Dir::Left: return Dir::Right;
    }
    return Dir::Up;
}

constexpr int dx(Dir d) { return d == Dir::Right ? 1 : d == Dir::Left ? -1 : 0; }
constexpr int dy(Dir d) { return d == Dir::Down ? 1 : d == Dir::Up ? -1 : 0; }

// The neighbouring cell in direction d (may be outside the field).
constexpr Cell step(Cell c, Dir d) { return Cell{c.x + dx(d), c.y + dy(d)}; }

constexpr bool insideField(Cell c) {
    return c.x >= 0 && c.x < kFieldWidth && c.y >= 0 && c.y < kFieldHeight;
}

// Row-major index 0..kFieldWidth*kFieldHeight-1; only for cells inside the field.
constexpr int cellIndex(Cell c) { return c.y * kFieldWidth + c.x; }
constexpr Cell cellAt(int index) { return Cell{index % kFieldWidth, index / kFieldWidth}; }
inline constexpr int kCellCount = kFieldWidth * kFieldHeight;

// Direction from a to b when they are orthogonal neighbours.
constexpr std::optional<Dir> dirBetween(Cell a, Cell b) {
    for (Dir d : kAllDirs)
        if (step(a, d) == b) return d;
    return std::nullopt;
}

// "up", "right", "down", "left".
constexpr std::string_view dirName(Dir d) {
    switch (d) {
    case Dir::Up: return "up";
    case Dir::Right: return "right";
    case Dir::Down: return "down";
    case Dir::Left: return "left";
    }
    return "up";
}

constexpr std::optional<Dir> dirFromName(std::string_view s) {
    for (Dir d : kAllDirs)
        if (dirName(d) == s) return d;
    return std::nullopt;
}

} // namespace piton
