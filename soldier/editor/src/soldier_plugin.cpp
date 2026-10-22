#include "soldier_plugin.hpp"

#include <algorithm>
#include <cmath>
#include <system_error>

#include <vs/engine/error.hpp>
#include <vs/engine/json_file.hpp>
#include <vs/engine/storage.hpp>

#include "level_io.hpp"
#include "palette.hpp"
#include "placement.hpp"
#include "soldier/level.hpp"
#include "soldier/tile.hpp"

namespace soldier_editor {
namespace {

constexpr ImVec4 kErrorColor{1.0f, 0.45f, 0.40f, 1.0f};
constexpr ImVec4 kWarningColor{1.0f, 0.80f, 0.35f, 1.0f};
constexpr ImVec4 kOkColor{0.45f, 0.90f, 0.45f, 1.0f};
constexpr ImVec4 kSelectedColor{1.0f, 0.86f, 0.25f, 1.0f};
constexpr ImU32 kCursorActive = IM_COL32(255, 220, 64, 255);
constexpr ImU32 kCursorInactive = IM_COL32(255, 220, 64, 110);

std::string tile_label(int code) {
    std::string s = tile_display_name(code);
    if (code >= 0 && code < soldier::kTileCount)
        s += std::string("  [") + std::to_string(code) + ", '" + soldier::tile_char(static_cast<soldier::Tile>(code)) + "']";
    return s;
}

TilePainter::CellFn cell_fn(const vs::CellContext& c) {
    return [&c](int x, int y) { return c.at(x, y, TilePainter::kOutside); };
}

}  // namespace

SoldierPlugin::SoldierPlugin(std::filesystem::path dev_data_dir) : dev_data_dir_(std::move(dev_data_dir)) {}

// ---------------------------------------------------------------- tiles and drawing

std::vector<vs::TileInfo> SoldierPlugin::palette() const {
    std::vector<vs::TileInfo> out;
    for (const PaletteGroup& g : palette_groups())
        for (const PaletteEntry& e : g.entries) out.push_back({e.code, e.name, "", {128, 128, 128, 255}, g.label});
    return out;
}

int SoldierPlugin::palette_columns() const { return kPaletteColumns; }

bool SoldierPlugin::draw_field_cell(const vs::CellContext& c) const {
    painter_.draw_cell(c.draw, cell_fn(c), c.x, c.y, c.min, c.cell, c.time);
    return true;
}

void SoldierPlugin::draw_cell_overlay(const vs::CellContext& c) const {
    painter_.draw_overlay(c.draw, c.code, c.min, c.cell);
    painter_.draw_marks(c.draw, c.code, c.min, c.cell);
}

bool SoldierPlugin::draw_hover(vs::TileEditor&, const vs::GridView::Frame& f, int x, int y) {
    f.draw->AddRect(f.cell_min(x, y), f.cell_max(x, y), IM_COL32(255, 255, 255, 140));
    return true;
}

bool SoldierPlugin::draw_cursor(vs::TileEditor&, const vs::GridView::Frame& f, int x, int y, bool active) {
    const ImVec2 a = f.cell_min(x, y), b = f.cell_max(x, y);
    f.draw->AddRect({a.x - 1, a.y - 1}, {b.x + 1, b.y + 1}, IM_COL32(0, 0, 0, 160), 0, 0, 5.0f);
    f.draw->AddRect(a, b, active ? kCursorActive : kCursorInactive, 0, 0, 3.0f);
    return true;
}

// ---------------------------------------------------------------- palette panel

vs::PaletteStyle SoldierPlugin::palette_style() const {
    vs::PaletteStyle s;
    s.spacing_scale = 0.5f;
    s.fill_width = true;
    s.selection_rounding = 4.0f;
    s.selection_inactive = IM_COL32(255, 255, 255, 220);
    s.pick_returns_to_field = true;
    s.wrap_navigation = true;
    return s;
}

std::string SoldierPlugin::palette_header(bool active) const {
    return active ? "PALETTE  (active - arrows, Enter picks)" : "PALETTE  (Tab to select with keys)";
}

bool SoldierPlugin::draw_palette_item(ImDrawList* draw, const vs::TileInfo& tile, ImVec2 p, ImVec2 q, bool hovered) const {
    const float button = q.x - p.x;
    const float icon = button >= 70.0f ? 64.0f : (button >= 38.0f ? 32.0f : button - 4.0f);
    draw->AddRectFilled(p, q, hovered ? IM_COL32(70, 76, 90, 255) : IM_COL32(40, 44, 52, 255), 4.0f);
    const float off = std::floor((button - icon) * 0.5f);
    draw->PushClipRect(p, q, true);
    painter_.draw_icon(draw, tile.code, {p.x + off, p.y + off}, icon, ImGui::GetTime());
    draw->PopClipRect();
    return true;
}

void SoldierPlugin::palette_tooltip(const vs::TileInfo& tile) const {
    ImGui::BeginTooltip();
    ImGui::TextUnformatted(tile_label(tile.code).c_str());
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 22);
    ImGui::TextDisabled("%s", std::string(tile_hint(tile.code)).c_str());
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
}

void SoldierPlugin::draw_palette_footer(vs::TileEditor&, const vs::TileInfo& selected) {
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextColored(kSelectedColor, "Selected: %s", tile_label(selected.code).c_str());
    ImGui::PushTextWrapPos(0.0f);
    ImGui::TextUnformatted(std::string(tile_hint(selected.code)).c_str());
    ImGui::PopTextWrapPos();
    ImGui::Spacing();
}

// ---------------------------------------------------------------- rules

std::vector<vs::CellChange> SoldierPlugin::place(const vs::TileDocument& doc, int x, int y, int code) const {
    return placement_changes(doc, x, y, code);
}

std::string SoldierPlugin::place_label(int code) const {
    return code == erase_value() ? "Erase" : "Place " + tile_display_name(code);
}

void SoldierPlugin::on_placed(vs::TileEditor& editor, int x, int y, int code) {
    if (!is_start_code(code)) return;
    const std::string warning = start_placement_warning(editor.document(), x, y);
    editor.set_status(warning.empty() ? "Start moved to (" + std::to_string(x) + ", " + std::to_string(y) + ")" : warning);
}

std::vector<std::string> SoldierPlugin::validate(const vs::TileDocument& doc) const { return check_level(doc).errors; }

vs::ValidationResult SoldierPlugin::check(const vs::TileDocument& doc) const {
    CheckResult r = check_level(doc);
    return {std::move(r.errors), std::move(r.warnings)};
}

bool SoldierPlugin::properties_ui(vs::TileDocument& doc) {
    bool changed = false;
    const int w = doc.grid.width(), h = doc.grid.height();
    if (w != seen_w_ || h != seen_h_) pending_w_ = seen_w_ = w, pending_h_ = seen_h_ = h;

    ImGui::PushItemWidth(ImGui::GetFontSize() * 7);
    int number = doc_number(doc);
    if (ImGui::InputInt("Level number", &number)) {
        doc.properties["number"] = std::clamp(number, 1, 999);
        changed = true;
    }

    ImGui::InputInt("Width", &pending_w_);
    pending_w_ = std::clamp(pending_w_, 1, soldier::kMaxLevelSize);
    ImGui::InputInt("Height", &pending_h_);
    pending_h_ = std::clamp(pending_h_, 1, soldier::kMaxLevelSize);
    const bool size_differs = pending_w_ != w || pending_h_ != h;
    ImGui::BeginDisabled(!size_differs);
    if (ImGui::Button("Resize")) {
        doc = resized(doc, pending_w_, pending_h_);
        changed = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Revert")) pending_w_ = w, pending_h_ = h;
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::TextDisabled("1..100, keeps the top-left part");

    int limit = doc_time_limit(doc);
    if (ImGui::InputInt("Time limit", &limit, 1, 10)) {
        doc.properties["time_limit"] = std::clamp(limit, 0, 32767);
        changed = true;
    }
    ImGui::PopItemWidth();
    limit = doc_time_limit(doc);
    if (limit == 0)
        ImGui::TextDisabled("0 = no time limit");
    else
        ImGui::TextDisabled("%d timer units = %d:%02d at normal speed (1 unit = 2 s)", limit, limit * 2 / 60, limit * 2 % 60);
    return changed;
}

// ---------------------------------------------------------------- panels

bool SoldierPlugin::draw_check(vs::TileEditor& editor, const vs::ValidationResult& r) {
    // The file name is not part of the document, so this warning is added here.
    const std::string name_warning =
        editor.path().empty() ? std::string() : file_number_warning(editor.path(), doc_number(editor.document()));
    if (r.errors.empty() && r.warnings.empty() && name_warning.empty())
        ImGui::TextColored(kOkColor, "No problems: ready to test-play (F5)");
    ImGui::PushTextWrapPos(0.0f);
    for (const std::string& e : r.errors) ImGui::TextColored(kErrorColor, "Error: %s", e.c_str());
    if (!name_warning.empty()) ImGui::TextColored(kWarningColor, "Warning: %s", name_warning.c_str());
    for (const std::string& w : r.warnings) ImGui::TextColored(kWarningColor, "Warning: %s", w.c_str());
    ImGui::PopTextWrapPos();
    return true;
}

void SoldierPlugin::draw_panels(vs::TileEditor&, vs::SidePanelSlot slot) {
    if (slot != vs::SidePanelSlot::AfterCheck || !ImGui::CollapsingHeader("Controls")) return;
    ImGui::PushTextWrapPos(0.0f);
    ImGui::TextUnformatted(
        "Tab: switch field / palette\n"
        "Arrows: move the frame cursor (wraps)\n"
        "Space / Enter: place the tile (palette: pick it)\n"
        "Backspace / Delete: floor\n"
        "LMB: paint, RMB: erase, wheel: zoom, MMB: pan\n"
        "Ctrl+Z / Ctrl+Y: undo / redo\n"
        "Ctrl+N / O / S, Ctrl+Shift+S: files\n"
        "F5: test-play, Esc: quit");
    ImGui::PopTextWrapPos();
}

bool SoldierPlugin::draw_status_bar(vs::TileEditor& editor) {
    doc_number_ = doc_number(editor.document());  // drawn every frame
    const vs::TileGrid& g = editor.document().grid;
    const int cx = editor.cursor_x(), cy = editor.cursor_y(), hx = editor.hover_x(), hy = editor.hover_y();
    std::string text = std::to_string(g.width()) + "x" + std::to_string(g.height()) + "   cursor " + std::to_string(cx) + "," +
                       std::to_string(cy);
    if (g.in_bounds(cx, cy)) text += ": " + tile_display_name(g.at(cx, cy));
    if (g.in_bounds(hx, hy)) text += "   mouse " + std::to_string(hx) + "," + std::to_string(hy) + ": " + tile_display_name(g.at(hx, hy));
    text += "   |   " + editor.status();
    if (editor.modified()) text += "   (modified)";
    if (editor.status().rfind("Warning", 0) == 0)
        ImGui::TextColored(kWarningColor, "%s", text.c_str());
    else
        ImGui::TextUnformatted(text.c_str());
    return true;
}

float SoldierPlugin::side_panel_width(float avail_width) const { return std::min(ImGui::GetFontSize() * 22.0f, avail_width * 0.45f); }

vs::GridPos SoldierPlugin::initial_cursor(const vs::TileDocument&, bool opened) const {
    return opened ? vs::GridPos{0, 0} : vs::GridPos{1, 1};
}

// ---------------------------------------------------------------- files

vs::TileDocument SoldierPlugin::new_document() const { return make_new_document(); }

std::string SoldierPlugin::suggest_file_name(const std::filesystem::path&) const { return level_file_name(doc_number_); }

vs::TileDocument SoldierPlugin::load(const std::filesystem::path& path) const { return load_document(path); }

void SoldierPlugin::save(const std::filesystem::path& path, const vs::TileDocument& doc) const { save_document(path, doc); }

std::filesystem::path SoldierPlugin::levels_dir() const {
    std::error_code ec;
    if (!dev_data_dir_.empty() && std::filesystem::is_directory(dev_data_dir_ / "levels", ec)) return dev_data_dir_ / "levels";
    if (const auto p = vs::find_asset("levels"); !p.empty()) return p;
    return vs::exe_dir();
}

std::filesystem::path SoldierPlugin::find_game_exe() const {
    std::error_code ec;
    const std::filesystem::path here = vs::exe_dir();
    if (std::filesystem::is_regular_file(here / "soldier.exe", ec)) return here / "soldier.exe";
    // A build folder bin/<Config>/Soldier/ where only the editor was built: try the other configs.
    std::filesystem::path dir = here;
    if (!dir.has_filename()) dir = dir.parent_path();  // exe_dir() ends with a separator
    for (const char* config : {"Debug", "Release", "RelWithDebInfo", "MinSizeRel"}) {
        const auto p = dir.parent_path().parent_path() / config / dir.filename() / "soldier.exe";
        if (std::filesystem::is_regular_file(p, ec)) return p;
    }
    return {};
}

std::filesystem::path SoldierPlugin::test_play_file() const {
    return std::filesystem::temp_directory_path() / "soldier_editor" / "testplay.json";
}

bool SoldierPlugin::test_play(const std::filesystem::path& level_file) {
    const std::filesystem::path exe = find_game_exe();
    if (exe.empty())
        throw vs::Error("soldier.exe was not found next to the editor (" + vs::path_to_utf8(vs::exe_dir()) +
                        ").\nBuild the 'soldier' target in the same build folder.");
    if (!vs::launch_process({vs::path_to_utf8(exe), "--test-level", vs::path_to_utf8(level_file)}))
        throw vs::Error("Could not start " + vs::path_to_utf8(exe));
    return true;
}

}  // namespace soldier_editor
