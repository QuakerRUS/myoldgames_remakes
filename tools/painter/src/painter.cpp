// Painter: construction, per-frame layout and texture synchronisation.
#include "painter.hpp"

#include <algorithm>
#include <new>

#include <imgui_internal.h>

#include <vs/engine/renderer.hpp>

#include "ui_util.hpp"

namespace painter {

Painter::Painter(vs::Renderer& renderer, SDL_Window* window)
    : renderer_(renderer), window_(window), flow_(prompt_, save_hooks()) {
    for (vs::GridView* v : {&sheet_view_, &frame_view_}) {
        v->background = IM_COL32(23, 25, 29, 255);
        v->grid_line = IM_COL32(255, 255, 255, 34);
        v->border = IM_COL32(255, 255, 255, 70);
        v->min_zoom = 1;
        v->max_zoom = 64;
        v->integer_zoom = true;  // the wheel and +/- step through 1, 2, 3, 4, 5, 6, 8 ... 64
    }
    new_document(64, 64, false);
}

void Painter::sync_textures() {
    if (tex_revision_ != doc_.revision || !image_tex_.valid()) {
        if (!doc_.image.empty()) {
            if (!image_tex_.valid() || image_tex_.width() != doc_.image.width || image_tex_.height() != doc_.image.height)
                image_tex_ = vs::Texture::from_image(renderer_, doc_.image);
            else
                image_tex_.update(doc_.image);
        }
        tex_revision_ = doc_.revision;
    }
    if (floating_dirty_) {
        floating_dirty_ = false;
        floating_tex_ = vs::Texture();
        if (floating_ && !floating_->pixels.empty()) floating_tex_ = vs::Texture::from_image(renderer_, floating_->pixels);
    }
}

void Painter::draw() {
    // Large images: an edit, an undo snapshot or a resize may not fit in memory. Cancel that
    // operation and tell the user instead of terminating (and losing the document).
    ImGuiErrorRecoveryState imgui_state;
    ImGui::ErrorRecoveryStoreState(&imgui_state);
    try {
        draw_frame();
    } catch (const std::bad_alloc&) {
        ImGuiIO& io = ImGui::GetIO();
        const bool assert_on = io.ConfigErrorRecoveryEnableAssert;
        io.ConfigErrorRecoveryEnableAssert = false;  // unbalanced Begin/End are expected here
        ImGui::ErrorRecoveryTryToRecoverState(&imgui_state);
        io.ConfigErrorRecoveryEnableAssert = assert_on;
        recover_from_out_of_memory();
    }
}

void Painter::recover_from_out_of_memory() {
    drag_.reset();
    cancel_edit();          // back to the pixels before the failed edit
    sheet_before_.reset();
    undo_.break_merge();
    floating_dirty_ = true;
    try {
        status_ = "Out of memory";
        message_.title = "Out of memory";
        message_.open("Not enough memory for this operation; it was cancelled.\n"
                      "Save your work, or make the image smaller.");
    } catch (const std::bad_alloc&) {
    }
}

void Painter::draw_frame() {
    dialogs_.poll();
    // Keep selections valid after undo/redo or structural edits.
    if (doc_.sheet) {
        const int nf = static_cast<int>(doc_.sheet->data.frames.size());
        const int na = static_cast<int>(doc_.sheet->data.animations.size());
        sel_frame_ = nf ? std::clamp(sel_frame_, 0, nf - 1) : -1;
        sel_anim_ = na ? std::clamp(sel_anim_, -1, na - 1) : -1;
    } else {
        sel_frame_ = sel_anim_ = -1;
        if (view_ == View::Frame) view_ = View::Sheet;
        if (tool_needs_sheet(tool_)) tool_ = Tool::Pencil;
    }
    const IRect region = view_rect();
    cursor_x_ = std::clamp(cursor_x_, region.x, std::max(region.x, region.right() - 1));
    cursor_y_ = std::clamp(cursor_y_, region.y, std::max(region.y, region.bottom() - 1));
    if (!palette_.empty()) palette_sel_ = std::clamp(palette_sel_, 0, static_cast<int>(palette_.size()) - 1);

    update_title();
    handle_keys();
    sync_textures();
    if (preview_playing_) preview_ms_ += static_cast<double>(ImGui::GetIO().DeltaTime) * 1000.0;

    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {6, 6});
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_MenuBar |
                                   ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoSavedSettings;
    ImGui::Begin("##painter", nullptr, flags);
    ImGui::PopStyleVar();
    draw_menu();

    const ImGuiStyle& style = ImGui::GetStyle();
    const ImVec2 avail = ImGui::GetContentRegionAvail();
    const float status_h = ImGui::GetFrameHeight() + style.ItemSpacing.y;
    const float h = avail.y - status_h;
    const float tools_w = 54;
    const float left_w = std::clamp(avail.x * 0.21f, 250.0f, 320.0f);
    const float right_w = std::clamp(avail.x * 0.21f, 250.0f, 310.0f);
    const float canvas_w = avail.x - tools_w - left_w - right_w - style.ItemSpacing.x * 3;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {5, 6});
    ImGui::BeginChild("tools", {tools_w, h}, ImGuiChildFlags_Borders, ImGuiWindowFlags_NoScrollbar);
    ImGui::PopStyleVar();
    draw_toolstrip();
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild("left", {left_w, h}, ImGuiChildFlags_Borders);
    draw_document_panel();
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild("canvas_panel", {canvas_w, h}, ImGuiChildFlags_Borders,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    draw_canvas_panel();
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild("right", {right_w, h}, ImGuiChildFlags_Borders, ImGuiWindowFlags_NoScrollbar);
    draw_color_section();
    const float preview_h = doc_.sheet ? 250.0f : 0.0f;
    draw_palette_section(ImGui::GetContentRegionAvail().y - preview_h);
    if (doc_.sheet) draw_preview();
    ImGui::EndChild();
    draw_status_bar();
    ImGui::End();

    draw_new_popup();
    draw_resize_popup();
    draw_help_popup();
    prompt_.draw();
    message_.draw();
}

void Painter::show_frame(const std::string& name) {
    if (!doc_.sheet) return;
    for (std::size_t i = 0; i < doc_.sheet->data.frames.size(); ++i)
        if (doc_.sheet->data.frames[i].name == name) {
            select_frame(static_cast<int>(i));
            view_ = View::Frame;
            fit_pending_ = true;
            return;
        }
}

}  // namespace painter
