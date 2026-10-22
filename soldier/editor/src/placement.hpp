// Cell placement rules of the Soldier editor. SDL-free.
#pragma once

#include <string>
#include <vector>

#include <vs/editor/tile_document.hpp>

namespace soldier_editor {

// Cells to change when `code` is placed at (x, y):
// - a player start keeps the ground of the target cell (button -> player on button, plain door
//   -> player in door, guillotine -> player under guillotine); the current start (the last start
//   tile in row-major order) is moved, i.e. turned back into its ground, while the other player
//   tiles (decorations, as in levels 14 and 27) are kept. If the new tile is not the last one in
//   row-major order the game will not use it as the start: see start_placement_warning();
// - any other tile simply replaces the cell.
std::vector<vs::CellChange> placement_changes(const vs::TileDocument& doc, int x, int y, int code);

// Status-bar warning after placing a start at (x, y) in `doc` (already applied): empty if (x, y)
// is the start the game will use.
std::string start_placement_warning(const vs::TileDocument& doc, int x, int y);

// New size with the top-left content kept; new cells are floor. Size is clamped to 1..100.
vs::TileDocument resized(const vs::TileDocument& doc, int width, int height);

}  // namespace soldier_editor
