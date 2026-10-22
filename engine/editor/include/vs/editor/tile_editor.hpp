#pragma once
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include <imgui.h>

#include <vs/editor/confirm_prompt.hpp>
#include <vs/editor/file_dialogs.hpp>
#include <vs/editor/grid_view.hpp>
#include <vs/editor/palette_grid.hpp>
#include <vs/editor/save_flow.hpp>
#include <vs/editor/tile_document.hpp>
#include <vs/engine/math.hpp>
#include <vs/engine/undo_stack.hpp>

struct SDL_Window;

namespace vs {

class Renderer;
class SpriteSheet;
class TileEditor;

// One entry of the palette.
struct TileInfo {
    int code = 0;              // value stored in the grid (or a palette-only code, see place_document)
    std::string name;          // tooltip / status bar
    std::string icon_frame;    // frame in TileEditorPlugin::icons(); empty = coloured square
    Color color{128, 128, 128, 255};  // fallback colour when there is no icon
    // Palette section: consecutive entries with the same non-empty group form a section with
    // this title; every section starts on a new row.
    std::string group;
};

// Result of the level check: errors block F5 (or ask first, see errors_block_test_play);
// warnings are only shown.
struct ValidationResult {
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    bool ok() const { return errors.empty(); }
    bool operator==(const ValidationResult&) const = default;
};

// Everything a cell-drawing hook needs: the cell, its screen rectangle and the document (for
// neighbour-dependent tiles such as autotiled walls).
struct CellContext {
    ImDrawList* draw = nullptr;
    const TileDocument* doc = nullptr;
    int x = 0, y = 0;
    int code = 0;          // doc->grid.at(x, y)
    ImVec2 min, max;       // screen rectangle of the cell
    float cell = 1.0f;     // screen pixels per cell
    double time = 0.0;     // seconds (ImGui time), for animations
    // Code at (x + dx, y + dy), or `outside` beyond the grid.
    int at(int nx, int ny, int outside = -1) const {
        return doc && doc->grid.in_bounds(nx, ny) ? doc->grid.at(nx, ny) : outside;
    }
};

// Layout of the palette panel.
struct PaletteStyle {
    float item_height = 0.0f;      // 0: square items
    float spacing_scale = 1.0f;    // item gap = ImGui ItemSpacing.x * scale (both directions when != 1)
    bool fill_width = false;       // items fill the panel exactly (else the classic layout with outer gaps)
    float selection_rounding = 0;  // corner radius of the selection frame
    ImU32 selection_inactive = IM_COL32(255, 255, 255, 200);  // its colour while the field has the keyboard
    bool pick_returns_to_field = false;  // clicking an item also gives the keyboard back to the field
    bool wrap_navigation = false;  // arrows wrap around (row-aware, see palette_move); else clamp
};

// Where TileEditorPlugin::draw_panels is called in the side panel.
enum class SidePanelSlot { AfterProperties, AfterCheck };

// Everything game-specific about a map editor. Implement it and pass it to run_tile_editor().
// Only the pure virtual members are required; every other hook has a default that gives the
// plain editor described in engine/README.md. Hooks taking a TileEditor& may use its public API
// (document(), apply_document(), set_status(), view(), ...).
class TileEditorPlugin {
public:
    virtual ~TileEditorPlugin() = default;

    virtual std::string title() const = 0;    // window title, e.g. "Soldier Map Editor"
    virtual std::string game_id() const = 0;  // user data folder (layout ini, test-play file)
    // Called once after the renderer exists: load sprite sheets for icons() here.
    virtual void load_resources(Renderer& renderer) { (void)renderer; }

    // --- tiles ---
    virtual std::vector<TileInfo> palette() const = 0;
    virtual int initial_palette_index() const { return 0; }
    virtual const SpriteSheet* icons() const { return nullptr; }
    virtual float cell_size() const { return 32.0f; }  // screen px per cell at zoom 1
    virtual int erase_value() const { return 0; }      // Backspace / right mouse button
    virtual int palette_columns() const { return 4; }
    // Simple custom drawing of a cell from its code alone (field and palette); return true if
    // drawn (else the palette icon / coloured square is used).
    virtual bool draw_cell(ImDrawList* draw, int code, ImVec2 min, ImVec2 max) const {
        (void)draw, (void)code, (void)min, (void)max;
        return false;
    }

    // --- field drawing (all optional) ---
    // Draws field cell `c` (with neighbour access); return false to fall back to draw_cell /
    // the palette icon.
    virtual bool draw_field_cell(const CellContext& c) const {
        (void)c;
        return false;
    }
    // Second pass over the visible cells, after all cells: parts sticking out of a cell
    // (e.g. tall objects drawn over the cell above), editor-only marks.
    virtual void draw_cell_overlay(const CellContext& c) const { (void)c; }
    // Extra rows below the visible area that are still drawn (objects reaching upwards).
    virtual int overdraw_rows() const { return 0; }
    // After the cells, before hover and cursor: multi-cell objects, guides, markers.
    virtual void draw_field_overlay(TileEditor& editor, const GridView::Frame& f) { (void)editor, (void)f; }
    // Hover marker for cell (x, y) under the mouse; return true if drawn (default: thin frame).
    virtual bool draw_hover(TileEditor& editor, const GridView::Frame& f, int x, int y) {
        (void)editor, (void)f, (void)x, (void)y;
        return false;
    }
    // The keyboard frame cursor at (x, y); `active` = the field has the keyboard. Return true if drawn.
    virtual bool draw_cursor(TileEditor& editor, const GridView::Frame& f, int x, int y, bool active) {
        (void)editor, (void)f, (void)x, (void)y, (void)active;
        return false;
    }

    // --- palette panel (all optional) ---
    virtual PaletteStyle palette_style() const { return {}; }
    virtual std::string palette_header(bool active) const { return active ? "Palette  [active]" : "Palette  (Tab)"; }
    // Draws a whole palette item (background, icon, labels) into [min, max]; return false for the
    // default (the tile icon inset by 3 px).
    virtual bool draw_palette_item(ImDrawList* draw, const TileInfo& tile, ImVec2 min, ImVec2 max, bool hovered) const {
        (void)draw, (void)tile, (void)min, (void)max, (void)hovered;
        return false;
    }
    // Tooltip of a hovered palette item (inside an ImGui frame). Default: "<code>: <name>".
    virtual void palette_tooltip(const TileInfo& tile) const;
    // Between the palette items and the Properties section. Default: "Selected: ..." + separator.
    virtual void draw_palette_footer(TileEditor& editor, const TileInfo& selected);

    // --- rules ---
    // Cells to change when `code` is placed at (x, y); default: just that cell. Return an empty
    // list to refuse. Example: placing a player also turns the previous player cell into floor.
    virtual std::vector<CellChange> place(const TileDocument& doc, int x, int y, int code) const {
        (void)doc;
        return {{x, y, code}};
    }
    // Placement that changes more than cells (properties, e.g. a start position): return the new
    // document (one undo step) or nullopt to use place().
    virtual std::optional<TileDocument> place_document(const TileDocument& doc, int x, int y, int code) const {
        (void)doc, (void)x, (void)y, (void)code;
        return std::nullopt;
    }
    // Whether dragging with the left button keeps placing `code` (false: on click only).
    virtual bool paints_on_drag(int code) const {
        (void)code;
        return true;
    }
    // Undo label for placing `code`; empty = "Place <palette name>".
    virtual std::string place_label(int code) const {
        (void)code;
        return {};
    }
    // Called after a placement changed the document (status messages...).
    virtual void on_placed(TileEditor& editor, int x, int y, int code) { (void)editor, (void)x, (void)y, (void)code; }
    // Problems shown in the side panel and blocking test-play (e.g. "No exit").
    virtual std::vector<std::string> validate(const TileDocument& doc) const {
        (void)doc;
        return {};
    }
    // Errors and warnings; default: validate() as errors. The editor caches the result per document.
    virtual ValidationResult check(const TileDocument& doc) const { return {validate(doc), {}}; }
    // true: F5 refuses while there are errors; false: it asks "Test-play anyway?".
    virtual bool errors_block_test_play() const { return true; }
    // true: saving a document with errors reports them (status + a popup); saving is never blocked.
    virtual bool report_errors_on_save() const { return false; }
    // Draws ImGui widgets for the document properties (size, time limit, number...) editing
    // `doc` (a copy). Return true when something changed: the editor records an undo step and
    // adopts the copy. Use doc.grid.resize() for size changes.
    virtual bool properties_ui(TileDocument& doc) {
        (void)doc;
        return false;
    }
    // The whole Properties section body; default: properties_ui() on a copy, adopted as one undo
    // step ("Properties", or "Resize" when the size changed: the field is then fitted again).
    // Override to name undo steps yourself (editor.apply_document(after, label)).
    virtual void draw_properties(TileEditor& editor);

    // --- extra UI (all optional) ---
    // Extra sections of the side panel (use ImGui::CollapsingHeader).
    virtual void draw_panels(TileEditor& editor, SidePanelSlot slot) { (void)editor, (void)slot; }
    // The Check section body; return true if drawn (default: errors red, warnings yellow).
    virtual bool draw_check(TileEditor& editor, const ValidationResult& result) {
        (void)editor, (void)result;
        return false;
    }
    // Extra items at the top of the Level menu, and extra top-level menus after it.
    virtual void level_menu_items(TileEditor& editor) { (void)editor; }
    virtual void draw_menus(TileEditor& editor) { (void)editor; }
    // Extra keys (no Ctrl), checked after Tab and before the arrows when no popup or text field
    // has the keyboard.
    virtual void handle_keys(TileEditor& editor) { (void)editor; }
    // true: Home fits the field to the window, G toggles the grid lines (shown in the Level menu).
    virtual bool view_shortcuts() const { return false; }
    // The status bar line; return true if drawn.
    virtual bool draw_status_bar(TileEditor& editor) {
        (void)editor;
        return false;
    }
    // Width of the side panel for a content area `avail_width` wide.
    virtual float side_panel_width(float avail_width) const { return avail_width * 0.4f < 320.0f ? avail_width * 0.4f : 320.0f; }

    // --- cursor ---
    // true: the arrows wrap around the field edges (as in the original editors); false: clamp.
    virtual bool wrap_cursor() const { return false; }
    // Cursor position after New (`opened` = false) or Open (true).
    virtual GridPos initial_cursor(const TileDocument& doc, bool opened) const {
        (void)doc, (void)opened;
        return {};
    }

    // --- files ---
    virtual TileDocument new_document() const = 0;
    virtual std::vector<FileFilter> file_filters() const { return {{"Level (JSON)", "json"}}; }
    virtual std::string default_file_name() const { return "level.json"; }
    // Save As suggestion for a new document in `dir`; default: default_file_name().
    virtual std::string suggest_file_name(const std::filesystem::path& dir) const {
        (void)dir;
        return default_file_name();
    }
    // Where Open / Save As start before any file was opened; empty = the dialog's default.
    virtual std::filesystem::path default_directory() const { return {}; }
    // Throw vs::Error with a readable message on failure.
    virtual TileDocument load(const std::filesystem::path& path) const = 0;
    virtual void save(const std::filesystem::path& path, const TileDocument& doc) const = 0;
    virtual std::string new_document_status() const { return "New document"; }
    virtual std::string unsaved_changes_question() const { return "The document has unsaved changes. Save?"; }

    // F5: the editor saves the current document with save() to test_play_file() and passes its
    // path (e.g. launch_process({game_exe, "--test-level", path})). Return false (or throw
    // vs::Error with the reason) if the game cannot be started. Default: unsupported.
    virtual bool test_play(const std::filesystem::path& level_file) {
        (void)level_file;
        return false;
    }
    // Temporary file for F5; empty = user_data_dir(game_id) / "testplay_<default_file_name>".
    virtual std::filesystem::path test_play_file() const { return {}; }
};

// Map editor UI (Dear ImGui) driven by a plugin. Layout: menu bar, the field on the left,
// palette + properties + problems on the right, status bar at the bottom.
// Keyboard: Tab switches the active panel (field / palette); arrows move the frame cursor in
// it; Space/Enter places the selected tile (in the palette: picks the tile and returns to the
// field); Backspace erases; Ctrl+Z / Ctrl+Y (Ctrl+Shift+Z) undo/redo; Ctrl+N/O/S new/open/save,
// Ctrl+Shift+S save as; F5 test-play; Esc quits (asking "Save? Yes/No/Cancel" if modified).
// Mouse: LMB places (drag paints, one undo step per stroke), RMB erases, wheel zooms, middle
// button pans; clicking a palette tile selects it.
class TileEditor {
public:
    TileEditor(TileEditorPlugin& plugin, SDL_Window* window);

    // Draw the whole UI; call between ImGuiLayer::new_frame() and render().
    void draw();

    void new_document();
    bool open(const std::filesystem::path& path);   // shows errors in a popup
    bool save_to(const std::filesystem::path& path);
    void save();         // Save As dialog when the document has no path yet
    void save_as();
    void test_play();

    // Asks to save if modified. Returns true if the program may quit right now; otherwise the
    // prompt is shown and quit_requested() becomes true once the user answers Yes/No.
    bool request_quit();
    bool quit_requested() const { return quit_now_; }

    bool modified() const { return !undo_.is_clean(); }
    TileDocument& document() { return doc_; }
    const TileDocument& document() const { return doc_; }
    UndoStack& undo_stack() { return undo_; }
    const std::filesystem::path& path() const { return path_; }
    int selected_tile() const { return selected_; }  // index into the palette
    int cursor_x() const { return cursor_x_; }
    int cursor_y() const { return cursor_y_; }

    // --- for plugin hooks ---
    TileEditorPlugin& plugin() { return plugin_; }
    const std::vector<TileInfo>& palette() const { return palette_; }
    int selected_code() const;
    void select_index(int index);              // clamped to the palette
    void select_code(int code);                // first palette entry with this code
    bool field_active() const { return panel_ == Panel::Field; }
    void set_cursor(int x, int y);             // clamped to the grid
    int hover_x() const { return hover_x_; }   // cell under the mouse, -1 if none
    int hover_y() const { return hover_y_; }
    // Replaces the document as one undo step (nothing happens when `after` equals it).
    void apply_document(TileDocument after, const std::string& label);
    // Places `code` at (x, y) through the plugin's rules (one undo step; `stroke` != 0 merges).
    void place_at(int x, int y, int code, int stroke = 0);
    const ValidationResult& check();           // plugin().check(document()), cached
    const std::string& status() const { return status_; }
    void set_status(std::string text) { status_ = std::move(text); }
    GridView& view() { return view_; }
    void fit_to_window() { fit_pending_ = true; }
    MessagePopup& message() { return message_; }
    ConfirmPrompt& prompt() { return prompt_; }

private:
    enum class Panel { Field, Palette };
    using After = AfterSave;

    void handle_keys();
    void draw_menu();
    void draw_field(ImVec2 size);
    void draw_palette(ImVec2 size);
    void draw_side_panel();
    void draw_default_check(const ValidationResult& r);
    void draw_tile(ImDrawList* draw, int code, ImVec2 min, ImVec2 max) const;
    void launch_test_play();
    SaveFlow::Hooks save_hooks();
    void run_after(After action);
    void ask_save_then(After action);
    void update_title();
    std::vector<int> palette_group_sizes() const;

    TileEditorPlugin& plugin_;
    SDL_Window* window_;
    std::vector<TileInfo> palette_;
    TileDocument doc_;
    UndoStack undo_;
    GridView view_;
    FileDialogs dialogs_;
    ConfirmPrompt prompt_;
    MessagePopup message_;
    std::filesystem::path path_;
    std::filesystem::path last_dir_;
    Panel panel_ = Panel::Field;
    int selected_ = 0;        // index into palette_
    int cursor_x_ = 0;
    int cursor_y_ = 0;
    int hover_x_ = -1;
    int hover_y_ = -1;
    int stroke_ = 0;          // current mouse stroke id (0 = none)
    int next_stroke_ = 1;
    SaveFlow flow_;  // after prompt_, dialogs_ and path_ (uses them)
    bool quit_now_ = false;
    bool fit_pending_ = true;
    std::string status_;
    std::string last_title_;
    TileDocument checked_doc_;
    ValidationResult check_;
    bool check_valid_ = false;
};

}  // namespace vs
