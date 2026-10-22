// Painter: undoable edits, the shown region and selection helpers.
#include <algorithm>

#include "commands.hpp"
#include "painter.hpp"

namespace painter {

void Painter::begin_edit() {
    if (!edit_before_) edit_before_ = doc_.image;
}

void Painter::end_edit(const std::string& label) {
    if (!edit_before_) return;
    const IRect r = diff_bounds(*edit_before_, doc_.image);
    if (!r.empty())
        undo_.push(std::make_unique<RegionCommand>(doc_, r, copy_region(*edit_before_, r), copy_region(doc_.image, r), label));
    edit_before_.reset();
}

void Painter::cancel_edit() {
    if (!edit_before_) return;
    doc_.image = std::move(*edit_before_);
    edit_before_.reset();
    touch();
}

void Painter::push_sheet(Sheet after, const std::string& label) {
    std::optional<Sheet> before = doc_.sheet;
    undo_.push(std::make_unique<SheetCommand>(doc_, std::move(before), std::move(after), label));
}

void Painter::push_image(vs::Image after, std::optional<Sheet> sheet_after, const std::string& label) {
    commit_floating();
    undo_.push(std::make_unique<ImageCommand>(doc_, doc_.image, std::move(after), doc_.sheet, std::move(sheet_after), label));
    selection_.reset();
}

void Painter::track_sheet_item(const char* label) {
    if (!doc_.sheet) return;
    // The widget edits doc_.sheet live; the value before the first change is kept so that the
    // whole drag / typing session becomes one undo step when the widget is released.
    if (ImGui::IsItemActivated()) sheet_before_ = *doc_.sheet;
    if (ImGui::IsItemEdited()) ++doc_.sheet_revision;
    if (ImGui::IsItemDeactivated()) {
        if (ImGui::IsItemDeactivatedAfterEdit() && sheet_before_) {
            Sheet after = *doc_.sheet;
            doc_.sheet = std::move(*sheet_before_);
            push_sheet(std::move(after), label);
        }
        sheet_before_.reset();
    }
}

void Painter::undo() {
    if (drag_ || ImGui::IsAnyItemActive()) return;
    commit_floating();
    if (undo_.undo()) status_ = "Undid";
}

void Painter::redo() {
    if (drag_ || ImGui::IsAnyItemActive()) return;
    commit_floating();
    if (undo_.redo()) status_ = "Redid";
}

vs::FrameRect* Painter::selected_frame() {
    if (!doc_.sheet || sel_frame_ < 0 || sel_frame_ >= static_cast<int>(doc_.sheet->data.frames.size())) return nullptr;
    return &doc_.sheet->data.frames[static_cast<std::size_t>(sel_frame_)];
}

vs::AnimationDef* Painter::selected_animation() {
    if (!doc_.sheet || sel_anim_ < 0 || sel_anim_ >= static_cast<int>(doc_.sheet->data.animations.size())) return nullptr;
    return &doc_.sheet->data.animations[static_cast<std::size_t>(sel_anim_)];
}

IRect Painter::view_rect() const {
    const IRect all = bounds(doc_.image);
    if (view_ == View::Frame && doc_.sheet && sel_frame_ >= 0 && sel_frame_ < static_cast<int>(doc_.sheet->data.frames.size())) {
        const IRect r = intersect(frame_rect(doc_.sheet->data.frames[static_cast<std::size_t>(sel_frame_)]), all);
        if (!r.empty()) return r;
    }
    return all;
}

IRect Painter::edit_clip() const {
    IRect clip = view_rect();
    if (selection_ && !floating_) clip = intersect(clip, *selection_);
    return clip;
}

void Painter::use_color(vs::Color c) {
    std::erase(recent_, c);
    recent_.push_front(c);
    while (recent_.size() > 16) recent_.pop_back();
}

}  // namespace painter
