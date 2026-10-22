#pragma once
// Palette layout and keyboard navigation for grouped palettes (SDL-free, unit-tested).
#include <vector>

namespace vs {

struct PaletteCell {
    int row = 0;
    int col = 0;
    bool operator==(const PaletteCell&) const = default;
};

// Position of every entry of a palette made of groups of `group_sizes[i]` entries, `columns`
// per row, each group starting on a new row. Entries are numbered group by group.
std::vector<PaletteCell> palette_layout(const std::vector<int>& group_sizes, int columns);

// Keyboard navigation in that layout. With `wrap`: Left/Right step through the flattened list,
// Up/Down go to the nearest entry of the previous/next row, every direction wraps around.
// Without: index + dx + dy * columns, clamped (the classic flat palette).
int palette_move(const std::vector<int>& group_sizes, int columns, int index, int dx, int dy, bool wrap);

}  // namespace vs
