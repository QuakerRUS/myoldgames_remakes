#include <vs/editor/tile_document.hpp>

#include <algorithm>

namespace vs {

TileGrid::TileGrid(int width, int height, int fill) { resize(width, height, fill); }

void TileGrid::resize(int width, int height, int fill) {
    width = std::max(width, 0);
    height = std::max(height, 0);
    std::vector<int> cells(static_cast<std::size_t>(width) * static_cast<std::size_t>(height), fill);
    for (int y = 0; y < std::min(height, height_); ++y)
        for (int x = 0; x < std::min(width, width_); ++x) cells[static_cast<std::size_t>(y * width + x)] = at(x, y);
    cells_ = std::move(cells);
    width_ = width;
    height_ = height;
}

void TileGrid::fill(int value) { std::fill(cells_.begin(), cells_.end(), value); }

int TileGrid::count(int value) const { return static_cast<int>(std::count(cells_.begin(), cells_.end(), value)); }

CellsCommand::CellsCommand(TileDocument& doc, std::vector<CellChange> changes, std::string label, int stroke)
    : doc_(doc), changes_(std::move(changes)), label_(std::move(label)), stroke_(stroke) {}

void CellsCommand::apply() {
    old_.clear();
    old_.reserve(changes_.size());
    for (const CellChange& c : changes_) {
        if (!doc_.grid.in_bounds(c.x, c.y)) {
            old_.push_back(c.value);
            continue;
        }
        old_.push_back(doc_.grid.at(c.x, c.y));
        doc_.grid.set(c.x, c.y, c.value);
    }
}

void CellsCommand::revert() {
    for (std::size_t i = changes_.size(); i-- > 0;) {
        const CellChange& c = changes_[i];
        if (doc_.grid.in_bounds(c.x, c.y)) doc_.grid.set(c.x, c.y, old_[i]);
    }
}

bool CellsCommand::merge(const Command& next) {
    const auto* n = dynamic_cast<const CellsCommand*>(&next);
    if (!n || stroke_ == 0 || n->stroke_ != stroke_ || &n->doc_ != &doc_) return false;
    changes_.insert(changes_.end(), n->changes_.begin(), n->changes_.end());
    old_.insert(old_.end(), n->old_.begin(), n->old_.end());
    return true;
}

DocumentCommand::DocumentCommand(TileDocument& doc, TileDocument after, std::string label)
    : doc_(doc), after_(std::move(after)), label_(std::move(label)) {}

void DocumentCommand::apply() {
    before_ = doc_;
    doc_ = after_;
}

void DocumentCommand::revert() { doc_ = before_; }

}  // namespace vs
