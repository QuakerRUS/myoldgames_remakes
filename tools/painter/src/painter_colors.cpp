// Painter: the colour editor (RGBA picker, hex), recent colours and the palette.
#include <algorithm>
#include <cmath>

#include <vs/engine/json_file.hpp>

#include <imgui.h>
#include <imgui_stdlib.h>

#include "painter.hpp"
#include "ui_util.hpp"

namespace painter {

bool Painter::color_swatch(const char* id, vs::Color c, ImVec2 size, bool outlined) {
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const bool clicked = ImGui::InvisibleButton(id, size, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
    ImDrawList* d = ImGui::GetWindowDrawList();
    const ImVec2 q{p.x + size.x, p.y + size.y};
    if (c.a < 255) draw_checker(d, p, q, p, q, std::max(3.0f, std::floor(size.x / 4)), IM_COL32(90, 90, 90, 255), IM_COL32(150, 150, 150, 255));
    d->AddRectFilled(p, q, to_imcol(c));
    if (outlined) {
        d->AddRect({p.x - 2, p.y - 2}, {q.x + 2, q.y + 2}, IM_COL32(255, 255, 255, 255), 0, 0, 2.0f);
    } else if (ImGui::IsItemHovered()) {
        d->AddRect(p, q, IM_COL32(255, 255, 255, 180));
    }
    return clicked;
}

void Painter::draw_color_section() {
    section_title("COLOUR");
    vs::Color& target = edit_bg_ ? bg_ : fg_;

    // Foreground / background selector.
    const float sw = 34;
    if (color_swatch("##fg_sel", fg_, {sw, sw}, !edit_bg_)) edit_bg_ = false;
    ImGui::SetItemTooltip("Foreground #%s (LMB, Space, Enter)", to_hex(fg_, true).c_str());
    ImGui::SameLine(0, 10);
    if (color_swatch("##bg_sel", bg_, {sw, sw}, edit_bg_)) edit_bg_ = true;
    ImGui::SetItemTooltip("Background #%s (RMB, Backspace)", to_hex(bg_, true).c_str());
    ImGui::SameLine(0, 10);
    ImGui::BeginGroup();
    ImGui::TextUnformatted(edit_bg_ ? "Background" : "Foreground");
    if (!hex_editing_) hex_buf_ = to_hex(target, true);
    ImGui::SetNextItemWidth(92);
    ImGui::InputText("##hex", &hex_buf_, ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase);
    hex_editing_ = ImGui::IsItemActive();
    if (ImGui::IsItemDeactivatedAfterEdit())
        if (const std::optional<vs::Color> c = parse_hex(hex_buf_)) target = *c;
    ImGui::SetItemTooltip("RRGGBBAA");
    ImGui::SameLine();
    if (ImGui::SmallButton("Swap")) std::swap(fg_, bg_);
    ImGui::EndGroup();

    float col[4] = {target.r / 255.0f, target.g / 255.0f, target.b / 255.0f, target.a / 255.0f};
    ImGui::SetNextItemWidth(std::min(ImGui::GetContentRegionAvail().x - 4, 230.0f));
    const ImGuiColorEditFlags flags = ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoLabel |
                                      ImGuiColorEditFlags_PickerHueBar | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaPreviewHalf;
    if (ImGui::ColorPicker4("##picker", col, flags)) target = from_vec4(col);
    ImGui::SetNextItemWidth(-1);
    if (ImGui::ColorEdit4("##rgba", col, ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_NoSmallPreview | ImGuiColorEditFlags_DisplayRGB |
                                             ImGuiColorEditFlags_Uint8 | ImGuiColorEditFlags_AlphaBar))
        target = from_vec4(col);

    // Recent colours.
    ImGui::TextDisabled("Recent");
    const float rs = 18;
    for (std::size_t i = 0; i < 16; ++i) {
        if (i % 8 != 0) ImGui::SameLine(0, 3);
        ImGui::PushID(static_cast<int>(i));
        if (i < recent_.size()) {
            const vs::Color c = recent_[i];
            if (color_swatch("##recent", c, {rs, rs}, false)) fg_ = c;
            if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) bg_ = c;
            ImGui::SetItemTooltip("#%s", to_hex(c, true).c_str());
        } else {
            const ImVec2 p = ImGui::GetCursorScreenPos();
            ImGui::Dummy({rs, rs});
            ImGui::GetWindowDrawList()->AddRect(p, {p.x + rs, p.y + rs}, IM_COL32(255, 255, 255, 30));
        }
        ImGui::PopID();
    }
    ImGui::Spacing();
}

void Painter::draw_palette_section(float height) {
    const bool focused = focus_ == Focus::Palette;
    section_title(focused ? "PALETTE  [keyboard: arrows, Enter, Backspace, Tab]" : "PALETTE  (Tab)");
    ImGui::TextDisabled("%d colours from %s", static_cast<int>(palette_.size()), palette_source_.c_str());
    const float buttons_h = ImGui::GetFrameHeightWithSpacing();
    const float list_h = std::max(60.0f, height - buttons_h - ImGui::GetTextLineHeightWithSpacing() * 2 - 12);
    ImGui::BeginChild("palette", {0, list_h}, ImGuiChildFlags_FrameStyle);
    const float s = 20, gap = 3;
    palette_cols_ = std::max(1, static_cast<int>((ImGui::GetContentRegionAvail().x + gap) / (s + gap)));
    for (int i = 0; i < static_cast<int>(palette_.size()); ++i) {
        const vs::Color c = palette_[static_cast<std::size_t>(i)];
        if (i % palette_cols_ != 0) ImGui::SameLine(0, gap);
        ImGui::PushID(i);
        const ImVec2 p = ImGui::GetCursorScreenPos();
        if (color_swatch("##c", c, {s, s}, false)) {
            fg_ = c;
            palette_sel_ = i;
        }
        if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
            bg_ = c;
            palette_sel_ = i;
        }
        ImGui::SetItemTooltip("#%s  (LMB foreground, RMB background)", to_hex(c, true).c_str());
        ImDrawList* d = ImGui::GetWindowDrawList();
        if (c == fg_) d->AddTriangleFilled(p, {p.x + 8, p.y}, {p.x, p.y + 8}, IM_COL32(255, 255, 255, 255));
        if (c == bg_) d->AddTriangleFilled({p.x + s, p.y + s}, {p.x + s - 8, p.y + s}, {p.x + s, p.y + s - 8}, IM_COL32(255, 255, 255, 200));
        if (i == palette_sel_) {
            d->AddRect({p.x - 2, p.y - 2}, {p.x + s + 2, p.y + s + 2}, focused ? kCursor : IM_COL32(255, 204, 64, 110), 0, 0, 2.0f);
            if (focused && ImGui::GetScrollMaxY() > 0) {
                const float y = p.y - ImGui::GetWindowPos().y;
                if (y < 0 || y + s > ImGui::GetWindowHeight()) ImGui::SetScrollHereY(0.5f);
            }
        }
        ImGui::PopID();
    }
    ImGui::EndChild();
    if (ImGui::IsItemClicked()) focus_ = Focus::Palette;

    if (ImGui::SmallButton("From image")) {
        palette_ = palette_from_image(doc_.image);
        palette_source_ = "image";
    }
    ImGui::SetItemTooltip("Rebuild from the image's colours (sorted)");
    ImGui::SameLine();
    if (ImGui::SmallButton("+")) {
        const vs::Color c = edit_bg_ ? bg_ : fg_;
        if (std::find(palette_.begin(), palette_.end(), c) == palette_.end()) palette_.push_back(c);
        palette_source_ = "edits";
    }
    ImGui::SetItemTooltip("Add the edited colour");
    ImGui::SameLine();
    if (button_enabled("-", !palette_.empty() && palette_sel_ < static_cast<int>(palette_.size()))) {
        palette_.erase(palette_.begin() + palette_sel_);
        palette_source_ = "edits";
    }
    ImGui::SetItemTooltip("Remove the marked colour");
    ImGui::SameLine();
    if (ImGui::SmallButton("Sort")) sort_palette(palette_);
    ImGui::SameLine();
    if (ImGui::SmallButton("Load")) load_palette_dialog();
    ImGui::SetItemTooltip(".hex or .gpl");
    ImGui::SameLine();
    if (ImGui::SmallButton("Save")) save_palette_dialog();
    ImGui::SetItemTooltip(".hex (with alpha) or .gpl (GIMP)");
}

void Painter::load_palette_dialog() {
    dialogs_.open_file(window_, {{"Palette", "hex;gpl"}, {"Lospec HEX", "hex"}, {"GIMP palette", "gpl"}}, last_dir_,
                       [this](const std::filesystem::path& chosen) {
                           if (chosen.empty()) return;
                           try {
                               palette_ = load_palette(chosen);
                               palette_source_ = vs::path_to_utf8(chosen.filename());
                               palette_sel_ = 0;
                           } catch (const std::exception& e) {
                               message_.title = "Palette";
                               message_.open(e.what());
                           }
                       });
}

void Painter::save_palette_dialog() {
    const std::filesystem::path suggestion = last_dir_ / (doc_.path.empty() ? std::filesystem::path("palette.hex") : doc_.path.stem() += ".hex");
    dialogs_.save_file(window_, {{"Lospec HEX", "hex"}, {"GIMP palette", "gpl"}}, suggestion, [this](const std::filesystem::path& chosen) {
        if (chosen.empty()) return;
        std::filesystem::path p = chosen;
        if (!p.has_extension()) p += ".hex";
        try {
            save_palette(p, palette_);
            status_ = "Palette saved: " + vs::path_to_utf8(p.filename());
        } catch (const std::exception& e) {
            message_.title = "Palette";
            message_.open(e.what());
        }
    });
}

}  // namespace painter
