#pragma once
// Frame and animation list operations on a Sheet (SDL-free; each returns the edited copy or
// edits in place so the caller can wrap the change in a SheetCommand).
#include <optional>
#include <string>
#include <string_view>

#include "document.hpp"
#include "raster.hpp"

namespace painter {

// Tile grid used for snapping frame rectangles: cells of w x h with `gap` pixels between them.
struct TileGrid {
    int w = 32;
    int h = 32;
    int gap = 0;
    int pitch_x() const { return w + gap; }
    int pitch_y() const { return h + gap; }
};

// Guess from the sheet's frames (first frame's size; gap 1 when frames are 1 px apart).
TileGrid guess_tile_grid(const Sheet& sheet, TileGrid fallback);
// The smallest union of whole tiles that covers r.
IRect snap_to_tiles(const IRect& r, const TileGrid& grid);

// "base", "base_2", "base_3"... whichever is not yet a frame (or animation) name.
std::string unique_frame_name(const Sheet& sheet, std::string_view base);
std::string unique_animation_name(const Sheet& sheet, std::string_view base);

IRect frame_rect(const vs::FrameRect& f);
// Top-left of the first position on the tile grid where a w x h rectangle fits inside the
// image without overlapping any frame (row by row); nullopt if there is none.
std::optional<IRect> find_free_slot(const Sheet& sheet, int image_w, int image_h, int w, int h, const TileGrid& grid);

// Renames frame `index` and every animation reference to it. False if the name is empty or taken.
bool rename_frame(Sheet& sheet, std::size_t index, const std::string& name);
bool rename_animation(Sheet& sheet, std::size_t index, const std::string& name);
// Removes frame `index` and its animation entries; animations left empty are removed.
// Returns the number of animations removed.
int delete_frame(Sheet& sheet, std::size_t index);

// Index of the frame that shows before `frame` (onion skin): the previous entry of `anim`
// if it contains the frame, otherwise the previous frame of the list; -1 if none.
int previous_frame_index(const Sheet& sheet, int frame, int anim);

// Animation frame shown at `elapsed_ms` (loops wrap, others stop at the last frame).
std::size_t animation_entry_at(const vs::AnimationDef& anim, double elapsed_ms);

}  // namespace painter
