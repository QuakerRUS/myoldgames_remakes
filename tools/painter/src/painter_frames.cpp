// Painter: the left panel — document info, the frame list and the selected frame's properties.
#include <algorithm>

#include <vs/engine/json_file.hpp>

#include <imgui.h>
#include <imgui_stdlib.h>

#include "painter.hpp"
#include "ui_util.hpp"

namespace painter {

namespace {

// The image grown downwards so that a w x h rectangle fits below the content; returns the slot.
IRect grow_for(vs::Image& image, int w, int h, int gap) {
    const int y = image.height + gap;
    image = resized_canvas(image, std::max(image.width, w), y + h);
    return {0, y, w, h};
}

}  // namespace

void Painter::select_frame(int index) {
    if (!doc_.sheet || index < 0 || index >= static_cast<int>(doc_.sheet->data.frames.size())) return;
    if (index == sel_frame_) return;
    commit_floating();
    sel_frame_ = index;
    if (view_ == View::Frame) {
        selection_.reset();
        fit_pending_ = true;
        const vs::FrameRect& f = doc_.sheet->data.frames[static_cast<std::size_t>(index)];
        cursor_x_ = f.x;
        cursor_y_ = f.y;
    }
}

void Painter::make_sheet() {
    if (doc_.sheet) return;
    Sheet s;
    s.data.image = doc_.path.empty() ? std::string("untitled.png") : vs::path_to_utf8(doc_.path.stem()) + ".png";
    push_sheet(std::move(s), "Make sprite sheet");
    status_ = "Now a sprite sheet: add frames with the Frame rectangle tool (T) or Add";
}

void Painter::add_frame() {
    if (!doc_.sheet) return;
    Sheet s = *doc_.sheet;
    const vs::FrameRect* cur = selected_frame();
    const int w = cur ? cur->w : grid_.w;
    const int h = cur ? cur->h : grid_.h;
    vs::FrameRect f;
    f.name = unique_frame_name(s, "frame");
    if (cur) {
        f.pivot_x = cur->pivot_x;
        f.pivot_y = cur->pivot_y;
    }
    std::optional<IRect> slot = find_free_slot(s, doc_.image.width, doc_.image.height, w, h, grid_);
    vs::Image image = doc_.image;
    const bool grow = !slot;
    if (grow) slot = grow_for(image, w, h, grid_.gap);
    f.x = slot->x;
    f.y = slot->y;
    f.w = slot->w;
    f.h = slot->h;
    s.data.frames.push_back(f);
    if (grow)
        push_image(std::move(image), std::move(s), "Add frame");
    else
        push_sheet(std::move(s), "Add frame");
    sel_frame_ = static_cast<int>(doc_.sheet->data.frames.size()) - 1;
    status_ = "Added \"" + f.name + "\"" + (grow ? " (the image grew)" : "");
}

void Painter::duplicate_frame() {
    const vs::FrameRect* cur = selected_frame();
    if (!cur) return;
    commit_floating();
    Sheet s = *doc_.sheet;
    vs::FrameRect f = *cur;
    f.name = unique_frame_name(s, cur->name);
    vs::Image image = doc_.image;
    std::optional<IRect> slot = find_free_slot(s, image.width, image.height, cur->w, cur->h, grid_);
    const bool grow = !slot;
    if (grow) slot = grow_for(image, cur->w, cur->h, grid_.gap);
    const vs::Image pixels = copy_region(image, frame_rect(*cur));
    blit(image, pixels, slot->x, slot->y, bounds(image));
    f.x = slot->x;
    f.y = slot->y;
    s.data.frames.insert(s.data.frames.begin() + sel_frame_ + 1, f);
    push_image(std::move(image), std::move(s), "Duplicate frame");
    ++sel_frame_;
    status_ = "Duplicated as \"" + f.name + "\"" + (grow ? " (the image grew)" : "");
}

void Painter::remove_frame() {
    if (!selected_frame()) return;
    Sheet s = *doc_.sheet;
    const std::string name = s.data.frames[static_cast<std::size_t>(sel_frame_)].name;
    const int removed = delete_frame(s, static_cast<std::size_t>(sel_frame_));
    push_sheet(std::move(s), "Delete frame");
    status_ = "Deleted \"" + name + "\" (pixels kept)";
    if (removed) status_ += ", " + std::to_string(removed) + " empty animation(s) removed";
}

void Painter::move_frame(int dir) {
    if (!selected_frame()) return;
    const int to = sel_frame_ + dir;
    if (to < 0 || to >= static_cast<int>(doc_.sheet->data.frames.size())) return;
    Sheet s = *doc_.sheet;
    std::swap(s.data.frames[static_cast<std::size_t>(sel_frame_)], s.data.frames[static_cast<std::size_t>(to)]);
    push_sheet(std::move(s), "Reorder frames");
    sel_frame_ = to;
    scrolled_frame_ = -2;
}

void Painter::draw_document_panel() {
    section_title("DOCUMENT");
    const std::string name = doc_.path.empty() ? std::string("Untitled") : vs::path_to_utf8(doc_.path.filename());
    ImGui::TextUnformatted(name.c_str());
    if (!doc_.path.empty()) ImGui::SetItemTooltip("%s", vs::path_to_utf8(doc_.path).c_str());
    ImGui::SameLine();
    ImGui::TextDisabled("%dx%d %s", doc_.image.width, doc_.image.height, doc_.sheet ? "sheet" : "PNG");
    if (!doc_.sheet) {
        ImGui::Spacing();
        ImGui::TextWrapped("A plain PNG image. Make it a sprite sheet to define frames and animations (saved as a JSON next to the PNG).");
        if (ImGui::Button("Make sprite sheet", {-1, 0})) make_sheet();
        return;
    }
    ImGui::Spacing();
    draw_frames_section();
    draw_frame_properties();
    ImGui::Spacing();
    draw_animations_section();

    const std::vector<std::string> problems = validate_sheet(*doc_.sheet, doc_.image.width, doc_.image.height);
    ImGui::Spacing();
    if (problems.empty()) {
        ImGui::TextColored(ImVec4(0.45f, 0.85f, 0.5f, 1.0f), "Sheet OK");
    } else {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.42f, 1.0f), "%d problem(s):", static_cast<int>(problems.size()));
        for (std::size_t i = 0; i < problems.size() && i < 5; ++i) ImGui::TextWrapped("- %s", problems[i].c_str());
    }
}

void Painter::draw_frames_section() {
    const auto& frames = doc_.sheet->data.frames;
    section_title(("FRAMES  " + std::to_string(frames.size())).c_str());
    const float list_h = std::max(120.0f, ImGui::GetContentRegionAvail().y * 0.26f);
    ImGui::BeginChild("frame_list", {0, list_h}, ImGuiChildFlags_FrameStyle);
    ImGuiListClipper clipper;
    clipper.Begin(static_cast<int>(frames.size()));
    if (scrolled_frame_ != sel_frame_ && sel_frame_ >= 0) clipper.IncludeItemByIndex(sel_frame_);
    while (clipper.Step())
        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i) {
            const vs::FrameRect& f = frames[static_cast<std::size_t>(i)];
            ImGui::PushID(i);
            if (ImGui::Selectable(f.name.c_str(), i == sel_frame_, ImGuiSelectableFlags_AllowDoubleClick)) {
                select_frame(i);
                scrolled_frame_ = i;
                if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                    view_ = View::Frame;
                    fit_pending_ = true;
                }
            }
            const std::string size = std::to_string(f.w) + "x" + std::to_string(f.h);
            ImGui::SameLine(ImGui::GetContentRegionMax().x - ImGui::CalcTextSize(size.c_str()).x);
            ImGui::TextDisabled("%s", size.c_str());
            if (i == sel_frame_ && scrolled_frame_ != sel_frame_) {
                ImGui::SetScrollHereY(0.5f);
                scrolled_frame_ = sel_frame_;
            }
            ImGui::PopID();
        }
    ImGui::EndChild();
    const bool has = selected_frame() != nullptr;
    if (ImGui::Button("Add")) add_frame();
    ImGui::SetItemTooltip("New frame in the first free tile (the image grows when full)");
    ImGui::SameLine();
    if (button_enabled("Duplicate", has)) duplicate_frame();
    ImGui::SetItemTooltip("Copy the frame and its pixels into a free tile");
    ImGui::SameLine();
    if (button_enabled("Delete", has)) remove_frame();
    ImGui::SetItemTooltip("Remove the frame (its pixels stay in the image)");
    ImGui::SameLine();
    ImGui::BeginDisabled(!has || sel_frame_ <= 0);
    if (ImGui::ArrowButton("frame_up", ImGuiDir_Up)) move_frame(-1);
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(!has || sel_frame_ + 1 >= static_cast<int>(frames.size()));
    if (ImGui::ArrowButton("frame_down", ImGuiDir_Down)) move_frame(1);
    ImGui::EndDisabled();
}

void Painter::draw_frame_properties() {
    vs::FrameRect* f = selected_frame();
    if (!f) return;
    const float label_w = 58;
    auto label = [&](const char* text) {
        ImGui::AlignTextToFramePadding();
        ImGui::TextDisabled("%s", text);
        ImGui::SameLine(label_w);
        ImGui::SetNextItemWidth(-1);
    };

    label("Name");
    if (frame_name_for_ != sel_frame_ || (!frame_name_editing_ && frame_name_buf_ != f->name)) {
        frame_name_buf_ = f->name;
        frame_name_for_ = sel_frame_;
    }
    ImGui::InputText("##frame_name", &frame_name_buf_);
    frame_name_editing_ = ImGui::IsItemActive();
    if (ImGui::IsItemDeactivatedAfterEdit() && frame_name_buf_ != f->name) {
        Sheet s = *doc_.sheet;
        if (rename_frame(s, static_cast<std::size_t>(sel_frame_), frame_name_buf_)) {
            push_sheet(std::move(s), "Rename frame");
        } else {
            status_ = frame_name_buf_.empty() ? "A frame name cannot be empty" : "\"" + frame_name_buf_ + "\" is already a frame name";
            frame_name_buf_ = f->name;
        }
        f = selected_frame();
    }

    int xy[2] = {f->x, f->y};
    label("Position");
    if (ImGui::DragInt2("##pos", xy, 0.25f, 0, 16384)) {
        f->x = xy[0];
        f->y = xy[1];
    }
    track_sheet_item("Move frame");
    if (!(f = selected_frame())) return;
    int wh[2] = {f->w, f->h};
    label("Size");
    if (ImGui::DragInt2("##size", wh, 0.25f, 1, 16384)) {
        f->w = std::max(1, wh[0]);
        f->h = std::max(1, wh[1]);
    }
    track_sheet_item("Resize frame");
    if (!(f = selected_frame())) return;
    int pv[2] = {f->pivot_x, f->pivot_y};
    label("Pivot");
    if (ImGui::DragInt2("##pivot", pv, 0.25f, -4096, 4096)) {
        f->pivot_x = pv[0];
        f->pivot_y = pv[1];
    }
    track_sheet_item("Set pivot");

    ImGui::SetCursorPosX(label_w);
    if (ImGui::SmallButton("Snap to tiles")) {
        Sheet s = *doc_.sheet;
        vs::FrameRect& g = s.data.frames[static_cast<std::size_t>(sel_frame_)];
        const IRect r = snap_to_tiles(frame_rect(g), grid_);
        g.x = r.x;
        g.y = r.y;
        g.w = r.w;
        g.h = r.h;
        push_sheet(std::move(s), "Snap frame to tiles");
    }
    ImGui::SetItemTooltip("Grow the rectangle to whole %dx%d tiles (gap %d)", grid_.w, grid_.h, grid_.gap);
    ImGui::SameLine();
    if (ImGui::SmallButton("Pivot: reset")) {
        Sheet s = *doc_.sheet;
        s.data.frames[static_cast<std::size_t>(sel_frame_)].pivot_x = 0;
        s.data.frames[static_cast<std::size_t>(sel_frame_)].pivot_y = 0;
        push_sheet(std::move(s), "Reset pivot");
    }
}

}  // namespace painter
