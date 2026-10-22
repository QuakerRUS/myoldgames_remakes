// Painter: the animation list, the selected animation's frames and durations, and the live preview.
#include <algorithm>
#include <cmath>

#include <vs/imgui/imgui_layer.hpp>

#include <imgui.h>
#include <imgui_stdlib.h>

#include "painter.hpp"
#include "ui_util.hpp"

namespace painter {

namespace {

double entry_start_ms(const vs::AnimationDef& a, std::size_t index) {
    double t = 0;
    for (std::size_t i = 0; i < index && i < a.frames.size(); ++i) t += a.frames[i].ms;
    return t;
}

}  // namespace

void Painter::add_animation() {
    const vs::FrameRect* f = selected_frame();
    if (!f) {
        status_ = "Add a frame first: an animation needs at least one frame";
        return;
    }
    Sheet s = *doc_.sheet;
    vs::AnimationDef a;
    a.name = unique_animation_name(s, "anim");
    a.frames.push_back({f->name, 100});
    s.data.animations.push_back(a);
    push_sheet(std::move(s), "Add animation");
    sel_anim_ = static_cast<int>(doc_.sheet->data.animations.size()) - 1;
    preview_ms_ = 0;
}

void Painter::remove_animation() {
    if (!selected_animation()) return;
    Sheet s = *doc_.sheet;
    s.data.animations.erase(s.data.animations.begin() + sel_anim_);
    push_sheet(std::move(s), "Delete animation");
}

void Painter::draw_animations_section() {
    const auto& anims = doc_.sheet->data.animations;
    section_title(("ANIMATIONS  " + std::to_string(anims.size())).c_str());
    ImGui::BeginChild("anim_list", {0, 112}, ImGuiChildFlags_FrameStyle);
    for (int i = 0; i < static_cast<int>(anims.size()); ++i) {
        const vs::AnimationDef& a = anims[static_cast<std::size_t>(i)];
        ImGui::PushID(i);
        if (ImGui::Selectable(a.name.c_str(), i == sel_anim_)) {
            sel_anim_ = i;
            scrolled_anim_ = i;
            preview_ms_ = 0;
        }
        const std::string info = std::to_string(a.frames.size()) + " fr, " + std::to_string(a.total_ms()) + " ms" + (a.loop ? "" : ", once");
        ImGui::SameLine(ImGui::GetContentRegionMax().x - ImGui::CalcTextSize(info.c_str()).x);
        ImGui::TextDisabled("%s", info.c_str());
        if (i == sel_anim_ && scrolled_anim_ != sel_anim_) {
            ImGui::SetScrollHereY(0.5f);
            scrolled_anim_ = sel_anim_;
        }
        ImGui::PopID();
    }
    ImGui::EndChild();
    if (button_enabled("Add##anim", selected_frame() != nullptr)) add_animation();
    ImGui::SetItemTooltip("New animation starting with the selected frame");
    ImGui::SameLine();
    if (button_enabled("Delete##anim", selected_animation() != nullptr)) remove_animation();

    vs::AnimationDef* a = selected_animation();
    if (!a) return;
    ImGui::AlignTextToFramePadding();
    ImGui::TextDisabled("Name");
    ImGui::SameLine(58);
    ImGui::SetNextItemWidth(-72);
    if (anim_name_for_ != sel_anim_ || (!anim_name_editing_ && anim_name_buf_ != a->name)) {
        anim_name_buf_ = a->name;
        anim_name_for_ = sel_anim_;
    }
    ImGui::InputText("##anim_name", &anim_name_buf_);
    anim_name_editing_ = ImGui::IsItemActive();
    if (ImGui::IsItemDeactivatedAfterEdit() && anim_name_buf_ != a->name) {
        Sheet s = *doc_.sheet;
        if (rename_animation(s, static_cast<std::size_t>(sel_anim_), anim_name_buf_)) {
            push_sheet(std::move(s), "Rename animation");
        } else {
            status_ = anim_name_buf_.empty() ? "An animation name cannot be empty" : "\"" + anim_name_buf_ + "\" is already an animation";
            anim_name_buf_ = a->name;
        }
        if (!(a = selected_animation())) return;
    }
    ImGui::SameLine();
    bool loop = a->loop;
    if (ImGui::Checkbox("Loop", &loop)) {
        Sheet s = *doc_.sheet;
        s.data.animations[static_cast<std::size_t>(sel_anim_)].loop = loop;
        push_sheet(std::move(s), loop ? "Loop animation" : "Play animation once");
        if (!(a = selected_animation())) return;
    }
    draw_animation_entries(*a);
}

void Painter::draw_animation_entries(vs::AnimationDef& anim) {
    const auto& frames = doc_.sheet->data.frames;
    const std::size_t playing = animation_entry_at(anim, preview_ms_);
    const float h = std::max(96.0f, ImGui::GetContentRegionAvail().y - ImGui::GetFrameHeightWithSpacing() - ImGui::GetTextLineHeightWithSpacing() * 2 - 10);
    std::optional<Sheet> change;
    std::string change_label;
    const ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_SizingFixedFit;
    if (ImGui::BeginTable("entries", 4, flags, {0, h})) {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed, 22);
        ImGui::TableSetupColumn("Frame", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("ms", ImGuiTableColumnFlags_WidthFixed, 52);
        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 62);
        ImGui::TableHeadersRow();
        for (std::size_t k = 0; k < anim.frames.size(); ++k) {
            vs::AnimationFrame& e = anim.frames[k];
            ImGui::PushID(static_cast<int>(k));
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            const std::string num = std::to_string(k + 1);
            if (ImGui::Selectable(num.c_str(), k == playing && !preview_playing_, ImGuiSelectableFlags_None)) {
                preview_playing_ = false;
                preview_ms_ = entry_start_ms(anim, k) + 0.5;
                for (std::size_t i = 0; i < frames.size(); ++i)
                    if (frames[i].name == e.frame) select_frame(static_cast<int>(i));
            }
            if (k == playing) ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, IM_COL32(64, 156, 255, 40));
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(-1);
            if (ImGui::BeginCombo("##frame", e.frame.c_str(), ImGuiComboFlags_HeightLarge)) {
                for (const vs::FrameRect& f : frames)
                    if (ImGui::Selectable(f.name.c_str(), f.name == e.frame) && f.name != e.frame) {
                        change = *doc_.sheet;
                        change->data.animations[static_cast<std::size_t>(sel_anim_)].frames[k].frame = f.name;
                        change_label = "Change animation frame";
                    }
                ImGui::EndCombo();
            }
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(-1);
            if (ImGui::DragInt("##ms", &e.ms, 1.0f, 1, 60000)) e.ms = std::max(1, e.ms);
            track_sheet_item("Change duration");
            ImGui::TableNextColumn();
            const std::size_t n = doc_.sheet->data.animations[static_cast<std::size_t>(sel_anim_)].frames.size();
            ImGui::BeginDisabled(k == 0);
            if (ImGui::ArrowButton("up", ImGuiDir_Up) && !change) {
                change = *doc_.sheet;
                auto& v = change->data.animations[static_cast<std::size_t>(sel_anim_)].frames;
                std::swap(v[k], v[k - 1]);
                change_label = "Reorder animation";
            }
            ImGui::EndDisabled();
            ImGui::SameLine(0, 2);
            ImGui::BeginDisabled(n <= 1);
            if (ImGui::SmallButton("x") && !change) {
                change = *doc_.sheet;
                auto& v = change->data.animations[static_cast<std::size_t>(sel_anim_)].frames;
                v.erase(v.begin() + static_cast<std::ptrdiff_t>(k));
                change_label = "Remove animation frame";
            }
            ImGui::EndDisabled();
            ImGui::SetItemTooltip("Remove this entry");
            ImGui::PopID();
            if (doc_.sheet->data.animations.size() <= static_cast<std::size_t>(sel_anim_)) break;  // replaced by an undo step
            if (&doc_.sheet->data.animations[static_cast<std::size_t>(sel_anim_)] != &anim) break;
        }
        ImGui::EndTable();
    }
    if (button_enabled("+ Selected frame", selected_frame() != nullptr) && !change) {
        change = *doc_.sheet;
        change->data.animations[static_cast<std::size_t>(sel_anim_)].frames.push_back(
            {selected_frame()->name, anim.frames.empty() ? 100 : anim.frames.back().ms});
        change_label = "Add animation frame";
    }
    ImGui::SetItemTooltip("Append the selected frame (same duration as the last entry)");
    ImGui::SameLine();
    static int all_ms = 100;
    ImGui::SetNextItemWidth(60);
    ImGui::DragInt("##all_ms", &all_ms, 1.0f, 1, 60000);
    ImGui::SameLine();
    if (ImGui::Button("Set all ms") && !change) {
        change = *doc_.sheet;
        for (vs::AnimationFrame& e : change->data.animations[static_cast<std::size_t>(sel_anim_)].frames) e.ms = std::max(1, all_ms);
        change_label = "Set all durations";
    }
    if (change) push_sheet(std::move(*change), change_label);
}

void Painter::draw_preview() {
    section_title("PREVIEW");
    const vs::AnimationDef* anim = selected_animation();
    const auto& frames = doc_.sheet->data.frames;
    auto find = [&](const std::string& name) -> const vs::FrameRect* { return doc_.sheet->data.find_frame(name); };

    // Controls.
    if (anim) {
        if (ImGui::Button(preview_playing_ ? "Pause" : "Play", {60, 0})) preview_playing_ = !preview_playing_;
        ImGui::SetItemTooltip("P");
        ImGui::SameLine();
        const std::size_t cur = animation_entry_at(*anim, preview_ms_);
        const std::size_t n = anim->frames.size();
        if (ImGui::ArrowButton("prev", ImGuiDir_Left)) {
            preview_playing_ = false;
            preview_ms_ = entry_start_ms(*anim, (cur + n - 1) % n) + 0.5;
        }
        ImGui::SameLine();
        if (ImGui::ArrowButton("next", ImGuiDir_Right)) {
            preview_playing_ = false;
            preview_ms_ = entry_start_ms(*anim, (cur + 1) % n) + 0.5;
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Restart")) preview_ms_ = 0;
    } else {
        ImGui::TextDisabled("No animation selected: the frame");
    }
    ImGui::SameLine();
    const char* scales[] = {"Auto", "1x", "2x", "3x", "4x", "6x", "8x"};
    const int scale_values[] = {0, 1, 2, 3, 4, 6, 8};
    int scale_index = 0;
    for (int i = 0; i < 7; ++i)
        if (scale_values[i] == preview_scale_) scale_index = i;
    ImGui::SetNextItemWidth(-1);
    if (ImGui::Combo("##scale", &scale_index, scales, 7)) preview_scale_ = scale_values[scale_index];

    // What to show and the box that holds every frame aligned on its pivot.
    const vs::FrameRect* shown = nullptr;
    std::string caption;
    IRect box;
    if (anim && !anim->frames.empty()) {
        const std::size_t k = animation_entry_at(*anim, preview_ms_);
        shown = find(anim->frames[k].frame);
        caption = std::to_string(k + 1) + "/" + std::to_string(anim->frames.size()) + "  " + anim->frames[k].frame + "  " +
                  std::to_string(anim->frames[k].ms) + " ms";
        for (const vs::AnimationFrame& e : anim->frames)
            if (const vs::FrameRect* f = find(e.frame)) box = unite(box, {-f->pivot_x, -f->pivot_y, f->w, f->h});
    } else if (sel_frame_ >= 0 && sel_frame_ < static_cast<int>(frames.size())) {
        shown = &frames[static_cast<std::size_t>(sel_frame_)];
        caption = shown->name;
        box = {-shown->pivot_x, -shown->pivot_y, shown->w, shown->h};
    }

    const ImVec2 avail = ImGui::GetContentRegionAvail();
    const float area_h = std::max(60.0f, avail.y - ImGui::GetTextLineHeightWithSpacing() - 4);
    const ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::Dummy({avail.x, area_h});
    ImDrawList* d = ImGui::GetWindowDrawList();
    d->AddRectFilled(p, {p.x + avail.x, p.y + area_h}, IM_COL32(23, 25, 29, 255), 4.0f);
    if (shown && !box.empty() && image_tex_.valid()) {
        int scale = preview_scale_;
        if (scale <= 0) {
            scale = static_cast<int>(std::floor(std::min((avail.x - 16) / static_cast<float>(box.w), (area_h - 16) / static_cast<float>(box.h))));
            scale = std::clamp(scale, 1, 8);
        }
        const float bw = static_cast<float>(box.w * scale), bh = static_cast<float>(box.h * scale);
        const ImVec2 o{std::floor(p.x + (avail.x - bw) * 0.5f), std::floor(p.y + (area_h - bh) * 0.5f)};
        d->PushClipRect(p, {p.x + avail.x, p.y + area_h}, true);
        draw_checkerboard(d, o, {o.x + bw, o.y + bh}, p, {p.x + avail.x, p.y + area_h});
        const IRect src = intersect(frame_rect(*shown), bounds(doc_.image));
        const float iw = static_cast<float>(doc_.image.width), ih = static_cast<float>(doc_.image.height);
        const ImVec2 at{o.x + static_cast<float>((-shown->pivot_x - box.x) * scale), o.y + static_cast<float>((-shown->pivot_y - box.y) * scale)};
        if (!src.empty())
            d->AddImage(vs::ImGuiLayer::texture_id(image_tex_), at, {at.x + src.w * scale, at.y + src.h * scale}, {src.x / iw, src.y / ih},
                        {src.right() / iw, src.bottom() / ih});
        d->PopClipRect();
    }
    ImGui::TextDisabled("%s", caption.c_str());
}

}  // namespace painter
