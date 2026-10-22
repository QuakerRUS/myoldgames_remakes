#include "tile_painter.hpp"

#include <algorithm>
#include <array>
#include <cfloat>
#include <cstdio>
#include <string>

#include <vs/engine/storage.hpp>
#include <vs/imgui/imgui_layer.hpp>

#include "soldier/tile.hpp"

namespace soldier_editor {
namespace {

using soldier::Tile;

constexpr std::array<const char*, 6> kColors = {"red", "green", "blue", "yellow", "white", "black"};
constexpr std::array<const char*, 4> kDirs = {"d", "u", "l", "r"};

bool is_wall_kind(int c) { return c == 1 || c == 45 || c == 46 || c == TilePainter::kOutside; }
bool is_dark_kind(int c) { return c == 45 || c == 46; }
bool is_watery(int c) { return c >= 54 && c <= 63; }  // water, drowned, sunk box (+ what stands on it)
bool on_guillotine(int c) { return c >= 48 && c <= 53; }

int anim_index(double time, int frame_ms, int frames) {
    return static_cast<int>(static_cast<long long>(time * 1000.0) / frame_ms % frames);
}

}  // namespace

void TilePainter::load(vs::Renderer& r) {
    tiles_ = vs::SpriteSheet::load(r, vs::asset_path("gfx/tiles.json"));
    tiles_smooth_ = vs::SpriteSheet::load(r, vs::asset_path("gfx/tiles.json"), vs::ScaleMode::Linear);
    player_ = vs::SpriteSheet::load(r, vs::asset_path("gfx/player.json"));
    player_smooth_ = vs::SpriteSheet::load(r, vs::asset_path("gfx/player.json"), vs::ScaleMode::Linear);
    loaded_ = true;
}

void TilePainter::frame(ImDrawList* draw, const vs::SpriteSheet& sheet, const char* name, ImVec2 min, float cell) const {
    const vs::FrameRect* f = sheet.frame(name);
    if (!f) return;
    const float s = cell / 32.0f;
    const float tw = static_cast<float>(sheet.texture().width());
    const float th = static_cast<float>(sheet.texture().height());
    const ImVec2 a{min.x - static_cast<float>(f->pivot_x) * s, min.y - static_cast<float>(f->pivot_y) * s};
    const ImVec2 b{a.x + static_cast<float>(f->w) * s, a.y + static_cast<float>(f->h) * s};
    draw->AddImage(vs::ImGuiLayer::texture_id(sheet.texture()), a, b, {f->x / tw, f->y / th},
                   {(f->x + f->w) / tw, (f->y + f->h) / th});
}

void TilePainter::draw_cell(ImDrawList* draw, const CellFn& at, int x, int y, ImVec2 min, float cell, double time) const {
    if (!loaded_) return;
    const int c = at(x, y);
    const vs::SpriteSheet& t = tiles(cell);
    char name[32];

    // Walls: autotile masks (1 = N, 2 = E, 4 = S, 8 = W). Outside the level counts as a plain
    // wall, as in the game's LevelView: plain walls join the map edge, dark walls do not.
    if (c == 1 || c == 45 || c == 46) {
        const bool dark = c != 1;
        auto same = [&](int dx, int dy) {
            const int n = at(x + dx, y + dy);
            return dark ? is_dark_kind(n) : (n == 1 || n == kOutside);
        };
        const int mask = (same(0, -1) ? 1 : 0) | (same(1, 0) ? 2 : 0) | (same(0, 1) ? 4 : 0) | (same(-1, 0) ? 8 : 0);
        std::snprintf(name, sizeof name, "%s_%d", dark ? "darkwall" : "wall", mask);
        frame(draw, t, name, min, cell);
        return;
    }

    // Ground.
    const int color = (c >= 21 && c <= 44) ? (c - 21) / 4 : -1;
    const int kind = color >= 0 ? (c - 21) % 4 : -1;  // 0 key, 1 coloured door, 2 key on button, 3 key in door
    const auto tile = static_cast<Tile>(c);
    const auto pground = soldier::player_ground(tile);
    if (c == 9) {
        frame(draw, t, "button_up", min, cell);
    } else if (c == 10 || kind == 2 || (pground && *pground == soldier::Ground::Button)) {
        frame(draw, t, "button_down", min, cell);
    } else if (c == 15) {
        frame(draw, t, "door_0", min, cell);
    } else if (c == 20 || kind == 3 || (pground && *pground == soldier::Ground::Door)) {
        frame(draw, t, "door_4", min, cell);
    } else if (kind == 1) {
        std::snprintf(name, sizeof name, "door_%s_0", kColors[static_cast<std::size_t>(color)]);
        frame(draw, t, name, min, cell);
    } else if (on_guillotine(c)) {
        frame(draw, t, "guillotine_base", min, cell);
    } else if (c == 54 || c == 55) {
        std::snprintf(name, sizeof name, "water_%d", anim_index(time, 140, 8));
        frame(draw, t, name, min, cell);
    } else if (c >= 56) {
        std::snprintf(name, sizeof name, "sunk_box_%d", anim_index(time, 140, 8));
        frame(draw, t, name, min, cell);
    } else {
        const int h = x * 7 + y * 13;
        std::snprintf(name, sizeof name, "floor_%d", h % 5 == 0 ? h % 4 : 0);
        frame(draw, t, name, min, cell);
        if (c == 2) {
            std::snprintf(name, sizeof name, "exit_%d", anim_index(time, 110, 8));
            frame(draw, t, name, min, cell);
        }
    }

    // Shadows cast by walls to the north / west.
    if (!on_guillotine(c)) {
        const bool n = is_wall_kind(at(x, y - 1));
        const bool w = is_wall_kind(at(x - 1, y));
        if (n) frame(draw, t, "shadow_n", min, cell);
        if (w) frame(draw, t, "shadow_w", min, cell);
        if (!n && !w && is_wall_kind(at(x - 1, y - 1))) frame(draw, t, "shadow_nw", min, cell);
    }
    if (is_watery(c)) {
        static constexpr std::array<const char*, 4> edges = {"water_edge_n", "water_edge_e", "water_edge_s", "water_edge_w"};
        static constexpr std::array<std::pair<int, int>, 4> d = {{{0, -1}, {1, 0}, {0, 1}, {-1, 0}}};
        for (std::size_t i = 0; i < 4; ++i)
            if (!is_watery(at(x + d[i].first, y + d[i].second))) frame(draw, t, edges[i], min, cell);
    }

    // Objects.
    const vs::SpriteSheet& p = player(cell);
    if (c == 8 || c == 10 || c == 20 || c == 63) frame(draw, t, "box", min, cell);
    if (color >= 0 && kind != 1) {
        std::snprintf(name, sizeof name, "key_%s", kColors[static_cast<std::size_t>(color)]);
        frame(draw, t, name, min, cell);
    }
    if (pground) {
        std::snprintf(name, sizeof name, "idle_%s_0", kDirs[static_cast<std::size_t>(*soldier::player_facing(tile))]);
        frame(draw, p, name, min, cell);
    }
    if (c == 7 || c == 61) frame(draw, p, "die_giveup_7", min, cell);
    if (c == 47 || c == 62) frame(draw, p, "die_timeout_5", min, cell);
    if (c == 55) frame(draw, p, "die_drown_7", min, cell);
}

void TilePainter::draw_overlay(ImDrawList* draw, int c, ImVec2 min, float cell) const {
    if (!loaded_) return;
    if (c >= 48 && c <= 52) frame(draw, tiles(cell), "guillotine_top_0", min, cell);
    if (c == 53) frame(draw, player(cell), "die_guillotine_6", min, cell);
}

void TilePainter::draw_marks(ImDrawList* draw, int c, ImVec2 min, float cell) const {
    if (c != 46 || cell < 12.0f) return;
    // Fake wall: a dashed outline and a '?' so the editor can tell it from a dark wall.
    const ImU32 col = IM_COL32(255, 230, 120, 220);
    const float inset = std::max(1.0f, cell * 0.08f);
    const ImVec2 a{min.x + inset, min.y + inset}, b{min.x + cell - inset, min.y + cell - inset};
    const int dashes = 4;
    for (int i = 0; i < dashes; ++i) {
        const float t0 = static_cast<float>(i) / dashes, t1 = t0 + 0.5f / dashes;
        const float x0 = a.x + (b.x - a.x) * t0, x1 = a.x + (b.x - a.x) * t1;
        const float y0 = a.y + (b.y - a.y) * t0, y1 = a.y + (b.y - a.y) * t1;
        draw->AddLine({x0, a.y}, {x1, a.y}, col);
        draw->AddLine({x0, b.y}, {x1, b.y}, col);
        draw->AddLine({a.x, y0}, {a.x, y1}, col);
        draw->AddLine({b.x, y0}, {b.x, y1}, col);
    }
    if (cell >= 18.0f) {
        ImFont* font = ImGui::GetFont();
        const float size = std::min(cell * 0.6f, ImGui::GetFontSize() * 1.2f);
        const ImVec2 ts = font->CalcTextSizeA(size, FLT_MAX, 0.0f, "?");
        const ImVec2 pos{min.x + (cell - ts.x) * 0.5f, min.y + (cell - ts.y) * 0.5f};
        draw->AddText(font, size, {pos.x + 1, pos.y + 1}, IM_COL32(0, 0, 0, 200), "?");
        draw->AddText(font, size, pos, col, "?");
    }
}

void TilePainter::draw_icon(ImDrawList* draw, int code, ImVec2 min, float size, double time) const {
    const CellFn at = [code](int x, int y) { return x == 0 && y == 0 ? code : 0; };
    draw_cell(draw, at, 0, 0, min, size, time);
    draw_overlay(draw, code, min, size);
    draw_marks(draw, code, min, size);
}

}  // namespace soldier_editor
