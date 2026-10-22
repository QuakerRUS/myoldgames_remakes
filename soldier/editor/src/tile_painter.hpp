// Draws Soldier tiles into an ImGui draw list from the game's sprite sheets
// (data/gfx/tiles.json, player.json), with the game's autotiling: wall / dark-wall masks,
// wall shadows on floor-like cells, water edges, composed tiles (box on button, key in door,
// player under a guillotine, ...) and the animated exit / water.
#pragma once

#include <functional>

#include <imgui.h>

#include <vs/engine/sprite_sheet.hpp>

namespace vs {
class Renderer;
}

namespace soldier_editor {

class TilePainter {
public:
    // Code at (x, y); called for neighbours too, so it must handle out-of-range coordinates
    // (return -1 for "outside the level").
    using CellFn = std::function<int(int x, int y)>;
    static constexpr int kOutside = -1;

    void load(vs::Renderer& renderer);  // throws vs::Error
    bool loaded() const { return loaded_; }

    // Base layer of cell (x, y) at screen rect [min, min + cell): ground, shadows, water edges,
    // objects, the player. `time` (seconds) drives the animations.
    void draw_cell(ImDrawList* draw, const CellFn& at, int x, int y, ImVec2 min, float cell, double time) const;
    // Parts sticking out above the cell (guillotine posts and blade). Draw after all cells.
    void draw_overlay(ImDrawList* draw, int code, ImVec2 min, float cell) const;
    // Editor-only marks that the game does not show (the fake wall's '?').
    void draw_marks(ImDrawList* draw, int code, ImVec2 min, float cell) const;
    // One tile on its own (palette icon), surrounded by floor.
    void draw_icon(ImDrawList* draw, int code, ImVec2 min, float size, double time) const;

private:
    void frame(ImDrawList* draw, const vs::SpriteSheet& sheet, const char* name, ImVec2 min, float cell) const;
    const vs::SpriteSheet& tiles(float cell) const { return cell < 31.5f ? tiles_smooth_ : tiles_; }
    const vs::SpriteSheet& player(float cell) const { return cell < 31.5f ? player_smooth_ : player_; }

    vs::SpriteSheet tiles_, tiles_smooth_, player_, player_smooth_;
    bool loaded_ = false;
};

}  // namespace soldier_editor
