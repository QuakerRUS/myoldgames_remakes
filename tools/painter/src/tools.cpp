#include "tools.hpp"

#include <cmath>

namespace painter {

const char* tool_name(Tool t) {
    switch (t) {
    case Tool::Pencil: return "Pencil";
    case Tool::Eraser: return "Eraser";
    case Tool::Fill: return "Fill";
    case Tool::Line: return "Line";
    case Tool::Rect: return "Rectangle";
    case Tool::FilledRect: return "Filled rectangle";
    case Tool::Picker: return "Colour picker";
    case Tool::Select: return "Select";
    case Tool::FrameRect: return "Frame rectangle";
    case Tool::Pivot: return "Pivot";
    case Tool::Count: break;
    }
    return "";
}

const char* tool_shortcut(Tool t) {
    switch (t) {
    case Tool::Pencil: return "B";
    case Tool::Eraser: return "E";
    case Tool::Fill: return "G";
    case Tool::Line: return "L";
    case Tool::Rect: return "R";
    case Tool::FilledRect: return "Shift+R";
    case Tool::Picker: return "I";
    case Tool::Select: return "M";
    case Tool::FrameRect: return "T";
    case Tool::Pivot: return "Y";
    case Tool::Count: break;
    }
    return "";
}

const char* tool_hint(Tool t) {
    switch (t) {
    case Tool::Pencil: return "LMB: foreground colour, RMB: background colour";
    case Tool::Eraser: return "Paints transparent pixels";
    case Tool::Fill: return "4-way flood fill of one exact colour";
    case Tool::Line: return "Drag from one end to the other";
    case Tool::Rect: return "Drag from corner to corner";
    case Tool::FilledRect: return "Drag from corner to corner";
    case Tool::Picker: return "LMB: foreground, RMB: background (Alt+click with any tool)";
    case Tool::Select: return "Drag: select; drag inside: move; Ctrl+C/X/V, Del, Shift+H/V flip";
    case Tool::FrameRect: return "Click: select a frame; drag: set its rectangle (snaps to the tile grid)";
    case Tool::Pivot: return "Click: set the selected frame's pivot";
    case Tool::Count: break;
    }
    return "";
}

bool tool_needs_sheet(Tool t) { return t == Tool::FrameRect || t == Tool::Pivot; }

void draw_tool_icon(ImDrawList* d, Tool t, ImVec2 min, ImVec2 max, ImU32 col) {
    const float s = std::fmin(max.x - min.x, max.y - min.y) * 0.5f;
    const ImVec2 c{(min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f};
    auto P = [&](float x, float y) { return ImVec2{std::floor(c.x + x * s) + 0.5f, std::floor(c.y + y * s) + 0.5f}; };
    const float w = std::fmax(1.5f, s * 0.11f);
    switch (t) {
    case Tool::Pencil: {
        const ImVec2 body[4] = {P(-0.42f, 0.30f), P(0.30f, -0.42f), P(0.46f, -0.26f), P(-0.26f, 0.46f)};
        d->AddPolyline(body, 4, col, ImDrawFlags_Closed, w);
        d->AddTriangleFilled(P(-0.42f, 0.30f), P(-0.26f, 0.46f), P(-0.55f, 0.55f), col);
        d->AddLine(P(0.18f, -0.30f), P(0.34f, -0.14f), col, w);
        break;
    }
    case Tool::Eraser: {
        const ImVec2 body[4] = {P(-0.55f, 0.15f), P(0.05f, -0.45f), P(0.50f, 0.00f), P(-0.10f, 0.60f)};
        d->AddPolyline(body, 4, col, ImDrawFlags_Closed, w);
        const ImVec2 tip[4] = {P(-0.55f, 0.15f), P(-0.25f, -0.15f), P(0.20f, 0.30f), P(-0.10f, 0.60f)};
        d->AddConvexPolyFilled(tip, 4, col);
        d->AddLine(P(-0.10f, 0.62f), P(0.55f, 0.62f), col, w);
        break;
    }
    case Tool::Fill: {
        const ImVec2 bucket[4] = {P(-0.55f, -0.05f), P(-0.05f, -0.55f), P(0.40f, -0.10f), P(-0.10f, 0.40f)};
        d->AddPolyline(bucket, 4, col, ImDrawFlags_Closed, w);
        const ImVec2 paint[3] = {P(-0.55f, -0.05f), P(0.40f, -0.10f), P(-0.10f, 0.40f)};
        d->AddConvexPolyFilled(paint, 3, col);
        d->AddCircleFilled(P(0.48f, 0.42f), s * 0.14f, col);
        d->AddTriangleFilled(P(0.37f, 0.36f), P(0.59f, 0.36f), P(0.48f, 0.14f), col);
        break;
    }
    case Tool::Line:
        d->AddLine(P(-0.5f, 0.5f), P(0.5f, -0.5f), col, w);
        d->AddRectFilled(P(-0.62f, 0.38f), P(-0.38f, 0.62f), col);
        d->AddRectFilled(P(0.38f, -0.62f), P(0.62f, -0.38f), col);
        break;
    case Tool::Rect: d->AddRect(P(-0.55f, -0.45f), P(0.55f, 0.45f), col, 0, 0, w); break;
    case Tool::FilledRect:
        d->AddRect(P(-0.55f, -0.45f), P(0.55f, 0.45f), col, 0, 0, w);
        d->AddRectFilled(P(-0.38f, -0.28f), P(0.38f, 0.28f), col);
        break;
    case Tool::Picker:
        d->AddLine(P(-0.55f, 0.55f), P(0.15f, -0.15f), col, w);
        d->AddLine(P(-0.05f, -0.35f), P(0.35f, 0.05f), col, w * 1.4f);
        d->AddCircleFilled(P(0.30f, -0.30f), s * 0.24f, col);
        break;
    case Tool::Select: {
        const float a = -0.55f, b = 0.55f, step = 0.22f;
        for (float x = a; x < b - 0.01f; x += step * 2) {
            d->AddLine(P(x, a), P(std::fmin(x + step, b), a), col, w);
            d->AddLine(P(x, b), P(std::fmin(x + step, b), b), col, w);
        }
        for (float y = a; y < b - 0.01f; y += step * 2) {
            d->AddLine(P(a, y), P(a, std::fmin(y + step, b)), col, w);
            d->AddLine(P(b, y), P(b, std::fmin(y + step, b)), col, w);
        }
        break;
    }
    case Tool::FrameRect: {
        const float a = 0.6f, k = 0.28f;
        for (int sx = -1; sx <= 1; sx += 2)
            for (int sy = -1; sy <= 1; sy += 2) {
                const float x = a * static_cast<float>(sx), y = a * static_cast<float>(sy);
                d->AddLine(P(x, y), P(x - k * static_cast<float>(sx), y), col, w);
                d->AddLine(P(x, y), P(x, y - k * static_cast<float>(sy)), col, w);
            }
        d->AddRectFilled(P(-0.22f, -0.22f), P(0.22f, 0.22f), col);
        break;
    }
    case Tool::Pivot:
        d->AddCircle(P(0, 0), s * 0.38f, col, 20, w);
        d->AddLine(P(-0.65f, 0), P(0.65f, 0), col, w);
        d->AddLine(P(0, -0.65f), P(0, 0.65f), col, w);
        break;
    case Tool::Count: break;
    }
}

}  // namespace painter
