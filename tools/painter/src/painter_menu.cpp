// Painter: menu bar, tool strip, status bar and the controls reference.
#include <algorithm>

#include "painter.hpp"
#include "ui_util.hpp"

namespace painter {

void Painter::draw_menu() {
    if (!ImGui::BeginMenuBar()) return;
    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("New...", "Ctrl+N")) ask_save_then(After::New);
        if (ImGui::MenuItem("Open...", "Ctrl+O")) ask_save_then(After::Open);
        if (ImGui::MenuItem("Save", "Ctrl+S")) save();
        if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S")) save_as();
        ImGui::Separator();
        if (ImGui::MenuItem("Load palette...")) load_palette_dialog();
        if (ImGui::MenuItem("Save palette...")) save_palette_dialog();
        ImGui::Separator();
        if (ImGui::MenuItem("Exit", "Esc") && request_quit()) quit_now_ = true;
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Edit")) {
        const std::string u = undo_.can_undo() ? "Undo " + undo_.undo_label() : std::string("Undo");
        const std::string r = undo_.can_redo() ? "Redo " + undo_.redo_label() : std::string("Redo");
        if (ImGui::MenuItem(u.c_str(), "Ctrl+Z", false, undo_.can_undo())) undo();
        if (ImGui::MenuItem(r.c_str(), "Ctrl+Y", false, undo_.can_redo())) redo();
        ImGui::Separator();
        if (ImGui::MenuItem("Cut", "Ctrl+X", false, selection_.has_value())) cut();
        if (ImGui::MenuItem("Copy", "Ctrl+C")) copy();
        if (ImGui::MenuItem("Paste", "Ctrl+V", false, !clipboard_.empty())) paste();
        if (ImGui::MenuItem("Delete", "Del", false, selection_.has_value())) delete_selection();
        ImGui::Separator();
        if (ImGui::MenuItem("Select all", "Ctrl+A")) select_all();
        if (ImGui::MenuItem("Deselect", "Ctrl+D", false, selection_.has_value())) deselect();
        ImGui::Separator();
        const char* what = selection_ ? "selection" : view_ == View::Frame ? "frame" : "image";
        if (ImGui::MenuItem((std::string("Flip ") + what + " horizontally").c_str(), "Shift+H")) flip(true);
        if (ImGui::MenuItem((std::string("Flip ") + what + " vertically").c_str(), "Shift+V")) flip(false);
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Image")) {
        if (ImGui::MenuItem("Canvas size...")) {
            resize_w_ = doc_.image.width;
            resize_h_ = doc_.image.height;
            open_resize_popup_ = true;
        }
        if (ImGui::MenuItem("Make sprite sheet", nullptr, false, !doc_.sheet)) make_sheet();
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("View")) {
        if (ImGui::MenuItem("Zoom in", "+")) zoom_step(1);
        if (ImGui::MenuItem("Zoom out", "-")) zoom_step(-1);
        if (ImGui::MenuItem("Fit", "Home")) fit_view();
        ImGui::Separator();
        if (ImGui::MenuItem("Frame only", "F", view_ == View::Frame, doc_.sheet && selected_frame())) {
            view_ = view_ == View::Frame ? View::Sheet : View::Frame;
            fit_pending_ = view_ == View::Frame;
            selection_.reset();
        }
        ImGui::MenuItem("Pixel grid", nullptr, &pixel_grid_);
        ImGui::MenuItem("Tile grid", nullptr, &tile_grid_);
        ImGui::MenuItem("Frame outlines", nullptr, &show_frames_, doc_.sheet.has_value());
        ImGui::MenuItem("Onion skin", "O", &onion_skin_, doc_.sheet.has_value());
        ImGui::MenuItem("Light checkerboard", nullptr, &light_checker_);
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Help")) {
        if (ImGui::MenuItem("Controls", "")) open_help_popup_ = true;
        ImGui::EndMenu();
    }
    ImGui::EndMenuBar();
}

void Painter::draw_toolstrip() {
    const float size = ImGui::GetContentRegionAvail().x;
    ImDrawList* d = ImGui::GetWindowDrawList();
    for (int i = 0; i < static_cast<int>(Tool::Count); ++i) {
        const Tool t = static_cast<Tool>(i);
        if (t == Tool::Select || t == Tool::FrameRect) ImGui::Dummy({0, 4});
        const bool enabled = doc_.sheet || !tool_needs_sheet(t);
        ImGui::PushID(i);
        const ImVec2 p = ImGui::GetCursorScreenPos();
        ImGui::BeginDisabled(!enabled);
        if (ImGui::InvisibleButton("tool", {size, size}) && enabled) {
            if (t != Tool::Select) commit_floating();
            tool_ = t;
        }
        ImGui::EndDisabled();
        const bool hovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled);
        const bool active = tool_ == t;
        const ImVec2 q{p.x + size, p.y + size};
        if (active)
            d->AddRectFilled(p, q, IM_COL32(44, 110, 190, 255), 5.0f);
        else if (hovered && enabled)
            d->AddRectFilled(p, q, IM_COL32(64, 70, 82, 255), 5.0f);
        const ImU32 col = !enabled ? IM_COL32(100, 104, 112, 255) : active ? IM_COL32(255, 255, 255, 255) : IM_COL32(205, 210, 218, 255);
        draw_tool_icon(d, t, {p.x + 8, p.y + 8}, {q.x - 8, q.y - 8}, col);
        if (hovered && ImGui::BeginTooltip()) {
            ImGui::Text("%s  (%s)", tool_name(t), tool_shortcut(t));
            ImGui::TextDisabled("%s", enabled ? tool_hint(t) : "Sprite sheets only");
            ImGui::EndTooltip();
        }
        ImGui::PopID();
    }

    // Foreground / background swatches (classic overlapped squares); click swaps.
    ImGui::Dummy({0, 8});
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const float s = size * 0.62f;
    const ImVec2 bg_a{p.x + size - s, p.y + size * 0.36f};
    const ImVec2 fg_a{p.x, p.y};
    draw_checker(d, bg_a, {bg_a.x + s, bg_a.y + s}, bg_a, {bg_a.x + s, bg_a.y + s}, 4, IM_COL32(90, 90, 90, 255), IM_COL32(150, 150, 150, 255));
    d->AddRectFilled(bg_a, {bg_a.x + s, bg_a.y + s}, to_imcol(bg_));
    d->AddRect(bg_a, {bg_a.x + s, bg_a.y + s}, IM_COL32(200, 200, 200, 255));
    draw_checker(d, fg_a, {fg_a.x + s, fg_a.y + s}, fg_a, {fg_a.x + s, fg_a.y + s}, 4, IM_COL32(90, 90, 90, 255), IM_COL32(150, 150, 150, 255));
    d->AddRectFilled(fg_a, {fg_a.x + s, fg_a.y + s}, to_imcol(fg_));
    d->AddRect(fg_a, {fg_a.x + s, fg_a.y + s}, IM_COL32(255, 255, 255, 255));
    if (ImGui::InvisibleButton("swap", {size, size * 0.36f + s})) std::swap(fg_, bg_);
    ImGui::SetItemTooltip("Foreground #%s / background #%s\nClick or X: swap", to_hex(fg_, true).c_str(), to_hex(bg_, true).c_str());
}

void Painter::draw_status_bar() {
    const char* focus = focus_ == Focus::Canvas ? "Canvas" : "Palette";
    ImGui::AlignTextToFramePadding();
    ImGui::TextColored(ImVec4(0.40f, 0.68f, 1.0f, 1.0f), "%s", tool_name(tool_));
    ImGui::SameLine(0, 18);
    ImGui::TextDisabled("Focus");
    ImGui::SameLine();
    ImGui::TextUnformatted(focus);
    ImGui::SameLine(0, 18);
    ImGui::TextDisabled("Cursor");
    ImGui::SameLine();
    ImGui::Text("%d, %d", cursor_x_, cursor_y_);
    if (hover_) {
        const vs::Color c = doc_.image.get(hover_->first, hover_->second);
        ImGui::SameLine(0, 18);
        ImGui::TextDisabled("Pointer");
        ImGui::SameLine();
        ImGui::Text("%d, %d  #%s", hover_->first, hover_->second, to_hex(c, true).c_str());
    }
    ImGui::SameLine(0, 18);
    ImGui::TextDisabled("Image");
    ImGui::SameLine();
    ImGui::Text("%dx%d", doc_.image.width, doc_.image.height);
    if (selection_) {
        ImGui::SameLine(0, 18);
        ImGui::TextDisabled("Selection");
        ImGui::SameLine();
        ImGui::Text("%dx%d at %d, %d%s", selection_->w, selection_->h, selection_->x, selection_->y, floating_ ? " (floating)" : "");
    }
    ImGui::SameLine(0, 18);
    ImGui::TextDisabled("Zoom");
    ImGui::SameLine();
    ImGui::Text("%dx", static_cast<int>(current_view().zoom));
    if (!status_.empty()) {
        const float w = ImGui::CalcTextSize(status_.c_str()).x;
        const float x = ImGui::GetWindowContentRegionMax().x - w;
        ImGui::SameLine(std::max(x, ImGui::GetCursorPosX() + 18));
        ImGui::TextDisabled("%s", status_.c_str());
    }
}

void Painter::draw_help_popup() {
    if (open_help_popup_) {
        ImGui::OpenPopup("Controls");
        open_help_popup_ = false;
    }
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, {0.5f, 0.5f});
    if (!ImGui::BeginPopupModal("Controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) return;
    static const char* rows[][2] = {
        {"Mouse", ""},
        {"LMB / RMB", "Use the tool with the foreground / background colour"},
        {"Alt+click", "Pick a colour (LMB foreground, RMB background)"},
        {"Wheel", "Zoom (1x-64x) around the pointer"},
        {"Middle drag, Space+drag", "Pan"},
        {"Keyboard mode (PAINTER2)", ""},
        {"Arrows (Shift: 8 px)", "Move the frame cursor / the palette selection"},
        {"Space, Enter", "Paint the foreground colour (palette: take the colour)"},
        {"Backspace", "Paint the background colour (palette: set it as background)"},
        {"Delete", "Erase the pixel (or the selection) to transparent"},
        {"F1", "Flood fill from the cursor"},
        {"Tab", "Switch between the canvas and the palette"},
        {"Esc", "Cancel paste / deselect / ask to save and quit"},
        {"Tools", ""},
        {"B E G L R Shift+R I M", "Pencil, eraser, fill, line, rectangle, filled rectangle, picker, select"},
        {"T, Y", "Frame rectangle, pivot (sprite sheets)"},
        {"X", "Swap foreground and background colours"},
        {"Shift+H, Shift+V", "Flip selection / frame / image"},
        {"Ctrl+C X V, Del, Ctrl+A, Ctrl+D", "Copy, cut, paste, delete, select all, deselect"},
        {"Ctrl+Z, Ctrl+Y", "Undo, redo"},
        {"Ctrl+N O S, Ctrl+Shift+S", "New, open, save, save as"},
        {"Sheets", ""},
        {"F", "Show the selected frame alone / the whole sheet"},
        {"[ ]", "Previous / next frame"},
        {"O", "Onion skin"},
        {"P", "Play / pause the animation preview"},
        {"+ - Home", "Zoom in, zoom out, fit"},
    };
    if (ImGui::BeginTable("keys", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingFixedFit)) {
        for (const auto& row : rows) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            if (row[1][0] == '\0') {
                ImGui::TextColored(ImVec4(0.40f, 0.68f, 1.0f, 1.0f), "%s", row[0]);
                continue;
            }
            ImGui::TextUnformatted(row[0]);
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(row[1]);
        }
        ImGui::EndTable();
    }
    ImGui::Spacing();
    if (ImGui::Button("Close", {120, 0}) || ImGui::IsKeyPressed(ImGuiKey_Escape) || ImGui::IsKeyPressed(ImGuiKey_Enter))
        ImGui::CloseCurrentPopup();
    ImGui::EndPopup();
}

}  // namespace painter
