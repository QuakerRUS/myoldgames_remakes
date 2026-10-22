#pragma once
// Small ImGui helpers shared by the panels, and Painter's visual style.
#include <string>

#include <imgui.h>

#include <vs/engine/math.hpp>

namespace painter {

inline ImU32 to_imcol(vs::Color c) { return IM_COL32(c.r, c.g, c.b, c.a); }
inline ImVec4 to_vec4(vs::Color c) { return {c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f}; }
vs::Color from_vec4(const float v[4]);

// Accent colours of the UI.
constexpr ImU32 kAccent = IM_COL32(64, 156, 255, 255);
constexpr ImU32 kCursor = IM_COL32(255, 204, 64, 255);
constexpr ImU32 kFrameSelected = IM_COL32(255, 170, 40, 255);

// Section title: small caps text with a thin rule under it.
void section_title(const char* text);
// A button that is greyed out and inert when !enabled.
bool button_enabled(const char* label, bool enabled, ImVec2 size = {0, 0});
void help_marker(const char* text);
// Checkerboard of `square` px cells covering [a, b] (clipped to [clip_min, clip_max]).
void draw_checker(ImDrawList* draw, ImVec2 a, ImVec2 b, ImVec2 clip_min, ImVec2 clip_max, float square, ImU32 dark, ImU32 light);

void apply_painter_style();

}  // namespace painter
