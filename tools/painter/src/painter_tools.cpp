// Painter: mouse tools on the canvas and the keyboard painting actions.
#include <algorithm>
#include <cmath>

#include "painter.hpp"

namespace painter {

void Painter::handle_canvas_mouse(const vs::GridView::Frame& f, const IRect& region) {
    const ImGuiIO& io = ImGui::GetIO();
    const int px = region.x + static_cast<int>(std::floor((io.MousePos.x - f.origin.x) / f.cell));
    const int py = region.y + static_cast<int>(std::floor((io.MousePos.y - f.origin.y) / f.cell));
    hover_.reset();
    if (f.hovered && region.contains(px, py)) hover_ = std::pair{px, py};

    // Space + left drag pans (like the middle button).
    if (!drag_ && ImGui::IsKeyDown(ImGuiKey_Space) && !io.WantTextInput) {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && (f.hovered || space_panned_) && (io.MouseDelta.x != 0 || io.MouseDelta.y != 0)) {
            current_view().pan.x += io.MouseDelta.x;
            current_view().pan.y += io.MouseDelta.y;
            space_panned_ = true;
        }
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && f.hovered) space_panned_ = true;
        return;
    }

    const bool lclick = f.hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    const bool rclick = f.hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right);
    if (!drag_ && (lclick || rclick)) {
        focus_ = Focus::Canvas;
        if (region.contains(px, py)) {
            cursor_x_ = px;
            cursor_y_ = py;
        }
        const Tool t = io.KeyAlt ? Tool::Picker : tool_;
        begin_drag(t, rclick && !lclick, px, py);
    }
    if (drag_) {
        const bool down = ImGui::IsMouseDown(drag_->right ? ImGuiMouseButton_Right : ImGuiMouseButton_Left);
        if (px != drag_->lx || py != drag_->ly) continue_drag(px, py);
        if (!down) {
            finish_drag(px, py);
            drag_.reset();
        }
    }
}

void Painter::begin_drag(Tool t, bool right, int px, int py) {
    drag_ = Drag{};
    drag_->tool = t;
    drag_->right = right;
    drag_->ax = drag_->lx = px;
    drag_->ay = drag_->ly = py;
    if (t != Tool::Select && t != Tool::Picker && floating_) commit_floating();
    switch (t) {
    case Tool::Picker: pick_color(px, py, right); break;
    case Tool::Pencil:
    case Tool::Eraser:
        begin_edit();
        plot(doc_.image, px, py, t == Tool::Eraser ? vs::Color{0, 0, 0, 0} : tool_color(right), edit_clip());
        touch();
        break;
    case Tool::Fill:
        begin_edit();
        flood_fill(doc_.image, px, py, tool_color(right), edit_clip());
        touch();
        end_edit("Fill");
        use_color(tool_color(right));
        break;
    case Tool::Select:
        if (floating_ && IRect{floating_->x, floating_->y, floating_->pixels.width, floating_->pixels.height}.contains(px, py)) {
            drag_->moving = true;
        } else if (floating_) {
            commit_floating();
            selection_.reset();
        } else if (selection_ && selection_->contains(px, py) && !right) {
            lift_selection();
            drag_->moving = floating_.has_value();
        } else {
            selection_.reset();
        }
        if (drag_->moving) {
            drag_->grab_dx = px - floating_->x;
            drag_->grab_dy = py - floating_->y;
        }
        break;
    case Tool::Pivot:
        if (vs::FrameRect* fr = selected_frame()) {
            sheet_before_ = *doc_.sheet;
            fr->pivot_x = px - fr->x;
            fr->pivot_y = py - fr->y;
            ++doc_.sheet_revision;
        }
        break;
    default: break;
    }
}

void Painter::continue_drag(int px, int py) {
    Drag& d = *drag_;
    switch (d.tool) {
    case Tool::Picker: pick_color(px, py, d.right); break;
    case Tool::Pencil:
    case Tool::Eraser:
        draw_line(doc_.image, d.lx, d.ly, px, py, d.tool == Tool::Eraser ? vs::Color{0, 0, 0, 0} : tool_color(d.right), edit_clip());
        touch();
        break;
    case Tool::Select:
        if (d.moving && floating_) {
            floating_->x = px - d.grab_dx;
            floating_->y = py - d.grab_dy;
            selection_ = IRect{floating_->x, floating_->y, floating_->pixels.width, floating_->pixels.height};
        } else {
            const IRect r = intersect(span(d.ax, d.ay, px, py), view_rect());
            selection_ = r.empty() ? std::nullopt : std::optional<IRect>(r);
        }
        break;
    case Tool::Pivot:
        if (vs::FrameRect* fr = selected_frame(); fr && sheet_before_) {
            fr->pivot_x = px - fr->x;
            fr->pivot_y = py - fr->y;
            ++doc_.sheet_revision;
        }
        break;
    default: break;
    }
    d.lx = px;
    d.ly = py;
    d.moved = true;
}

void Painter::finish_drag(int px, int py) {
    const Drag d = *drag_;
    const vs::Color c = tool_color(d.right);
    switch (d.tool) {
    case Tool::Pencil:
        end_edit("Pencil");
        use_color(c);
        break;
    case Tool::Eraser: end_edit("Eraser"); break;
    case Tool::Line:
        begin_edit();
        draw_line(doc_.image, d.ax, d.ay, px, py, c, edit_clip());
        touch();
        end_edit("Line");
        use_color(c);
        break;
    case Tool::Rect:
    case Tool::FilledRect:
        begin_edit();
        draw_rect(doc_.image, span(d.ax, d.ay, px, py), c, d.tool == Tool::FilledRect, edit_clip());
        touch();
        end_edit(d.tool == Tool::FilledRect ? "Filled rectangle" : "Rectangle");
        use_color(c);
        break;
    case Tool::Select:
        if (!d.moving && !d.moved) selection_.reset();  // a plain click deselects
        break;
    case Tool::FrameRect: {
        if (!doc_.sheet) break;
        Sheet s = *doc_.sheet;
        if (!d.moved) {
            // Click: select the smallest frame under the pointer.
            int best = -1;
            long long best_area = 0;
            for (std::size_t i = 0; i < s.data.frames.size(); ++i) {
                const vs::FrameRect& f = s.data.frames[i];
                const long long area = static_cast<long long>(f.w) * f.h;
                if (frame_rect(f).contains(d.ax, d.ay) && (best < 0 || area < best_area)) {
                    best = static_cast<int>(i);
                    best_area = area;
                }
            }
            if (best >= 0) select_frame(best);
            break;
        }
        IRect r = span(d.ax, d.ay, px, py);
        if (snap_to_grid_) r = snap_to_tiles(r, grid_);
        r = intersect(r, bounds(doc_.image));
        if (r.empty()) break;
        if (sel_frame_ < 0 || sel_frame_ >= static_cast<int>(s.data.frames.size())) {
            vs::FrameRect nf;
            nf.name = unique_frame_name(s, "frame");
            s.data.frames.push_back(nf);
            sel_frame_ = static_cast<int>(s.data.frames.size()) - 1;
        }
        vs::FrameRect& f = s.data.frames[static_cast<std::size_t>(sel_frame_)];
        f.x = r.x;
        f.y = r.y;
        f.w = r.w;
        f.h = r.h;
        const std::string name = f.name;  // f refers into s, which push_sheet consumes
        push_sheet(std::move(s), "Set frame rectangle");
        status_ = "Frame \"" + name + "\" = " + std::to_string(r.w) + "x" + std::to_string(r.h) + " at " + std::to_string(r.x) +
                  "," + std::to_string(r.y);
        break;
    }
    case Tool::Pivot:
        if (sheet_before_ && doc_.sheet) {
            Sheet after = *doc_.sheet;
            doc_.sheet = std::move(*sheet_before_);
            push_sheet(std::move(after), "Set pivot");
        }
        sheet_before_.reset();
        break;
    default: break;
    }
}

void Painter::pick_color(int px, int py, bool right) {
    if (!doc_.image.in_bounds(px, py)) return;
    const vs::Color c = doc_.image.get(px, py);
    (right ? bg_ : fg_) = c;
    status_ = std::string(right ? "Background" : "Foreground") + " = #" + to_hex(c, true);
}

void Painter::paint_at_cursor(vs::Color c, const char* label) {
    if (!edit_clip().contains(cursor_x_, cursor_y_)) return;
    begin_edit();
    plot(doc_.image, cursor_x_, cursor_y_, c, edit_clip());
    touch();
    end_edit(label);
    use_color(c);
}

void Painter::fill_at_cursor() {
    begin_edit();
    flood_fill(doc_.image, cursor_x_, cursor_y_, fg_, edit_clip());
    touch();
    end_edit("Fill");
    use_color(fg_);
}

}  // namespace painter
