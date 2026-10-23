#include "snake_renderer.hpp"

#include <algorithm>
#include <cmath>
#include <string>

#include "field_renderer.hpp"
#include "turn_sector.hpp"

namespace piton_game {
namespace {

using piton::Cell;
using piton::Dir;

vs::Vec2 dir_vec(Dir d) { return {static_cast<float>(piton::dx(d)), static_cast<float>(piton::dy(d))}; }

vs::Vec2 cell_pos(vs::Vec2 origin, Cell c) {
    return {origin.x + static_cast<float>(c.x) * kCell, origin.y + static_cast<float>(c.y) * kCell};
}

bool vertical(Dir d) { return d == Dir::Up || d == Dir::Down; }

// The skin pattern repeats every kSkinPeriod px; snake.png holds kSkinPeriod crawl phases of
// every body piece ("body_h", "body_h_p1" ... "body_h_p11"), phase k moved k px along the
// piece's own direction: right / down for straight pieces, from the n/s edge to the e/w edge
// for turns (SKIN_PHASES in gen_piton_gfx.py).
constexpr int kSkinPeriod = 12;

// Frame of a body piece with the skin moved `shift` px along the snake's motion; `forward`
// is true when the snake moves along the piece's own direction.
std::string skin_frame(const char* base, int shift, bool forward) {
    const int s = shift % kSkinPeriod;
    const int k = forward ? s : (kSkinPeriod - s) % kSkinPeriod;
    return k == 0 ? std::string(base) : std::string(base) + "_p" + std::to_string(k);
}

std::string straight_frame(Dir d, int shift) {
    return skin_frame(vertical(d) ? "body_v" : "body_h", shift, d == Dir::Right || d == Dir::Down);
}

// Entering moving `in`: the snake runs from the n/s edge iff `in` is vertical.
std::string turn_skin_frame(Dir in, Dir out, int shift) { return skin_frame(turn_frame(in, out), shift, vertical(in)); }

// Local rectangle of the part of a cell between distances `from` and `to` (pixels, 0..24)
// from the edge where a snake moving `d` enters it.
vs::RectF span_rect(Dir d, float from, float to) {
    from = std::clamp(from, 0.0f, kCell);
    to = std::clamp(to, 0.0f, kCell);
    switch (d) {
    case Dir::Right: return {from, 0, to - from, kCell};
    case Dir::Left: return {kCell - to, 0, to - from, kCell};
    case Dir::Down: return {0, from, kCell, to - from};
    case Dir::Up: return {0, kCell - to, kCell, to - from};
    }
    return {};
}

// Draws the part `local` (cell pixels) of a 24x24 frame placed at `pos`.
void draw_part(vs::Renderer& r, const vs::SpriteSheet& sheet, const std::string& frame, vs::Vec2 pos,
               vs::RectF local) {
    const vs::FrameRect* f = sheet.frame(frame);
    if (!f || local.w <= 0 || local.h <= 0) return;
    const vs::RectF src{static_cast<float>(f->x) + local.x, static_cast<float>(f->y) + local.y, local.w, local.h};
    r.draw_stretched(sheet.texture(), src, {pos.x + local.x, pos.y + local.y, local.w, local.h});
}

// Middle piece of a cell entered moving `in` and left moving `out`.
void draw_piece(vs::Renderer& r, const vs::SpriteSheet& sheet, vs::Vec2 pos, Dir in, Dir out, int shift) {
    sheet.draw(r, in == out ? straight_frame(in, shift) : turn_skin_frame(in, out, shift), pos);
}

std::string head_frame_for(const std::string& right_frame, Dir d) {
    // "head_r..." -> "head_<d>..."
    std::string f = right_frame;
    if (f.size() >= 6 && f.compare(0, 6, "head_r") == 0) f[5] = dir_letter(d)[0];
    return f;
}

Dir dir_or(Cell a, Cell b, Dir fallback) { return piton::dirBetween(a, b).value_or(fallback); }

}  // namespace

const char* dir_letter(Dir d) {
    switch (d) {
    case Dir::Up: return "u";
    case Dir::Right: return "r";
    case Dir::Down: return "d";
    case Dir::Left: return "l";
    }
    return "r";
}

SnakePose pose_from_body(const std::vector<Cell>& body, Dir heading, Cell prev_head, Cell prev_tail, float progress,
                         std::string head_frame) {
    SnakePose p;
    p.body = body;
    p.heading = heading;
    p.prev_head = prev_head;
    p.prev_tail = prev_tail;
    p.progress = progress;
    p.head_frame = std::move(head_frame);
    return p;
}

void draw_snake(vs::Renderer& r, const vs::SpriteSheet& sheet, vs::Vec2 origin, const SnakePose& pose) {
    const std::vector<Cell>& body = pose.body;
    const std::size_t n = body.size();
    if (n < 3) return;
    const float t = std::clamp(pose.progress, 0.0f, 1.0f);
    const float slide = std::round(t * kCell);  // whole pixels: no shimmering

    // Directions: out(i) towards the head-side neighbour, in(i) from the tail-side neighbour.
    auto out_dir = [&](std::size_t i) { return i == 0 ? pose.heading : dir_or(body[i], body[i - 1], pose.heading); };
    auto in_dir = [&](std::size_t i) { return i + 1 < n ? dir_or(body[i + 1], body[i], out_dir(i)) : out_dir(i); };

    const bool head_moving = piton::dirBetween(pose.prev_head, body[0]).has_value() && pose.prev_head == body[1];
    const Dir neck_in = in_dir(1);
    const Dir neck_out = out_dir(1);
    const bool head_swing = head_moving && neck_in != neck_out;
    // The skin crawls through every cell with the head: the snake moves as a whole.
    const int shift = head_moving ? static_cast<int>(slide) : 0;

    // --- Middle pieces (neck .. the segment before the tail) --------------------------------
    for (std::size_t i = n - 2; i >= 1; --i) {
        const vs::Vec2 pos = cell_pos(origin, body[i]);
        if (i == 1 && head_moving) {
            if (head_swing) {
                // The curve grows behind the swinging head (a little extra hides under it).
                draw_turn_sector(r, sheet, turn_skin_frame(neck_in, neck_out, shift).c_str(), pos,
                                 turn_geometry(neck_in, neck_out), 0.0f, t + 0.12f);
            } else {
                // Straight neck: only up to the middle of the sliding head.
                draw_part(r, sheet, straight_frame(neck_out, shift), pos, span_rect(neck_out, 0, slide + kCell * 0.5f));
            }
        } else {
            draw_piece(r, sheet, pos, in_dir(i), out_dir(i), shift);
        }
        if (i == 1) break;
    }

    // --- Tail ------------------------------------------------------------------------------
    const Cell tail = body[n - 1];
    const vs::Vec2 tail_pos = cell_pos(origin, tail);
    const Dir to_next = out_dir(n - 1);  // from the tail cell towards the rest of the body
    const std::optional<Dir> tail_motion = piton::dirBetween(pose.prev_tail, tail);
    if (!tail_motion) {
        sheet.draw(r, std::string("tail_") + dir_letter(piton::opposite(to_next)), tail_pos);
    } else if (*tail_motion == to_next) {
        // Straight: the body continues in front of the tail's wide end.
        const Dir d = *tail_motion;
        draw_part(r, sheet, straight_frame(d, shift), tail_pos, span_rect(d, slide - 3.0f, kCell));
        const vs::Vec2 v = dir_vec(d);
        const vs::Vec2 prev = cell_pos(origin, pose.prev_tail);
        sheet.draw(r, std::string("tail_") + dir_letter(piton::opposite(d)), {prev.x + v.x * slide, prev.y + v.y * slide});
    } else {
        // Corner: the tail swings around the pivot of the tail cell; the curve ahead of it stays.
        const Dir d1 = *tail_motion;
        const TurnGeometry g = turn_geometry(d1, to_next);
        draw_turn_sector(r, sheet, turn_skin_frame(d1, to_next, shift).c_str(), tail_pos, g, t - 0.06f, 1.0f);
        const vs::Vec2 v = dir_vec(d1);
        vs::DrawParams p;
        p.origin = vs::Vec2{g.pivot.x + v.x * kCell, g.pivot.y + v.y * kCell};
        p.rotation = g.sign * 90.0f * t;
        sheet.draw(r, std::string("tail_") + dir_letter(piton::opposite(d1)),
                   {tail_pos.x + g.pivot.x, tail_pos.y + g.pivot.y}, p);
    }

    // --- Head ------------------------------------------------------------------------------
    if (!head_moving) {
        sheet.draw(r, head_frame_for(pose.head_frame, pose.heading), cell_pos(origin, body[0]));
    } else if (!head_swing) {
        const vs::Vec2 v = dir_vec(neck_out);
        const vs::Vec2 prev = cell_pos(origin, pose.prev_head);
        sheet.draw(r, head_frame_for(pose.head_frame, neck_out), {prev.x + v.x * slide, prev.y + v.y * slide});
    } else {
        // Swing from the neck cell (facing the old direction) into the head cell.
        const TurnGeometry g = turn_geometry(neck_in, neck_out);
        const vs::Vec2 neck_pos = cell_pos(origin, body[1]);
        vs::DrawParams p;
        p.origin = g.pivot;
        p.rotation = g.sign * 90.0f * t;
        sheet.draw(r, head_frame_for(pose.head_frame, neck_in), {neck_pos.x + g.pivot.x, neck_pos.y + g.pivot.y}, p);
    }
}

}  // namespace piton_game
