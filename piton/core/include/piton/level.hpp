#pragma once
// A Piton level: 38x23 grid of empty/brick cells plus the snake's start.

#include "piton/types.hpp"

#include <array>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace piton {

enum class Tile : std::uint8_t { Empty = 0, Brick = 1 };

class Level {
public:
    // An empty field; start head at (20, 18) facing right (body at (19,18), (18,18)).
    Level();

    std::string name;  // shown in the game ("Open Field", ...); may be empty

    // Cells outside the field read as Brick (the border kills like a brick).
    Tile tile(Cell c) const;
    bool isBrick(Cell c) const { return tile(c) == Tile::Brick; }
    bool isFree(Cell c) const { return tile(c) == Tile::Empty; }  // inside and empty
    void setTile(Cell c, Tile t);  // ignored outside the field
    void fill(Tile t);

    // Snake start: head cell and the direction it is moving. The other kStartLength-1
    // segments lie straight behind the head (opposite to startDir).
    Cell startHead() const { return startHead_; }
    Dir startDir() const { return startDir_; }
    // Moves the start. Refused (returns false, the start is unchanged) when any of the
    // kStartLength cells would lie outside the field (see startFits / clampStart). Bricks under
    // an accepted start are removed, so the start cells are free afterwards.
    bool setStart(Cell head, Dir dir);
    // Head first, tail last.
    std::array<Cell, kStartLength> startCells() const { return startCellsFor(startHead_, startDir_); }

    // The start cells for a head and direction (head first; may lie outside the field).
    static std::array<Cell, kStartLength> startCellsFor(Cell head, Dir dir);
    // Whether all start cells for (head, dir) lie inside the field.
    static bool startFits(Cell head, Dir dir);
    // The nearest head cell for which the start snake facing `dir` fits inside the field.
    static Cell clampStart(Cell head, Dir dir);

    int freeCellCount() const;

    // JSON (see docs/formats/piton-level.md). On failure returns false and sets *error.
    // A start whose cells do not all lie inside the field is an error; bricks under the start
    // cells are removed (repaired) on load.
    static bool fromJson(std::string_view json, Level& out, std::string* error = nullptr);
    std::string toJson() const;
    static bool load(const std::filesystem::path& file, Level& out, std::string* error = nullptr);
    bool save(const std::filesystem::path& file, std::string* error = nullptr) const;

    // The rows as text: kFieldHeight strings of kFieldWidth chars, '#' brick, '.' empty.
    std::vector<std::string> rows() const;

    friend bool operator==(const Level&, const Level&) = default;

private:
    std::array<Tile, kCellCount> tiles_{};
    Cell startHead_{20, 18};
    Dir startDir_ = Dir::Right;
};

// Result of the level checks used by the tests and the map editor.
struct LevelCheck {
    bool ok() const { return problems.empty(); }
    // No hard errors: the level can be played (and completed). Warnings may remain.
    bool playable() const { return errors.empty(); }
    std::vector<std::string> problems;  // all of them (errors and warnings), human-readable, English
    std::vector<std::string> errors;    // hard errors: the level cannot be played or completed
    std::vector<std::string> warnings;  // design rules (room ahead, size, dead ends)
    int freeCells = 0;
    int regions = 0;          // connected regions of empty cells
    int longestDeadEnd = 0;   // longest 1-cell-wide dead-end corridor (cells)
};

inline constexpr int kMinFreeAhead = 6;      // free cells straight ahead of the start head
inline constexpr int kMaxDeadEndLength = 3;  // max 1-wide dead-end corridor length
// Free area needed: the full-grown snake (3 + 50) times this factor.
inline constexpr int kMinFreeCells = 3 * (kStartLength + kRabbitsPerLevel);

// Checks. Hard errors: start cells outside the field or not free; no free cell for a rabbit;
// the empty cells do not form exactly one 4-connected region (a rabbit could appear where
// the snake cannot reach it). Warnings: fewer than kMinFreeAhead free cells ahead of the head;
// fewer than kMinFreeCells free cells; a 1-cell-wide dead-end corridor longer than
// kMaxDeadEndLength.
LevelCheck checkLevel(const Level& level);

// "01.json".."10.json" for level number 1..10.
std::string levelFileName(int number);

} // namespace piton
