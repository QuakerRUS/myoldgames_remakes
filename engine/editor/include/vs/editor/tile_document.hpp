#pragma once
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include <vs/engine/undo_stack.hpp>

namespace vs {

// Rectangular grid of integer tile codes, row-major. SDL-free.
class TileGrid {
public:
    TileGrid() = default;
    TileGrid(int width, int height, int fill = 0);

    int width() const { return width_; }
    int height() const { return height_; }
    bool in_bounds(int x, int y) const { return x >= 0 && y >= 0 && x < width_ && y < height_; }
    // Precondition: in_bounds(x, y).
    int at(int x, int y) const { return cells_[static_cast<std::size_t>(y * width_ + x)]; }
    void set(int x, int y, int value) { cells_[static_cast<std::size_t>(y * width_ + x)] = value; }
    // New size; the overlapping top-left part is kept, new cells get `fill`.
    void resize(int width, int height, int fill = 0);
    void fill(int value);
    int count(int value) const;
    const std::vector<int>& cells() const { return cells_; }
    bool operator==(const TileGrid&) const = default;

private:
    int width_ = 0;
    int height_ = 0;
    std::vector<int> cells_;
};

// What a map editor edits: the grid plus free-form properties (time limit, level number,
// start direction...) defined by the game's plugin.
struct TileDocument {
    TileGrid grid;
    nlohmann::json properties = nlohmann::json::object();
    bool operator==(const TileDocument&) const = default;
};

// A cell position.
struct GridPos {
    int x = 0;
    int y = 0;
    bool operator==(const GridPos&) const = default;
};

struct CellChange {
    int x = 0;
    int y = 0;
    int value = 0;
    bool operator==(const CellChange&) const = default;
};

// Sets cells; remembers the previous values. Commands with the same non-zero stroke id merge
// (one undo step per brush stroke) when pushed with allow_merge = true.
class CellsCommand : public Command {
public:
    CellsCommand(TileDocument& doc, std::vector<CellChange> changes, std::string label = "Place", int stroke = 0);
    void apply() override;
    void revert() override;
    std::string label() const override { return label_; }
    bool merge(const Command& next) override;

private:
    TileDocument& doc_;
    std::vector<CellChange> changes_;
    std::vector<int> old_;
    std::string label_;
    int stroke_;
};

// Replaces the whole document (resize, property edits, clear...).
class DocumentCommand : public Command {
public:
    DocumentCommand(TileDocument& doc, TileDocument after, std::string label);
    void apply() override;
    void revert() override;
    std::string label() const override { return label_; }

private:
    TileDocument& doc_;
    TileDocument before_;
    TileDocument after_;
    std::string label_;
};

}  // namespace vs
