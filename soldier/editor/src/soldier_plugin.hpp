// vs::TileEditorPlugin for Soldier 2.0: the whole editor is vs::TileEditor (engine/editor)
// driven by this plugin: grouped palette with hints, the game's autotiled drawing, placement
// rules, errors vs warnings, properties, level files, the levels folder and F5 test-play.
#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include <vs/editor/tile_editor.hpp>

#include "level_check.hpp"
#include "tile_painter.hpp"

namespace soldier_editor {

class SoldierPlugin : public vs::TileEditorPlugin {
public:
    // `dev_data_dir`: the source tree's data folder (VS_DEV_DATA_DIR) or empty.
    explicit SoldierPlugin(std::filesystem::path dev_data_dir = {});

    std::string title() const override { return "Soldier 2.0 Map Editor"; }
    std::string game_id() const override { return "Soldier"; }
    void load_resources(vs::Renderer& renderer) override { painter_.load(renderer); }

    // --- tiles and drawing ---
    std::vector<vs::TileInfo> palette() const override;
    int initial_palette_index() const override { return 1; }  // wall
    int erase_value() const override { return 0; }
    int palette_columns() const override;
    bool draw_field_cell(const vs::CellContext& c) const override;
    void draw_cell_overlay(const vs::CellContext& c) const override;
    int overdraw_rows() const override { return 1; }  // guillotines stick out upwards
    bool draw_hover(vs::TileEditor& editor, const vs::GridView::Frame& f, int x, int y) override;
    bool draw_cursor(vs::TileEditor& editor, const vs::GridView::Frame& f, int x, int y, bool active) override;

    // --- palette panel ---
    vs::PaletteStyle palette_style() const override;
    std::string palette_header(bool active) const override;
    bool draw_palette_item(ImDrawList* draw, const vs::TileInfo& tile, ImVec2 min, ImVec2 max, bool hovered) const override;
    void palette_tooltip(const vs::TileInfo& tile) const override;
    void draw_palette_footer(vs::TileEditor& editor, const vs::TileInfo& selected) override;

    // --- rules ---
    std::vector<vs::CellChange> place(const vs::TileDocument& doc, int x, int y, int code) const override;
    std::string place_label(int code) const override;
    void on_placed(vs::TileEditor& editor, int x, int y, int code) override;
    std::vector<std::string> validate(const vs::TileDocument& doc) const override;  // errors only
    vs::ValidationResult check(const vs::TileDocument& doc) const override;
    bool properties_ui(vs::TileDocument& doc) override;

    // --- panels ---
    bool draw_check(vs::TileEditor& editor, const vs::ValidationResult& result) override;
    void draw_panels(vs::TileEditor& editor, vs::SidePanelSlot slot) override;
    bool draw_status_bar(vs::TileEditor& editor) override;
    float side_panel_width(float avail_width) const override;
    bool wrap_cursor() const override { return true; }
    vs::GridPos initial_cursor(const vs::TileDocument& doc, bool opened) const override;

    // --- files ---
    vs::TileDocument new_document() const override;
    std::vector<vs::FileFilter> file_filters() const override { return {{"Soldier level (JSON)", "json"}}; }
    std::string default_file_name() const override { return "level.json"; }
    // Save As of a new level suggests NN.json from its level number.
    std::string suggest_file_name(const std::filesystem::path& dir) const override;
    std::filesystem::path default_directory() const override { return levels_dir(); }
    vs::TileDocument load(const std::filesystem::path& path) const override;
    void save(const std::filesystem::path& path, const vs::TileDocument& doc) const override;
    std::string new_document_status() const override { return "New level"; }
    std::string unsaved_changes_question() const override { return "Save the level?"; }
    bool test_play(const std::filesystem::path& level_file) override;
    std::filesystem::path test_play_file() const override;

    // --- helpers ---
    const TilePainter& painter() const { return painter_; }
    // Where Open/Save dialogs start: the source tree's levels folder, else data/levels next to the exe.
    std::filesystem::path levels_dir() const;
    // soldier.exe next to the editor, or in a sibling configuration folder (bin/Debug <-> bin/Release).
    std::filesystem::path find_game_exe() const;

private:
    std::filesystem::path dev_data_dir_;
    TilePainter painter_;
    int pending_w_ = 0, pending_h_ = 0;  // size being typed (applied with the Resize button)
    int seen_w_ = -1, seen_h_ = -1;      // document size when pending_* was last synced
    int doc_number_ = 1;                 // level number of the open document (for suggest_file_name)
};

}  // namespace soldier_editor
