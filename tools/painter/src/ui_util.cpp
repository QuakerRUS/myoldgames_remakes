#include "ui_util.hpp"

#include <algorithm>
#include <cmath>

namespace painter {

vs::Color from_vec4(const float v[4]) {
    auto b = [](float f) { return static_cast<std::uint8_t>(std::lround(std::clamp(f, 0.0f, 1.0f) * 255.0f)); };
    return {b(v[0]), b(v[1]), b(v[2]), b(v[3])};
}

void section_title(const char* text) {
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
    ImGui::TextUnformatted(text);
    ImGui::PopStyleColor();
    const ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::GetWindowDrawList()->AddLine({p.x, p.y - 2}, {p.x + ImGui::GetContentRegionAvail().x, p.y - 2},
                                        ImGui::GetColorU32(ImGuiCol_Separator));
    ImGui::Dummy({0, 2});
}

bool button_enabled(const char* label, bool enabled, ImVec2 size) {
    ImGui::BeginDisabled(!enabled);
    const bool clicked = ImGui::Button(label, size);
    ImGui::EndDisabled();
    return clicked && enabled;
}

void help_marker(const char* text) {
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::BeginItemTooltip()) {
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 30.0f);
        ImGui::TextUnformatted(text);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

void draw_checker(ImDrawList* draw, ImVec2 a, ImVec2 b, ImVec2 clip_min, ImVec2 clip_max, float square, ImU32 dark, ImU32 light) {
    const ImVec2 lo{std::max(a.x, clip_min.x), std::max(a.y, clip_min.y)};
    const ImVec2 hi{std::min(b.x, clip_max.x), std::min(b.y, clip_max.y)};
    if (hi.x <= lo.x || hi.y <= lo.y) return;
    draw->AddRectFilled(lo, hi, dark);
    const int i0 = static_cast<int>(std::floor((lo.x - a.x) / square));
    const int j0 = static_cast<int>(std::floor((lo.y - a.y) / square));
    for (int j = j0;; ++j) {
        const float y = a.y + static_cast<float>(j) * square;
        if (y >= hi.y) break;
        for (int i = i0 + ((i0 + j) & 1);; i += 2) {
            const float x = a.x + static_cast<float>(i) * square;
            if (x >= hi.x) break;
            draw->AddRectFilled({std::max(x, lo.x), std::max(y, lo.y)}, {std::min(x + square, hi.x), std::min(y + square, hi.y)}, light);
        }
    }
}

void apply_painter_style() {
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding = 0;
    s.ChildRounding = 4;
    s.FrameRounding = 4;
    s.PopupRounding = 6;
    s.GrabRounding = 3;
    s.TabRounding = 4;
    s.ScrollbarRounding = 6;
    s.WindowBorderSize = 0;
    s.ChildBorderSize = 1;
    s.FrameBorderSize = 0;
    s.FramePadding = {7, 4};
    s.ItemSpacing = {6, 5};
    s.ItemInnerSpacing = {5, 4};
    s.WindowPadding = {9, 8};
    s.ScrollbarSize = 12;
    s.GrabMinSize = 9;

    ImVec4* c = s.Colors;
    auto rgb = [](int r, int g, int b, float a = 1.0f) { return ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, a); };
    c[ImGuiCol_Text] = rgb(222, 226, 232);
    c[ImGuiCol_TextDisabled] = rgb(128, 135, 148);
    c[ImGuiCol_WindowBg] = rgb(30, 32, 37);
    c[ImGuiCol_ChildBg] = rgb(36, 39, 45);
    c[ImGuiCol_PopupBg] = rgb(40, 43, 50, 0.98f);
    c[ImGuiCol_Border] = rgb(54, 58, 66);
    c[ImGuiCol_FrameBg] = rgb(48, 52, 60);
    c[ImGuiCol_FrameBgHovered] = rgb(58, 63, 73);
    c[ImGuiCol_FrameBgActive] = rgb(66, 72, 84);
    c[ImGuiCol_TitleBg] = rgb(30, 32, 37);
    c[ImGuiCol_TitleBgActive] = rgb(36, 39, 45);
    c[ImGuiCol_MenuBarBg] = rgb(26, 28, 32);
    c[ImGuiCol_ScrollbarBg] = rgb(30, 32, 37, 0.0f);
    c[ImGuiCol_ScrollbarGrab] = rgb(70, 75, 86);
    c[ImGuiCol_ScrollbarGrabHovered] = rgb(86, 92, 105);
    c[ImGuiCol_ScrollbarGrabActive] = rgb(100, 107, 122);
    c[ImGuiCol_CheckMark] = rgb(92, 172, 255);
    c[ImGuiCol_SliderGrab] = rgb(64, 156, 255);
    c[ImGuiCol_SliderGrabActive] = rgb(92, 172, 255);
    c[ImGuiCol_Button] = rgb(52, 57, 66);
    c[ImGuiCol_ButtonHovered] = rgb(64, 70, 82);
    c[ImGuiCol_ButtonActive] = rgb(44, 110, 190);
    c[ImGuiCol_Header] = rgb(44, 92, 150, 0.85f);
    c[ImGuiCol_HeaderHovered] = rgb(54, 104, 166, 0.80f);
    c[ImGuiCol_HeaderActive] = rgb(64, 120, 190);
    c[ImGuiCol_Separator] = rgb(56, 60, 69);
    c[ImGuiCol_Tab] = rgb(40, 43, 50);
    c[ImGuiCol_TabHovered] = rgb(64, 120, 190);
    c[ImGuiCol_TableHeaderBg] = rgb(44, 48, 56);
    c[ImGuiCol_TableBorderLight] = rgb(50, 54, 62);
    c[ImGuiCol_TableRowBgAlt] = rgb(255, 255, 255, 0.025f);
    c[ImGuiCol_ModalWindowDimBg] = rgb(0, 0, 0, 0.45f);
}

}  // namespace painter
