// The Soldier palette: which tiles the editor offers, how they are grouped, their names and
// the hint text explaining what each tile does in the game. SDL-free.
#pragma once

#include <string>
#include <string_view>
#include <vector>

#include <vs/editor/palette_grid.hpp>

namespace soldier_editor {

struct PaletteEntry {
    int code = 0;
    std::string name;  // "Box on button"
};

struct PaletteGroup {
    std::string label;  // "Boxes, buttons and doors"
    std::vector<PaletteEntry> entries;
};

// Every meaningful tile, grouped. Death tiles and the "player on ..." variants are not offered
// (a player is combined with the ground automatically when placed); they survive loading.
const std::vector<PaletteGroup>& palette_groups();

// Number of icons per palette row (each group starts a new row).
inline constexpr int kPaletteColumns = 8;

// Human-readable name for any code 0..63 ("Player on button (left)"); "Unknown" otherwise.
std::string tile_display_name(int code);
// What the tile does in the game (one or two sentences), for any code 0..63.
std::string_view tile_hint(int code);

// Position of each entry of the flattened palette (group order, then entry order) in a grid
// where every group starts on a new row with kPaletteColumns icons per row.
using PaletteCell = vs::PaletteCell;
std::vector<PaletteCell> palette_layout(int columns = kPaletteColumns);
// Flattened palette (index -> entry).
std::vector<PaletteEntry> flat_palette();
// Number of entries of each group.
std::vector<int> palette_group_sizes();

// Keyboard navigation in the palette (vs::palette_move with wrapping): Left/Right step through the
// flattened list, Up/Down go to the nearest icon of the previous/next row; every direction wraps.
int palette_move(int index, int dx, int dy, int columns = kPaletteColumns);

}  // namespace soldier_editor
