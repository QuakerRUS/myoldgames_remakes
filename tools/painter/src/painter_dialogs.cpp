// Painter: the "New" and "Canvas size" popups.
#include <algorithm>

#include "painter.hpp"

namespace painter {

namespace {

bool ok_cancel(bool& ok) {
    ImGui::Spacing();
    ok = ImGui::Button("OK", {110, 0}) || ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter);
    ImGui::SameLine();
    const bool cancel = ImGui::Button("Cancel", {110, 0}) || ImGui::IsKeyPressed(ImGuiKey_Escape);
    return ok || cancel;
}

}  // namespace

void Painter::draw_new_popup() {
    if (open_new_popup_) {
        ImGui::OpenPopup("New document");
        open_new_popup_ = false;
    }
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, {0.5f, 0.5f});
    if (!ImGui::BeginPopupModal("New document", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) return;
    ImGui::SetNextItemWidth(160);
    ImGui::InputInt("Width", &new_w_);
    ImGui::SetNextItemWidth(160);
    ImGui::InputInt("Height", &new_h_);
    new_w_ = std::clamp(new_w_, 1, 8192);
    new_h_ = std::clamp(new_h_, 1, 8192);
    for (int s : {16, 24, 32, 64, 128, 256, 512}) {
        ImGui::PushID(s);
        if (ImGui::SmallButton(std::to_string(s).c_str())) new_w_ = new_h_ = s;
        ImGui::PopID();
        ImGui::SameLine();
    }
    ImGui::NewLine();
    ImGui::Checkbox("Sprite sheet (PNG + JSON)", &new_sheet_);
    bool ok = false;
    if (ok_cancel(ok)) {
        if (ok) new_document(new_w_, new_h_, new_sheet_);
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}

void Painter::draw_resize_popup() {
    if (open_resize_popup_) {
        ImGui::OpenPopup("Canvas size");
        open_resize_popup_ = false;
    }
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, {0.5f, 0.5f});
    if (!ImGui::BeginPopupModal("Canvas size", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) return;
    ImGui::TextDisabled("Current: %dx%d. The content stays at the top-left;", doc_.image.width, doc_.image.height);
    ImGui::TextDisabled("new pixels are transparent.");
    ImGui::SetNextItemWidth(160);
    ImGui::InputInt("Width", &resize_w_);
    ImGui::SetNextItemWidth(160);
    ImGui::InputInt("Height", &resize_h_);
    resize_w_ = std::clamp(resize_w_, 1, 8192);
    resize_h_ = std::clamp(resize_h_, 1, 8192);
    if (doc_.sheet && (resize_w_ < doc_.image.width || resize_h_ < doc_.image.height))
        ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), "Shrinking may leave frames outside the image.");
    bool ok = false;
    if (ok_cancel(ok)) {
        if (ok && (resize_w_ != doc_.image.width || resize_h_ != doc_.image.height))
            push_image(resized_canvas(doc_.image, resize_w_, resize_h_), doc_.sheet, "Canvas size");
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}

}  // namespace painter
