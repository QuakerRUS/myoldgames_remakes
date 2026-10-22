#pragma once
// Draws an angular slice of a 24x24 turn piece: the part between two rays from the turn's
// pivot (the corner shared by the piece's entry and exit edges). Used to grow or shrink a
// curved body piece behind a head or tail that swings around the corner.

#include <vs/engine/renderer.hpp>
#include <vs/engine/sprite_sheet.hpp>

#include <piton/types.hpp>

namespace piton_game {

// Geometry of a turn: the snake enters the cell moving `in` and leaves it moving `out`.
struct TurnGeometry {
    vs::Vec2 pivot;  // corner of the cell, local pixels (0 or 24 on each axis)
    vs::Vec2 u0;     // unit vector from the pivot along the entry edge (angle 0)
    vs::Vec2 u1;     // unit vector from the pivot along the exit edge (angle 90)
    float sign;      // +1 when u0 -> u1 is a clockwise (screen) rotation, else -1
};

TurnGeometry turn_geometry(piton::Dir in, piton::Dir out);

// Frame name of the turn piece for (in, out): "turn_ne", "turn_sw", ...
const char* turn_frame(piton::Dir in, piton::Dir out);

// Draws the slice t0..t1 (fractions of the 90-degree turn, 0 = entry edge, 1 = exit edge) of
// `frame` with the cell's top-left corner at `cell_pos`.
void draw_turn_sector(vs::Renderer& r, const vs::SpriteSheet& sheet, const char* frame, vs::Vec2 cell_pos,
                      const TurnGeometry& g, float t0, float t1);

}  // namespace piton_game
