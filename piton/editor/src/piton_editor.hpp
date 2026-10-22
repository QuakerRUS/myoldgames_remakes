#pragma once
// The Piton map editor: vs::TileEditor (engine/editor, the Soldier 2.0 EDITOR model: field +
// palette, Tab switches the panel, arrows move the frame cursor, Space/Enter place, Backspace
// erases, Esc asks to save) driven by this plugin: the game's brick autotiles and grass, the
// 3-cell start snake placed as a document change, symmetry painting, the Tools menu/panel, the
// level check with its statistics.
#include <algorithm>
#include <array>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include <imgui.h>

#include <vs/editor/tile_editor.hpp>
#include <vs/engine/sprite_sheet.hpp>

#include "level_doc.hpp"

namespace piton_editor {

class PitonPlugin : public vs::TileEditorPlugin {
public:
    // `default_dir`: where Open / Save As start (the levels folder).
    explicit PitonPlugin(std::filesystem::path default_dir);

    std::string title() const override { return "Piton Map Editor"; }
    std::string game_id() const override { return "Piton"; }
    void load_resources(vs::Renderer& renderer) override;

    // --- tiles and drawing ---
    std::vector<vs::TileInfo> palette() const override;
    float cell_size() const override { return 24.0f; }  // sprite size = screen px per cell at zoom 1
    int erase_value() const override { return kEmpty; }
    int palette_columns() const override { return 2; }
    bool draw_field_cell(const vs::CellContext& c) const override;
    void draw_field_overlay(vs::TileEditor& editor, const vs::GridView::Frame& f) override;
    bool draw_hover(vs::TileEditor& editor, const vs::GridView::Frame& f, int x, int y) override;
    bool draw_cursor(vs::TileEditor& editor, const vs::GridView::Frame& f, int x, int y, bool active) override;

    // --- palette panel ---
    vs::PaletteStyle palette_style() const override;
    bool draw_palette_item(ImDrawList* draw, const vs::TileInfo& tile, ImVec2 min, ImVec2 max, bool hovered) const override;
    void palette_tooltip(const vs::TileInfo& tile) const override;
    void draw_palette_footer(vs::TileEditor& editor, const vs::TileInfo& selected) override;

    // --- rules ---
    std::vector<vs::CellChange> place(const vs::TileDocument& doc, int x, int y, int code) const override;
    std::optional<vs::TileDocument> place_document(const vs::TileDocument& doc, int x, int y, int code) const override;
    bool paints_on_drag(int code) const override { return !startCodeDir(code); }
    std::string place_label(int code) const override;
    void on_placed(vs::TileEditor& editor, int x, int y, int code) override;
    // Errors: piton::LevelCheck::errors (the game refuses to test-play them); warnings: the rest.
    vs::ValidationResult check(const vs::TileDocument& doc) const override;
    bool errors_block_test_play() const override { return true; }  // warnings do not block F5
    bool report_errors_on_save() const override { return true; }    // saving is allowed, with a warning
    void draw_properties(vs::TileEditor& editor) override;

    // --- panels, menus, keys ---
    void draw_panels(vs::TileEditor& editor, vs::SidePanelSlot slot) override;
    bool draw_check(vs::TileEditor& editor, const vs::ValidationResult& result) override;
    void level_menu_items(vs::TileEditor& editor) override;
    void draw_menus(vs::TileEditor& editor) override;
    void handle_keys(vs::TileEditor& editor) override;
    bool view_shortcuts() const override { return true; }
    bool draw_status_bar(vs::TileEditor& editor) override;
    float side_panel_width(float avail_width) const override { return std::min(360.0f, avail_width * 0.4f); }
    vs::GridPos initial_cursor(const vs::TileDocument& doc, bool opened) const override;

    // --- files ---
    vs::TileDocument new_document() const override { return newDocument(); }
    std::vector<vs::FileFilter> file_filters() const override { return {{"Piton level (JSON)", "json"}}; }
    std::string suggest_file_name(const std::filesystem::path& dir) const override { return suggestFileName(dir); }
    std::filesystem::path default_directory() const override { return default_dir_; }
    vs::TileDocument load(const std::filesystem::path& path) const override { return loadDocument(path); }
    void save(const std::filesystem::path& path, const vs::TileDocument& doc) const override { saveDocument(path, doc); }
    std::string new_document_status() const override { return "New level"; }
    std::string unsaved_changes_question() const override { return "The level has unsaved changes. Save?"; }
    bool test_play(const std::filesystem::path& level_file) override;
    std::filesystem::path test_play_file() const override;

private:
    void draw_tools(vs::TileEditor& editor);
    void rotate_start(vs::TileEditor& editor);
    // Applies withStart(head, dir); reports in the status bar when the head had to move inwards.
    void move_start(vs::TileEditor& editor, piton::Cell head, piton::Dir dir, const std::string& label);
    void draw_image(ImDrawList* draw, const vs::SpriteSheet& sheet, const vs::FrameRect* f, ImVec2 a, ImVec2 b,
                    ImU32 tint = IM_COL32_WHITE) const;
    void draw_palette_icon(ImDrawList* draw, int code, ImVec2 a, ImVec2 b) const;
    void draw_start_snake(ImDrawList* draw, const vs::GridView::Frame& f, piton::Cell head, piton::Dir dir, ImU32 tint) const;
    const piton::LevelCheck& level_check(const vs::TileDocument& doc) const;  // cached

    std::filesystem::path default_dir_;
    vs::SpriteSheet tiles_;
    vs::SpriteSheet snake_;
    std::array<const vs::FrameRect*, 8> grass_{};
    std::array<const vs::FrameRect*, 16> wall_{};
    const vs::FrameRect* shade_n_ = nullptr;
    const vs::FrameRect* shade_w_ = nullptr;
    const vs::FrameRect* shade_nw_ = nullptr;
    std::array<const vs::FrameRect*, 5> head_{};  // indexed by int(Dir), 1..4
    std::array<const vs::FrameRect*, 5> tail_{};
    const vs::FrameRect* body_h_ = nullptr;
    const vs::FrameRect* body_v_ = nullptr;

    Symmetry symmetry_ = Symmetry::None;
    bool name_active_ = false;
    std::string name_buf_;

    mutable vs::TileDocument checked_doc_;
    mutable piton::LevelCheck check_;
    mutable bool check_valid_ = false;
};

}  // namespace piton_editor
