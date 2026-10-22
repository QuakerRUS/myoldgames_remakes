#include <vs/editor/confirm_prompt.hpp>

#include <imgui.h>

namespace vs {

namespace {
// Keys are ignored in the frame that opened the popup (the key that opened it is still "pressed").
bool key(ImGuiKey k, int opened_frame) { return ImGui::GetFrameCount() > opened_frame && ImGui::IsKeyPressed(k, false); }
}  // namespace

void ConfirmPrompt::open(std::string message, std::function<void(PromptChoice)> on_choice, bool with_cancel) {
    message_ = std::move(message);
    on_choice_ = std::move(on_choice);
    with_cancel_ = with_cancel;
    opened_frame_ = ImGui::GetCurrentContext() ? ImGui::GetFrameCount() : 0;
    open_ = true;
    request_open_ = true;
}

void ConfirmPrompt::draw() {
    if (!open_) return;
    const std::string id = title + "##confirm";
    if (request_open_) {
        ImGui::OpenPopup(id.c_str());
        request_open_ = false;
    }
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, {0.5f, 0.5f});
    if (!ImGui::BeginPopupModal(id.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
        return;
    ImGui::TextUnformatted(message_.c_str());
    ImGui::Spacing();
    int choice = -1;
    if (ImGui::Button("Yes (Y)", {110, 0}) || key(ImGuiKey_Y, opened_frame_) || key(ImGuiKey_Enter, opened_frame_) ||
        key(ImGuiKey_KeypadEnter, opened_frame_))
        choice = 0;
    ImGui::SameLine();
    if (ImGui::Button("No (N)", {110, 0}) || key(ImGuiKey_N, opened_frame_)) choice = 1;
    if (with_cancel_) {
        ImGui::SameLine();
        if (ImGui::Button("Cancel (Esc)", {110, 0}) || key(ImGuiKey_Escape, opened_frame_) ||
            key(ImGuiKey_C, opened_frame_))
            choice = 2;
    } else if (key(ImGuiKey_Escape, opened_frame_)) {
        choice = 1;
    }
    if (choice >= 0) {
        ImGui::CloseCurrentPopup();
        open_ = false;
        auto cb = std::move(on_choice_);
        ImGui::EndPopup();
        if (cb) cb(choice == 0 ? PromptChoice::Yes : choice == 1 ? PromptChoice::No : PromptChoice::Cancel);
        return;
    }
    ImGui::EndPopup();
}

void MessagePopup::open(std::string message) {
    message_ = std::move(message);
    opened_frame_ = ImGui::GetCurrentContext() ? ImGui::GetFrameCount() : 0;
    open_ = true;
    request_open_ = true;
}

void MessagePopup::draw() {
    if (!open_) return;
    const std::string id = title + "##message";
    if (request_open_) {
        ImGui::OpenPopup(id.c_str());
        request_open_ = false;
    }
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, {0.5f, 0.5f});
    if (!ImGui::BeginPopupModal(id.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
        return;
    ImGui::TextUnformatted(message_.c_str());
    ImGui::Spacing();
    if (ImGui::Button("OK", {110, 0}) || key(ImGuiKey_Enter, opened_frame_) || key(ImGuiKey_Escape, opened_frame_) ||
        key(ImGuiKey_KeypadEnter, opened_frame_)) {
        ImGui::CloseCurrentPopup();
        open_ = false;
    }
    ImGui::EndPopup();
}

}  // namespace vs
