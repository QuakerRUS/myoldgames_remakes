#include "turn_sector.hpp"

#include <SDL3/SDL_render.h>

#include <algorithm>
#include <cmath>
#include <numbers>

#include "field_renderer.hpp"

namespace piton_game {
namespace {

vs::Vec2 dir_vec(piton::Dir d) { return {static_cast<float>(piton::dx(d)), static_cast<float>(piton::dy(d))}; }

char side_letter(piton::Dir d) {
    switch (d) {
    case piton::Dir::Up: return 'n';
    case piton::Dir::Down: return 's';
    case piton::Dir::Left: return 'w';
    case piton::Dir::Right: return 'e';
    }
    return 'n';
}

bool vertical(piton::Dir d) { return d == piton::Dir::Up || d == piton::Dir::Down; }

}  // namespace

TurnGeometry turn_geometry(piton::Dir in, piton::Dir out) {
    TurnGeometry g;
    const piton::Dir entry_side = piton::opposite(in);
    // The pivot is the cell corner on both the entry side and the exit side.
    for (piton::Dir s : {entry_side, out}) {
        if (s == piton::Dir::Left) g.pivot.x = 0;
        if (s == piton::Dir::Right) g.pivot.x = kCell;
        if (s == piton::Dir::Up) g.pivot.y = 0;
        if (s == piton::Dir::Down) g.pivot.y = kCell;
    }
    g.u0 = dir_vec(piton::opposite(out));
    g.u1 = dir_vec(in);
    // Screen coordinates (y down): rotating (x, y) clockwise by 90 degrees gives (-y, x).
    g.sign = (g.u1.x == -g.u0.y && g.u1.y == g.u0.x) ? 1.0f : -1.0f;
    return g;
}

const char* turn_frame(piton::Dir in, piton::Dir out) {
    const piton::Dir a = piton::opposite(in);
    const char ns = side_letter(vertical(a) ? a : out);
    const char ew = side_letter(vertical(a) ? out : a);
    if (ns == 'n') return ew == 'e' ? "turn_ne" : "turn_nw";
    return ew == 'e' ? "turn_se" : "turn_sw";
}

void draw_turn_sector(vs::Renderer& r, const vs::SpriteSheet& sheet, const char* frame, vs::Vec2 cell_pos,
                      const TurnGeometry& g, float t0, float t1) {
    t0 = std::clamp(t0, 0.0f, 1.0f);
    t1 = std::clamp(t1, 0.0f, 1.0f);
    if (t1 - t0 <= 0.001f) return;
    const vs::FrameRect* f = sheet.frame(frame);
    if (!f) return;
    const vs::Texture& tex = sheet.texture();

    // Polygon in (a, b) coordinates: a along u0, b along u1; the cell is [0,24] x [0,24].
    const float half_pi = std::numbers::pi_v<float> * 0.5f;
    auto hit = [&](float t) {
        const float phi = t * half_pi;
        const float c = std::cos(phi);
        const float s = std::sin(phi);
        const float len = kCell / std::max(c, s);
        return vs::Vec2{c * len, s * len};
    };
    vs::Vec2 poly[4];
    int count = 0;
    poly[count++] = {0, 0};
    poly[count++] = hit(t0);
    if (t0 < 0.5f && t1 > 0.5f) poly[count++] = {kCell, kCell};
    poly[count++] = hit(t1);

    SDL_Vertex verts[4];
    const float tw = static_cast<float>(tex.width());
    const float th = static_cast<float>(tex.height());
    for (int i = 0; i < count; ++i) {
        const vs::Vec2 local{g.pivot.x + poly[i].x * g.u0.x + poly[i].y * g.u1.x,
                             g.pivot.y + poly[i].x * g.u0.y + poly[i].y * g.u1.y};
        verts[i].position = {cell_pos.x + local.x, cell_pos.y + local.y};
        verts[i].color = {1.0f, 1.0f, 1.0f, 1.0f};
        verts[i].tex_coord = {(static_cast<float>(f->x) + local.x) / tw, (static_cast<float>(f->y) + local.y) / th};
    }
    int indices[6];
    int n = 0;
    for (int i = 1; i + 1 < count; ++i) {
        indices[n++] = 0;
        indices[n++] = i;
        indices[n++] = i + 1;
    }
    SDL_SetTextureColorMod(tex.sdl(), 255, 255, 255);
    SDL_SetTextureAlphaMod(tex.sdl(), 255);
    SDL_RenderGeometry(r.sdl(), tex.sdl(), verts, count, indices, n);
}

}  // namespace piton_game
