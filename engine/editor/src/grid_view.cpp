#include <vs/editor/grid_view.hpp>

#include <algorithm>
#include <cmath>
#include <iterator>

namespace vs {

namespace {
constexpr int kZoomLevels[] = {1, 2, 3, 4, 5, 6, 8, 10, 12, 16, 20, 24, 32, 40, 48, 64};
}  // namespace

float next_zoom_level(float zoom, int dir) {
    if (dir > 0) {
        for (int z : kZoomLevels)
            if (static_cast<float>(z) > zoom + 0.01f) return static_cast<float>(z);
        return 64;
    }
    for (int i = static_cast<int>(std::size(kZoomLevels)) - 1; i >= 0; --i)
        if (static_cast<float>(kZoomLevels[i]) < zoom - 0.01f) return static_cast<float>(kZoomLevels[i]);
    return 1;
}

float zoom_level_at_most(float zoom) {
    float best = 1;
    for (int z : kZoomLevels)
        if (static_cast<float>(z) <= zoom + 0.001f) best = static_cast<float>(z);
    return best;
}

void GridView::zoom_by(int dir, ImVec2 local) {
    if (dir == 0) return;
    const float old = zoom;
    const float wanted = integer_zoom ? next_zoom_level(old, dir) : old * (dir > 0 ? 1.25f : 0.8f);
    zoom = std::clamp(wanted, min_zoom, max_zoom);
    const ImVec2 world{(local.x - pan.x) / old, (local.y - pan.y) / old};
    pan = {local.x - world.x * zoom, local.y - world.y * zoom};
    if (integer_zoom) pan = {std::floor(pan.x), std::floor(pan.y)};
}

GridView::Frame GridView::begin(const char* id, int cols, int rows, float cell_px, ImVec2 size) {
    Frame f;
    f.cols = cols;
    f.rows = rows;
    ImVec2 avail = ImGui::GetContentRegionAvail();
    if (size.x <= 0) size.x = std::max(avail.x, 32.0f);
    if (size.y <= 0) size.y = std::max(avail.y, 32.0f);
    last_size_ = size;
    f.canvas_min = ImGui::GetCursorScreenPos();
    f.canvas_max = {f.canvas_min.x + size.x, f.canvas_min.y + size.y};
    ImGui::InvisibleButton(id, size,
                           ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight |
                               ImGuiButtonFlags_MouseButtonMiddle);
    f.hovered = ImGui::IsItemHovered();
    const ImGuiIO& io = ImGui::GetIO();

    // Middle-drag pan.
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0.0f)) {
        pan.x += io.MouseDelta.x;
        pan.y += io.MouseDelta.y;
    }
    // Wheel zoom around the pointer.
    if (f.hovered && io.MouseWheel != 0.0f && integer_zoom) {
        zoom_by(io.MouseWheel > 0 ? 1 : -1, {io.MousePos.x - f.canvas_min.x, io.MousePos.y - f.canvas_min.y});
    } else if (f.hovered && io.MouseWheel != 0.0f) {
        const float old_cell = cell_px * zoom;
        const float factor = io.MouseWheel > 0 ? 1.25f : 0.8f;
        zoom = std::clamp(zoom * factor, min_zoom, max_zoom);
        const float new_cell = cell_px * zoom;
        const ImVec2 local{io.MousePos.x - f.canvas_min.x, io.MousePos.y - f.canvas_min.y};
        const ImVec2 world{(local.x - pan.x) / old_cell, (local.y - pan.y) / old_cell};
        pan = {local.x - world.x * new_cell, local.y - world.y * new_cell};
    }

    f.cell = cell_px * zoom;
    f.origin = {std::floor(f.canvas_min.x + pan.x), std::floor(f.canvas_min.y + pan.y)};
    if (f.hovered) {
        const float cx = std::floor((io.MousePos.x - f.origin.x) / f.cell);
        const float cy = std::floor((io.MousePos.y - f.origin.y) / f.cell);
        if (cx >= 0 && cy >= 0 && cx < cols && cy < rows) {
            f.hovered_cell = true;
            f.cell_x = static_cast<int>(cx);
            f.cell_y = static_cast<int>(cy);
        }
    }
    f.draw = ImGui::GetWindowDrawList();
    f.draw->AddRectFilled(f.canvas_min, f.canvas_max, background);
    f.draw->PushClipRect(f.canvas_min, f.canvas_max, true);
    return f;
}

void GridView::end(const Frame& f) {
    const ImVec2 grid_max{f.origin.x + f.cols * f.cell, f.origin.y + f.rows * f.cell};
    if (show_grid_lines && f.cell >= 6.0f) {
        for (int x = 1; x < f.cols; ++x)
            f.draw->AddLine({f.origin.x + x * f.cell, f.origin.y}, {f.origin.x + x * f.cell, grid_max.y}, grid_line);
        for (int y = 1; y < f.rows; ++y)
            f.draw->AddLine({f.origin.x, f.origin.y + y * f.cell}, {grid_max.x, f.origin.y + y * f.cell}, grid_line);
    }
    f.draw->AddRect(f.origin, grid_max, border);
    f.draw->PopClipRect();
}

bool GridView::fit(int cols, int rows, float cell_px) {
    if (last_size_.x <= 0 || cols <= 0 || rows <= 0) return false;
    const float zx = (last_size_.x - 32) / (cols * cell_px);
    const float zy = (last_size_.y - 32) / (rows * cell_px);
    zoom = std::clamp(std::min(zx, zy), min_zoom, max_zoom);
    if (integer_zoom)
        zoom = std::clamp(zoom_level_at_most(zoom), min_zoom, max_zoom);
    else if (zoom >= 1.0f)
        zoom = std::floor(zoom);  // integer zoom keeps pixel art sharp
    const float cell = cell_px * zoom;
    pan = {std::floor((last_size_.x - cols * cell) * 0.5f), std::floor((last_size_.y - rows * cell) * 0.5f)};
    return true;
}

void GridView::ensure_visible(int x, int y, float cell_px) {
    const float cell = cell_px * zoom;
    const float left = pan.x + x * cell;
    const float top = pan.y + y * cell;
    if (left < 0) pan.x -= left;
    if (top < 0) pan.y -= top;
    if (left + cell > last_size_.x) pan.x -= left + cell - last_size_.x;
    if (top + cell > last_size_.y) pan.y -= top + cell - last_size_.y;
}

}  // namespace vs
