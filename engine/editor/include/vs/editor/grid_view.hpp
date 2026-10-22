#pragma once
#include <imgui.h>

namespace vs {

// Integer zoom levels for pixel art: 1, 2, 3, 4, 5, 6, 8, 10, 12, 16, 20, 24, 32, 40, 48, 64.
// The next level above `zoom` (dir > 0) or below it (dir < 0); stays within 1..64.
float next_zoom_level(float zoom, int dir);
// The largest level <= zoom (at least 1).
float zoom_level_at_most(float zoom);

// Zoomable, pannable canvas for a grid of cells inside the current ImGui window (map editors,
// Painter's pixel grid). Mouse: wheel zooms around the pointer, middle button drags to pan.
//
//   GridView::Frame f = view.begin("field", cols, rows, 32.0f);
//   ... f.draw->AddRectFilled(f.cell_min(x, y), f.cell_max(x, y), colour) ...
//   if (f.hovered_cell && ImGui::IsMouseDown(ImGuiMouseButton_Left)) place(f.cell_x, f.cell_y);
//   view.end(f);
class GridView {
public:
    float zoom = 1.0f;
    float min_zoom = 0.25f;
    float max_zoom = 16.0f;
    ImVec2 pan{16.0f, 16.0f};  // screen offset of cell (0,0) from the canvas' top-left corner
    bool show_grid_lines = true;
    // Pixel art: the wheel and zoom_by() step through the integer zoom levels (within
    // min_zoom..max_zoom), the pan stays on whole pixels and fit() picks the largest level that fits.
    bool integer_zoom = false;
    ImU32 background = IM_COL32(24, 26, 30, 255);
    ImU32 grid_line = IM_COL32(255, 255, 255, 28);
    ImU32 border = IM_COL32(255, 255, 255, 90);

    struct Frame {
        ImDrawList* draw = nullptr;
        ImVec2 canvas_min{};
        ImVec2 canvas_max{};
        ImVec2 origin{};       // screen position of cell (0,0)
        float cell = 1.0f;     // screen pixels per cell
        int cols = 0;
        int rows = 0;
        bool hovered = false;  // pointer over the canvas
        bool hovered_cell = false;
        int cell_x = -1;       // cell under the pointer (valid if hovered_cell)
        int cell_y = -1;
        ImVec2 cell_min(int x, int y) const { return {origin.x + x * cell, origin.y + y * cell}; }
        ImVec2 cell_max(int x, int y) const { return {origin.x + (x + 1) * cell, origin.y + (y + 1) * cell}; }
    };

    // Uses `size` (0,0 = the rest of the content region). Draws the background, then pushes a
    // clip rect; draw the cells, then call end() to draw grid lines and the border.
    Frame begin(const char* id, int cols, int rows, float cell_px, ImVec2 size = {0, 0});
    void end(const Frame& frame);

    // Zoom so that the whole grid fits the last canvas size, and centre it. Returns false
    // (and does nothing) before the first begin() has measured the canvas.
    bool fit(int cols, int rows, float cell_px);
    // Scrolls minimally so that cell (x, y) is visible in the last canvas.
    void ensure_visible(int x, int y, float cell_px);
    // Zooms one step in (dir > 0) or out around `local` (a point relative to the canvas'
    // top-left corner): the content under it stays put. Integer levels with integer_zoom, else x1.25.
    void zoom_by(int dir, ImVec2 local);

private:
    ImVec2 last_size_{0, 0};
};

}  // namespace vs
