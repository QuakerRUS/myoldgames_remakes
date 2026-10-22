// Painter: the canvas (checkerboard, image, onion skin, grids, frames, selection, cursor) and
// its toolbar; integer zoom steps (vs::GridView::integer_zoom).
#include <algorithm>
#include <cmath>

#include <vs/imgui/imgui_layer.hpp>

#include "painter.hpp"
#include "ui_util.hpp"

namespace painter {

namespace {

// Rectangle outline with alternating dark/light dashes moving with time ("marching ants").
void marching_rect(ImDrawList* d, ImVec2 a, ImVec2 b, double time) {
    const float dash = 4;
    const float phase = static_cast<float>(std::fmod(time * 16.0, dash * 2));
    d->AddRect(a, b, IM_COL32(20, 20, 20, 255));
    auto edge = [&](ImVec2 p, ImVec2 q) {
        const float len = std::hypot(q.x - p.x, q.y - p.y);
        if (len <= 0) return;
        const ImVec2 dir{(q.x - p.x) / len, (q.y - p.y) / len};
        for (float t = -phase; t < len; t += dash * 2) {
            const float t0 = std::max(t, 0.0f), t1 = std::min(t + dash, len);
            if (t1 > t0) d->AddLine({p.x + dir.x * t0, p.y + dir.y * t0}, {p.x + dir.x * t1, p.y + dir.y * t1}, IM_COL32(255, 255, 255, 255));
        }
    };
    edge(a, {b.x - 1, a.y});
    edge({b.x - 1, a.y}, {b.x - 1, b.y - 1});
    edge({b.x - 1, b.y - 1}, {a.x, b.y - 1});
    edge({a.x, b.y - 1}, a);
}

}  // namespace

void Painter::zoom_step(int dir) { current_view().zoom_by(dir, {canvas_size_.x * 0.5f, canvas_size_.y * 0.5f}); }

void Painter::fit_view() { fit_pending_ = true; }

void Painter::draw_checkerboard(ImDrawList* draw, ImVec2 a, ImVec2 b, ImVec2 clip_min, ImVec2 clip_max) const {
    if (light_checker_)
        draw_checker(draw, a, b, clip_min, clip_max, 8, IM_COL32(204, 204, 204, 255), IM_COL32(240, 240, 240, 255));
    else
        draw_checker(draw, a, b, clip_min, clip_max, 8, IM_COL32(58, 61, 68, 255), IM_COL32(74, 78, 86, 255));
}

void Painter::draw_canvas_toolbar() {
    if (doc_.sheet) {
        const bool frame_ok = selected_frame() != nullptr;
        if (ImGui::RadioButton("Sheet", view_ == View::Sheet) && view_ != View::Sheet) {
            commit_floating();
            view_ = View::Sheet;
        }
        ImGui::SetItemTooltip("Whole sheet (F)");
        ImGui::SameLine();
        ImGui::BeginDisabled(!frame_ok);
        if (ImGui::RadioButton("Frame", view_ == View::Frame) && view_ != View::Frame) {
            commit_floating();
            selection_.reset();
            view_ = View::Frame;
            fit_pending_ = true;
        }
        ImGui::EndDisabled();
        ImGui::SetItemTooltip("Only the selected frame (F)");
        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();
    }
    if (ImGui::SmallButton("-")) zoom_step(-1);
    ImGui::SameLine();
    ImGui::Text("%dx", static_cast<int>(current_view().zoom));
    ImGui::SameLine();
    if (ImGui::SmallButton("+")) zoom_step(1);
    ImGui::SameLine();
    if (ImGui::SmallButton("Fit")) fit_view();
    ImGui::SetItemTooltip("Fit to the panel (Home)");
    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();
    ImGui::Checkbox("Pixels", &pixel_grid_);
    ImGui::SetItemTooltip("Pixel grid (from 6x zoom)");
    ImGui::SameLine();
    ImGui::Checkbox("Tiles", &tile_grid_);
    ImGui::SetItemTooltip("Tile grid %dx%d, gap %d", grid_.w, grid_.h, grid_.gap);
    ImGui::SameLine();
    if (ImGui::SmallButton("...")) ImGui::OpenPopup("tile_grid");
    ImGui::SetItemTooltip("Tile grid settings");
    if (ImGui::BeginPopup("tile_grid")) {
        ImGui::SetNextItemWidth(110);
        ImGui::DragInt("Tile width", &grid_.w, 0.2f, 1, 1024);
        ImGui::SetNextItemWidth(110);
        ImGui::DragInt("Tile height", &grid_.h, 0.2f, 1, 1024);
        ImGui::SetNextItemWidth(110);
        ImGui::DragInt("Gap", &grid_.gap, 0.1f, 0, 64);
        for (int s : {8, 16, 24, 32, 48, 64}) {
            ImGui::PushID(s);
            if (ImGui::SmallButton(std::to_string(s).c_str())) grid_.w = grid_.h = s;
            ImGui::PopID();
            ImGui::SameLine();
        }
        ImGui::NewLine();
        ImGui::Checkbox("Snap frame rectangles", &snap_to_grid_);
        ImGui::EndPopup();
    }
    if (doc_.sheet && view_ == View::Frame) {
        ImGui::SameLine();
        ImGui::Checkbox("Onion", &onion_skin_);
        ImGui::SetItemTooltip("Onion skin: the previous frame shows through (O)");
    }
}

void Painter::draw_canvas_panel() {
    draw_canvas_toolbar();
    const IRect region = view_rect();
    vs::GridView& v = current_view();
    if (fit_pending_ && canvas_size_.x > 0 && !region.empty()) {
        const ImVec2 size = canvas_size_;
        v.zoom = vs::zoom_level_at_most(std::min((size.x - 32) / static_cast<float>(region.w), (size.y - 32) / static_cast<float>(region.h)));
        v.pan = {std::floor((size.x - region.w * v.zoom) * 0.5f), std::floor((size.y - region.h * v.zoom) * 0.5f)};
        if (v.pan.x < 8) v.pan.x = 8;
        if (v.pan.y < 8) v.pan.y = 8;
        fit_pending_ = false;
    }
    v.show_grid_lines = pixel_grid_;
    const vs::GridView::Frame f = v.begin("##canvas", region.w, region.h, 1.0f);
    canvas_size_ = {f.canvas_max.x - f.canvas_min.x, f.canvas_max.y - f.canvas_min.y};
    ImDrawList* draw = f.draw;
    const ImVec2 a = f.origin;
    const ImVec2 b{a.x + region.w * f.cell, a.y + region.h * f.cell};
    draw_checkerboard(draw, a, b, f.canvas_min, f.canvas_max);

    const float iw = static_cast<float>(doc_.image.width), ih = static_cast<float>(doc_.image.height);
    if (onion_skin_ && view_ == View::Frame && doc_.sheet) {
        const int prev = previous_frame_index(*doc_.sheet, sel_frame_, sel_anim_);
        if (prev >= 0) {
            const IRect pr = intersect(frame_rect(doc_.sheet->data.frames[static_cast<std::size_t>(prev)]), bounds(doc_.image));
            if (!pr.empty() && image_tex_.valid())
                draw->AddImage(vs::ImGuiLayer::texture_id(image_tex_), a, {a.x + pr.w * f.cell, a.y + pr.h * f.cell},
                               {pr.x / iw, pr.y / ih}, {pr.right() / iw, pr.bottom() / ih}, IM_COL32(255, 255, 255, 80));
        }
    }
    if (image_tex_.valid())
        draw->AddImage(vs::ImGuiLayer::texture_id(image_tex_), a, b, {region.x / iw, region.y / ih},
                       {region.right() / iw, region.bottom() / ih});
    if (floating_ && floating_tex_.valid()) {
        const ImVec2 fa{a.x + (floating_->x - region.x) * f.cell, a.y + (floating_->y - region.y) * f.cell};
        draw->AddImage(vs::ImGuiLayer::texture_id(floating_tex_), fa,
                       {fa.x + floating_->pixels.width * f.cell, fa.y + floating_->pixels.height * f.cell});
    }
    v.end(f);

    draw->PushClipRect(f.canvas_min, f.canvas_max, true);
    draw_canvas_overlays(f, region);
    draw->PopClipRect();

    handle_canvas_mouse(f, region);
}

void Painter::draw_canvas_overlays(const vs::GridView::Frame& f, const IRect& region) {
    ImDrawList* d = f.draw;
    const float cell = f.cell;
    auto S = [&](int x, int y) { return ImVec2{f.origin.x + (x - region.x) * cell, f.origin.y + (y - region.y) * cell}; };
    auto visible = [&](ImVec2 p, ImVec2 q) { return q.x >= f.canvas_min.x && q.y >= f.canvas_min.y && p.x <= f.canvas_max.x && p.y <= f.canvas_max.y; };

    // Tile grid (in image coordinates, starting at 0,0).
    if (tile_grid_ && grid_.w > 0 && grid_.h > 0 && grid_.pitch_x() * cell >= 4 && grid_.pitch_y() * cell >= 4) {
        const ImU32 col = IM_COL32(80, 220, 255, 90);
        const int c0 = std::max(0, region.x / grid_.pitch_x());
        const int r0 = std::max(0, region.y / grid_.pitch_y());
        for (int r = r0; r * grid_.pitch_y() < region.bottom(); ++r)
            for (int c = c0; c * grid_.pitch_x() < region.right(); ++c) {
                const ImVec2 p = S(c * grid_.pitch_x(), r * grid_.pitch_y());
                const ImVec2 q = S(c * grid_.pitch_x() + grid_.w, r * grid_.pitch_y() + grid_.h);
                if (visible(p, q)) d->AddRect(p, q, col);
            }
    }

    // Frames of the sheet.
    if (doc_.sheet && view_ == View::Sheet && show_frames_) {
        const auto& frames = doc_.sheet->data.frames;
        for (std::size_t i = 0; i < frames.size(); ++i) {
            if (static_cast<int>(i) == sel_frame_) continue;
            const vs::FrameRect& fr = frames[i];
            const ImVec2 p = S(fr.x, fr.y), q = S(fr.x + fr.w, fr.y + fr.h);
            if (visible(p, q)) d->AddRect(p, q, IM_COL32(255, 255, 255, 46));
        }
        if (const vs::FrameRect* fr = selected_frame()) {
            const ImVec2 p = S(fr->x, fr->y), q = S(fr->x + fr->w, fr->y + fr->h);
            d->AddRect({p.x - 1, p.y - 1}, {q.x + 1, q.y + 1}, kFrameSelected, 0, 0, 2.0f);
            const ImVec2 ts = ImGui::CalcTextSize(fr->name.c_str());
            const ImVec2 lp{p.x - 1, p.y - ts.y - 4};
            d->AddRectFilled(lp, {lp.x + ts.x + 8, lp.y + ts.y + 3}, IM_COL32(255, 170, 40, 230), 3.0f);
            d->AddText({lp.x + 4, lp.y + 1}, IM_COL32(20, 20, 20, 255), fr->name.c_str());
        }
    }
    // Pivot of the selected frame.
    if (const vs::FrameRect* fr = (doc_.sheet ? selected_frame() : nullptr); fr && (fr->pivot_x || fr->pivot_y || tool_ == Tool::Pivot)) {
        const ImVec2 p = S(fr->x + fr->pivot_x, fr->y + fr->pivot_y);
        const ImU32 col = IM_COL32(255, 80, 200, 255);
        d->AddLine({p.x - 7, p.y}, {p.x + 8, p.y}, col, 1.5f);
        d->AddLine({p.x, p.y - 7}, {p.x, p.y + 8}, col, 1.5f);
        d->AddCircle(p, 4.0f, col, 12, 1.5f);
    }

    // Shape previews while dragging.
    if (drag_) {
        const vs::Color c = tool_color(drag_->right);
        const ImU32 col = c.a == 0 ? IM_COL32(255, 255, 255, 140) : to_imcol(c.with_alpha(std::max<std::uint8_t>(c.a, 160)));
        const int ax = drag_->ax, ay = drag_->ay, bx = drag_->lx, by = drag_->ly;
        if (drag_->tool == Tool::Line) {
            for (const auto& [x, y] : line_points(ax, ay, bx, by)) d->AddRectFilled(S(x, y), S(x + 1, y + 1), col);
        } else if (drag_->tool == Tool::Rect || drag_->tool == Tool::FilledRect) {
            const IRect r = span(ax, ay, bx, by);
            if (drag_->tool == Tool::FilledRect) {
                d->AddRectFilled(S(r.x, r.y), S(r.right(), r.bottom()), col);
            } else {
                d->AddRectFilled(S(r.x, r.y), S(r.right(), r.y + 1), col);
                d->AddRectFilled(S(r.x, r.bottom() - 1), S(r.right(), r.bottom()), col);
                d->AddRectFilled(S(r.x, r.y + 1), S(r.x + 1, r.bottom() - 1), col);
                d->AddRectFilled(S(r.right() - 1, r.y + 1), S(r.right(), r.bottom() - 1), col);
            }
        } else if (drag_->tool == Tool::FrameRect && drag_->moved) {
            IRect r = span(ax, ay, bx, by);
            if (snap_to_grid_) r = snap_to_tiles(r, grid_);
            d->AddRectFilled(S(r.x, r.y), S(r.right(), r.bottom()), IM_COL32(255, 170, 40, 40));
            d->AddRect(S(r.x, r.y), S(r.right(), r.bottom()), kFrameSelected, 0, 0, 2.0f);
        }
    }

    // Selection.
    if (selection_) marching_rect(d, S(selection_->x, selection_->y), S(selection_->right(), selection_->bottom()), ImGui::GetTime());

    // Hovered pixel and the keyboard frame cursor (the ancestor's "frame").
    if (hover_ && cell >= 4) d->AddRect(S(hover_->first, hover_->second), S(hover_->first + 1, hover_->second + 1), IM_COL32(255, 255, 255, 110));
    {
        const float grow = cell < 6 ? 2.0f : 0.0f;
        const ImVec2 p = S(cursor_x_, cursor_y_), q = S(cursor_x_ + 1, cursor_y_ + 1);
        const ImU32 col = focus_ == Focus::Canvas ? kCursor : IM_COL32(255, 204, 64, 90);
        d->AddRect({p.x - grow - 1, p.y - grow - 1}, {q.x + grow + 1, q.y + grow + 1}, IM_COL32(0, 0, 0, 160), 0, 0, 1.0f);
        d->AddRect({p.x - grow, p.y - grow}, {q.x + grow, q.y + grow}, col, 0, 0, 2.0f);
    }
}

}  // namespace painter
