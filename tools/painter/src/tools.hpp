#pragma once
// Tool identifiers, names, shortcuts and the vector icons of the tool strip.
#include <imgui.h>

namespace painter {

enum class Tool { Pencil, Eraser, Fill, Line, Rect, FilledRect, Picker, Select, FrameRect, Pivot, Count };

const char* tool_name(Tool t);
const char* tool_shortcut(Tool t);
const char* tool_hint(Tool t);   // one line for the tooltip / status bar
bool tool_needs_sheet(Tool t);   // FrameRect, Pivot

// Draws the icon of `t` centred in [min, max].
void draw_tool_icon(ImDrawList* draw, Tool t, ImVec2 min, ImVec2 max, ImU32 color);

}  // namespace painter
