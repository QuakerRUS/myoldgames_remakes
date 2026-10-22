// Painter: keyboard. Shortcuts, tool keys, and the PAINTER2-style keyboard mode: a frame
// cursor moved by the arrows, Space/Enter paint, Backspace paints the background colour,
// F1 fills, Tab switches between the canvas and the palette, Esc asks to save and quits.
#include <algorithm>

#include "painter.hpp"

namespace painter {

void Painter::handle_keys() {
    const ImGuiIO& io = ImGui::GetIO();
    if (io.WantTextInput || prompt_.is_open() || message_.is_open() || dialogs_.busy() || ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId))
        return;
    auto pressed = [](ImGuiKey k, bool repeat = false) { return ImGui::IsKeyPressed(k, repeat); };
    const bool ctrl = io.KeyCtrl;
    const bool shift = io.KeyShift;
    // No shortcuts while a mouse action is in progress: cut/paste/select/new/open in the middle
    // of a stroke would interleave with the stroke's pending undo step.
    if (drag_) return;

    if (ctrl) {
        if (pressed(ImGuiKey_Z, true) && !shift) undo();
        if (pressed(ImGuiKey_Y, true) || (pressed(ImGuiKey_Z, true) && shift)) redo();
        if (pressed(ImGuiKey_N)) ask_save_then(After::New);
        if (pressed(ImGuiKey_O)) ask_save_then(After::Open);
        if (pressed(ImGuiKey_S)) shift ? save_as() : save();
        if (pressed(ImGuiKey_C)) copy();
        if (pressed(ImGuiKey_X)) cut();
        if (pressed(ImGuiKey_V)) paste();
        if (pressed(ImGuiKey_A)) select_all();
        if (pressed(ImGuiKey_D)) deselect();
        return;
    }
    if (ImGui::IsAnyItemActive()) return;

    if (pressed(ImGuiKey_Escape)) {
        if (floating_)
            cancel_floating();
        else if (selection_)
            selection_.reset();
        else if (request_quit())
            quit_now_ = true;
        return;
    }
    if (pressed(ImGuiKey_Tab)) {
        commit_floating();
        focus_ = focus_ == Focus::Canvas ? Focus::Palette : Focus::Canvas;
    }

    // Tools and view toggles.
    if (!io.KeyAlt) {
        struct Key { ImGuiKey key; bool shift; Tool tool; };
        static const Key keys[] = {{ImGuiKey_B, false, Tool::Pencil},    {ImGuiKey_E, false, Tool::Eraser},
                                   {ImGuiKey_G, false, Tool::Fill},      {ImGuiKey_L, false, Tool::Line},
                                   {ImGuiKey_R, false, Tool::Rect},      {ImGuiKey_R, true, Tool::FilledRect},
                                   {ImGuiKey_I, false, Tool::Picker},    {ImGuiKey_M, false, Tool::Select},
                                   {ImGuiKey_T, false, Tool::FrameRect}, {ImGuiKey_Y, false, Tool::Pivot}};
        for (const Key& k : keys)
            if (pressed(k.key) && shift == k.shift && (doc_.sheet || !tool_needs_sheet(k.tool))) {
                if (k.tool != Tool::Select) commit_floating();
                tool_ = k.tool;
            }
        if (pressed(ImGuiKey_H) && shift) flip(true);
        if (pressed(ImGuiKey_V) && shift) flip(false);
        if (pressed(ImGuiKey_X) && !shift) std::swap(fg_, bg_);
        if (pressed(ImGuiKey_O)) onion_skin_ = !onion_skin_;
        if (pressed(ImGuiKey_P)) preview_playing_ = !preview_playing_;
        if (pressed(ImGuiKey_F) && doc_.sheet) {
            commit_floating();
            selection_.reset();
            view_ = view_ == View::Frame || !selected_frame() ? View::Sheet : View::Frame;
            fit_pending_ = view_ == View::Frame;
        }
        if (pressed(ImGuiKey_Home)) fit_view();
        if (pressed(ImGuiKey_Equal, true) || pressed(ImGuiKey_KeypadAdd, true)) zoom_step(1);
        if (pressed(ImGuiKey_Minus, true) || pressed(ImGuiKey_KeypadSubtract, true)) zoom_step(-1);
        if (doc_.sheet && !doc_.sheet->data.frames.empty()) {
            const int n = static_cast<int>(doc_.sheet->data.frames.size());
            if (pressed(ImGuiKey_LeftBracket, true)) select_frame((sel_frame_ + n - 1) % n);
            if (pressed(ImGuiKey_RightBracket, true)) select_frame((sel_frame_ + 1) % n);
        }
    }

    int dx = 0, dy = 0;
    if (pressed(ImGuiKey_LeftArrow, true)) dx = -1;
    if (pressed(ImGuiKey_RightArrow, true)) dx = 1;
    if (pressed(ImGuiKey_UpArrow, true)) dy = -1;
    if (pressed(ImGuiKey_DownArrow, true)) dy = 1;
    const bool enter = pressed(ImGuiKey_Enter, true) || pressed(ImGuiKey_KeypadEnter, true);
    // Space paints on release, so that Space + drag can pan without painting.
    if (pressed(ImGuiKey_Space)) space_panned_ = false;
    const bool space = ImGui::IsKeyReleased(ImGuiKey_Space) && !space_panned_;

    if (focus_ == Focus::Palette) {
        if (!palette_.empty()) {
            const int n = static_cast<int>(palette_.size());
            palette_sel_ = std::clamp(palette_sel_ + dx + dy * std::max(palette_cols_, 1), 0, n - 1);
            const vs::Color c = palette_[static_cast<std::size_t>(palette_sel_)];
            if (enter || space) {
                fg_ = c;
                focus_ = Focus::Canvas;
            }
            if (pressed(ImGuiKey_Backspace)) {
                bg_ = c;
                focus_ = Focus::Canvas;
            }
        }
        return;
    }

    if (floating_) {
        const int step = shift ? 8 : 1;
        if (dx || dy) nudge_floating(dx * step, dy * step);
        if (enter) commit_floating();
        if (pressed(ImGuiKey_Delete)) delete_selection();
        return;
    }
    if (dx || dy) {
        const int step = shift ? 8 : 1;
        const IRect r = view_rect();
        cursor_x_ = std::clamp(cursor_x_ + dx * step, r.x, r.right() - 1);
        cursor_y_ = std::clamp(cursor_y_ + dy * step, r.y, r.bottom() - 1);
        vs::GridView& v = current_view();
        v.ensure_visible(cursor_x_ - r.x, cursor_y_ - r.y, 1.0f);
    }
    if (enter || space) paint_at_cursor(fg_, "Paint");
    if (pressed(ImGuiKey_Backspace, true)) paint_at_cursor(bg_, "Paint background");
    if (pressed(ImGuiKey_Delete)) {
        if (selection_)
            delete_selection();
        else
            paint_at_cursor({0, 0, 0, 0}, "Erase");
    }
    if (pressed(ImGuiKey_F1)) fill_at_cursor();
}

}  // namespace painter
