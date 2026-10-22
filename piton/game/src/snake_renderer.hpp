#pragma once
// Draws the snake from its cells with smooth sub-tick motion.
//
// Between two ticks (progress t in [0, 1)):
// - straight neck: the head slides from prevHead into the head cell; the neck's straight piece
//   is clipped just behind the head's middle so nothing pokes out in front of the snout;
// - turning neck: the head swings around the corner's pivot (the cell corner between the
//   neck's entry and exit edges) by 90 * t degrees; the neck's turn piece is shown only in the
//   swept sector, so the body grows along the curve behind the head;
// - the tail slides out of prevTail (straight) or swings around the corner of the tail cell,
//   and the tail cell's piece is shown only ahead of the tail's wide end.
// At t = 1 every pose equals the next tick's pose at t = 0, so the motion is continuous.

#include <string>
#include <vector>

#include <vs/engine/renderer.hpp>
#include <vs/engine/sprite_sheet.hpp>

#include <piton/types.hpp>

namespace piton_game {

struct SnakePose {
    std::vector<piton::Cell> body;  // [0] = head ... back() = tail (at least 3 cells)
    piton::Dir heading = piton::Dir::Right;  // where the head faces when it is not moving
    piton::Cell prev_head{};                 // == body[0]: the head does not move (start, death)
    piton::Cell prev_tail{};                 // == body.back(): the tail does not move (growth)
    float progress = 0.0f;                   // 0..1 between the last tick and the next
    // Head frame of the right-facing set ("head_r", "head_r_tongue1", "head_r_eat", ...);
    // the renderer swaps the direction letter.
    std::string head_frame = "head_r";
};

// Draws the snake with cell (0,0) at `origin`.
void draw_snake(vs::Renderer& r, const vs::SpriteSheet& sheet, vs::Vec2 origin, const SnakePose& pose);

// "u" / "r" / "d" / "l".
const char* dir_letter(piton::Dir d);

// The pose of a snake in a piton::Game, e.g. for the playfield.
SnakePose pose_from_body(const std::vector<piton::Cell>& body, piton::Dir heading, piton::Cell prev_head,
                         piton::Cell prev_tail, float progress, std::string head_frame);

}  // namespace piton_game
