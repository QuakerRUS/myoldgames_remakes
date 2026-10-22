#include <vs/editor/tile_editor.hpp>

#include <algorithm>
#include <cmath>
#include <memory>

#include <SDL3/SDL.h>

#include <vs/engine/error.hpp>
#include <vs/engine/json_file.hpp>
#include <vs/engine/sprite_sheet.hpp>
#include <vs/engine/storage.hpp>
#include <vs/imgui/imgui_layer.hpp>

namespace vs {

namespace {

ImU32 to_imcol(Color c) { return IM_COL32(c.r, c.g, c.b, c.a); }

constexpr ImU32 kCursorActive = IM_COL32(255, 220, 64, 255);
constexpr ImU32 kCursorInactive = IM_COL32(255, 220, 64, 110);

// First extension of the first filter ("json;txt" -> ".json"); empty for "*".
std::string default_extension(const std::vector<FileFilter>& filters) {
    if (filters.empty()) return {};
    std::string ext = filters.front().pattern.substr(0, filters.front().pattern.find(';'));
    if (ext.empty() || ext == "*") return {};
    return "." + ext;
}

}  // namespace

// ---------------------------------------------------------------- plugin defaults

void TileEditorPlugin::palette_tooltip(const TileInfo& tile) const { ImGui::SetTooltip("%d: %s", tile.code, tile.name.c_str()); }

void TileEditorPlugin::draw_palette_footer(TileEditor& editor, const TileInfo& selected) {
    (void)editor;
    ImGui::Text("Selected: %d %s", selected.code, selected.name.c_str());
    ImGui::Separator();
}

void TileEditorPlugin::draw_properties(TileEditor& editor) {
    TileDocument copy = editor.document();
    if (!properties_ui(copy)) return;
    const TileGrid& g = editor.document().grid;
    const bool resized = copy.grid.width() != g.width() || copy.grid.height() != g.height();
    editor.apply_document(std::move(copy), resized ? "Resize" : "Properties");
    if (resized) editor.fit_to_window();
}

// ---------------------------------------------------------------- editor

TileEditor::TileEditor(TileEditorPlugin& plugin, SDL_Window* window)
    : plugin_(plugin),
      window_(window),
      palette_(plugin.palette()),
      last_dir_(plugin.default_directory()),
      flow_(prompt_, save_hooks()) {
    selected_ = plugin.initial_palette_index();
    new_document();
}

int TileEditor::selected_code() const {
    if (palette_.empty()) return plugin_.erase_value();
    return palette_[static_cast<std::size_t>(std::clamp(selected_, 0, static_cast<int>(palette_.size()) - 1))].code;
}

void TileEditor::select_index(int index) {
    if (!palette_.empty()) selected_ = std::clamp(index, 0, static_cast<int>(palette_.size()) - 1);
}

void TileEditor::select_code(int code) {
    for (std::size_t i = 0; i < palette_.size(); ++i)
        if (palette_[i].code == code) {
            selected_ = static_cast<int>(i);
            return;
        }
}

void TileEditor::set_cursor(int x, int y) {
    cursor_x_ = std::clamp(x, 0, std::max(doc_.grid.width() - 1, 0));
    cursor_y_ = std::clamp(y, 0, std::max(doc_.grid.height() - 1, 0));
}

const ValidationResult& TileEditor::check() {
    if (!check_valid_ || !(doc_ == checked_doc_)) {
        checked_doc_ = doc_;
        check_ = plugin_.check(doc_);
        check_valid_ = true;
    }
    return check_;
}

void TileEditor::new_document() {
    doc_ = plugin_.new_document();
    undo_.clear();
    path_.clear();
    const GridPos c = plugin_.initial_cursor(doc_, false);
    cursor_x_ = c.x;
    cursor_y_ = c.y;
    fit_pending_ = true;
    status_ = plugin_.new_document_status();
}

bool TileEditor::open(const std::filesystem::path& path) {
    try {
        TileDocument loaded = plugin_.load(path);
        doc_ = std::move(loaded);
    } catch (const std::exception& e) {
        message_.title = "Open failed";
        message_.open(e.what());
        return false;
    }
    undo_.clear();
    path_ = path;
    last_dir_ = path.parent_path();
    const GridPos c = plugin_.initial_cursor(doc_, true);
    cursor_x_ = c.x;
    cursor_y_ = c.y;
    fit_pending_ = true;
    status_ = "Opened " + path_to_utf8(path.filename());
    return true;
}

bool TileEditor::save_to(const std::filesystem::path& path) {
    try {
        plugin_.save(path, doc_);
    } catch (const std::exception& e) {
        flow_.cancel_pending();
        message_.title = "Save failed";
        message_.open(e.what());
        return false;
    }
    path_ = path;
    last_dir_ = path.parent_path();
    undo_.set_clean();
    status_ = "Saved " + path_to_utf8(path.filename());
    if (plugin_.report_errors_on_save()) {
        const ValidationResult& r = check();
        if (!r.ok()) {
            status_ += " (with " + std::to_string(r.errors.size()) + " problem(s))";
            if (flow_.pending() == After::Nothing) {
                std::string text = "Saved, but the level has problems:";
                for (const std::string& p : r.errors) text += "\n - " + p;
                message_.title = "Saved with warnings";
                message_.open(text);
            }
        }
    }
    return true;
}

SaveFlow::Hooks TileEditor::save_hooks() {
    SaveFlow::Hooks h;
    h.modified = [this] { return modified(); };
    h.path = [this] { return path_; };
    h.choose_path = [this](SaveFlow::PathCallback done) {
        const std::vector<FileFilter> filters = plugin_.file_filters();
        const std::filesystem::path suggestion =
            path_.empty() ? last_dir_ / utf8_to_path(plugin_.suggest_file_name(last_dir_)) : path_;
        dialogs_.save_file(window_, filters, suggestion, [filters, done](const std::filesystem::path& chosen) {
            std::filesystem::path p = chosen;
            if (!p.empty() && !p.has_extension()) p += default_extension(filters);
            done(p);
        });
    };
    h.write = [this](const std::filesystem::path& p) { return save_to(p); };
    h.run = [this](AfterSave a) { run_after(a); };
    h.question = [this] { return plugin_.unsaved_changes_question(); };
    return h;
}

void TileEditor::save() { flow_.save(); }

void TileEditor::save_as() { flow_.save_as(); }

void TileEditor::run_after(After action) {
    switch (action) {
    case After::New:
        new_document();
        break;
    case After::Open:
        dialogs_.open_file(window_, plugin_.file_filters(), last_dir_, [this](const std::filesystem::path& chosen) {
            if (!chosen.empty()) open(chosen);
        });
        break;
    case After::Quit:
        quit_now_ = true;
        break;
    case After::Nothing:
        break;
    }
}

void TileEditor::ask_save_then(After action) { flow_.ask_save_then(action); }

bool TileEditor::request_quit() { return flow_.request_quit(); }

void TileEditor::test_play() {
    const ValidationResult& r = check();
    if (r.ok()) {
        launch_test_play();
        return;
    }
    if (plugin_.errors_block_test_play()) {
        std::string text = "Cannot test-play:";
        for (const std::string& p : r.errors) text += "\n - " + p;
        message_.title = "Level check";
        message_.open(text);
        return;
    }
    std::string text = "The level has problems:";
    for (const std::string& p : r.errors) text += "\n - " + p;
    text += "\n\nTest-play anyway?";
    prompt_.title = "Level check";
    prompt_.open(
        text,
        [this](PromptChoice c) {
            if (c == PromptChoice::Yes) launch_test_play();
        },
        false);
}

void TileEditor::launch_test_play() {
    try {
        std::filesystem::path tmp = plugin_.test_play_file();
        if (tmp.empty()) tmp = user_data_dir(plugin_.game_id()) / utf8_to_path("testplay_" + plugin_.default_file_name());
        plugin_.save(tmp, doc_);
        if (!plugin_.test_play(tmp)) {
            message_.title = "Test-play";
            message_.open("Test-play could not be started.");
        } else {
            status_ = "Test-play started";
        }
    } catch (const std::exception& e) {
        message_.title = "Test-play";
        message_.open(e.what());
    }
}

void TileEditor::apply_document(TileDocument after, const std::string& label) {
    if (after == doc_) return;
    undo_.push(std::make_unique<DocumentCommand>(doc_, std::move(after), label));
}

void TileEditor::place_at(int x, int y, int code, int stroke) {
    if (!doc_.grid.in_bounds(x, y)) return;
    std::string label = plugin_.place_label(code);
    if (label.empty()) {
        label = "Place";
        for (const TileInfo& t : palette_)
            if (t.code == code) label = "Place " + t.name;
    }
    if (std::optional<TileDocument> after = plugin_.place_document(doc_, x, y, code)) {
        if (*after == doc_) return;
        apply_document(std::move(*after), label);
        plugin_.on_placed(*this, x, y, code);
        return;
    }
    std::vector<CellChange> changes = plugin_.place(doc_, x, y, code);
    std::erase_if(changes, [this](const CellChange& c) {
        return !doc_.grid.in_bounds(c.x, c.y) || doc_.grid.at(c.x, c.y) == c.value;
    });
    if (changes.empty()) return;
    undo_.push(std::make_unique<CellsCommand>(doc_, std::move(changes), label, stroke), stroke != 0);
    plugin_.on_placed(*this, x, y, code);
}

void TileEditor::update_title() {
    std::string title = plugin_.title() + " - " + (path_.empty() ? std::string("Untitled") : path_to_utf8(path_.filename()));
    if (modified()) title += " *";
    if (title != last_title_) {
        last_title_ = title;
        SDL_SetWindowTitle(window_, title.c_str());
    }
}

std::vector<int> TileEditor::palette_group_sizes() const {
    std::vector<int> sizes;
    for (std::size_t i = 0; i < palette_.size(); ++i) {
        if (i == 0 || palette_[i].group != palette_[i - 1].group)
            sizes.push_back(1);
        else
            ++sizes.back();
    }
    return sizes;
}

void TileEditor::handle_keys() {
    const ImGuiIO& io = ImGui::GetIO();
    if (io.WantTextInput || prompt_.is_open() || message_.is_open() || dialogs_.busy()) return;
    const bool ctrl = io.KeyCtrl;
    const bool shift = io.KeyShift;
    auto pressed = [](ImGuiKey k, bool repeat = false) { return ImGui::IsKeyPressed(k, repeat); };

    if (ctrl) {
        if (pressed(ImGuiKey_Z, true) && !shift) undo_.undo();
        if (pressed(ImGuiKey_Y, true) || (pressed(ImGuiKey_Z, true) && shift)) undo_.redo();
        if (pressed(ImGuiKey_N)) ask_save_then(After::New);
        if (pressed(ImGuiKey_O)) ask_save_then(After::Open);
        if (pressed(ImGuiKey_S)) shift ? save_as() : save();
        return;
    }
    if (pressed(ImGuiKey_F5)) test_play();
    if (pressed(ImGuiKey_Escape)) {
        if (request_quit()) quit_now_ = true;
        return;
    }
    if (pressed(ImGuiKey_Tab)) panel_ = panel_ == Panel::Field ? Panel::Palette : Panel::Field;
    if (plugin_.view_shortcuts()) {
        if (pressed(ImGuiKey_G)) view_.show_grid_lines = !view_.show_grid_lines;
        if (pressed(ImGuiKey_Home)) fit_pending_ = true;
    }
    plugin_.handle_keys(*this);

    int dx = 0, dy = 0;
    if (pressed(ImGuiKey_LeftArrow, true)) dx = -1;
    if (pressed(ImGuiKey_RightArrow, true)) dx = 1;
    if (pressed(ImGuiKey_UpArrow, true)) dy = -1;
    if (pressed(ImGuiKey_DownArrow, true)) dy = 1;
    const bool place = pressed(ImGuiKey_Space, true) || pressed(ImGuiKey_Enter, true) || pressed(ImGuiKey_KeypadEnter, true);
    const float cell = plugin_.cell_size();

    if (panel_ == Panel::Field) {
        if (dx || dy) {
            const int w = std::max(doc_.grid.width(), 1), h = std::max(doc_.grid.height(), 1);
            if (plugin_.wrap_cursor()) {
                cursor_x_ = ((cursor_x_ + dx) % w + w) % w;
                cursor_y_ = ((cursor_y_ + dy) % h + h) % h;
            } else {
                cursor_x_ = std::clamp(cursor_x_ + dx, 0, w - 1);
                cursor_y_ = std::clamp(cursor_y_ + dy, 0, h - 1);
            }
            view_.ensure_visible(cursor_x_, cursor_y_, cell);
        }
        if (place && !palette_.empty()) place_at(cursor_x_, cursor_y_, selected_code(), 0);
        if (pressed(ImGuiKey_Backspace, true) || pressed(ImGuiKey_Delete, true))
            place_at(cursor_x_, cursor_y_, plugin_.erase_value(), 0);
    } else if (!palette_.empty()) {
        if (dx || dy)
            selected_ = palette_move(palette_group_sizes(), std::max(plugin_.palette_columns(), 1), selected_, dx, dy,
                                     plugin_.palette_style().wrap_navigation);
        if (place) panel_ = Panel::Field;
    }
}

void TileEditor::draw_tile(ImDrawList* draw, int code, ImVec2 min, ImVec2 max) const {
    if (plugin_.draw_cell(draw, code, min, max)) return;
    const TileInfo* info = nullptr;
    for (const TileInfo& t : palette_)
        if (t.code == code) info = &t;
    const SpriteSheet* icons = plugin_.icons();
    if (info && icons && !info->icon_frame.empty()) {
        if (const FrameRect* f = icons->frame(info->icon_frame)) {
            const float tw = static_cast<float>(icons->texture().width());
            const float th = static_cast<float>(icons->texture().height());
            draw->AddImage(ImGuiLayer::texture_id(icons->texture()), min, max, {f->x / tw, f->y / th},
                           {(f->x + f->w) / tw, (f->y + f->h) / th});
            return;
        }
    }
    draw->AddRectFilled(min, max, to_imcol(info ? info->color : Color{255, 0, 255, 255}));
    if (max.x - min.x >= 20.0f) {
        const std::string label = std::to_string(code);
        draw->AddText({min.x + 2, min.y + 1}, IM_COL32(0, 0, 0, 200), label.c_str());
    }
}

void TileEditor::draw_menu() {
    if (!ImGui::BeginMenuBar()) return;
    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("New", "Ctrl+N")) ask_save_then(After::New);
        if (ImGui::MenuItem("Open...", "Ctrl+O")) ask_save_then(After::Open);
        if (ImGui::MenuItem("Save", "Ctrl+S")) save();
        if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S")) save_as();
        ImGui::Separator();
        if (ImGui::MenuItem("Exit", "Esc") && request_quit()) quit_now_ = true;
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Edit")) {
        const std::string u = "Undo " + undo_.undo_label();
        const std::string r = "Redo " + undo_.redo_label();
        if (ImGui::MenuItem(u.c_str(), "Ctrl+Z", false, undo_.can_undo())) undo_.undo();
        if (ImGui::MenuItem(r.c_str(), "Ctrl+Y", false, undo_.can_redo())) undo_.redo();
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Level")) {
        if (ImGui::MenuItem("Test play", "F5")) test_play();
        plugin_.level_menu_items(*this);
        ImGui::Separator();
        const bool keys = plugin_.view_shortcuts();
        if (ImGui::MenuItem("Fit to window", keys ? "Home" : nullptr)) fit_pending_ = true;
        ImGui::MenuItem("Grid lines", keys ? "G" : nullptr, &view_.show_grid_lines);
        ImGui::EndMenu();
    }
    plugin_.draw_menus(*this);
    ImGui::EndMenuBar();
}

void TileEditor::draw_field(ImVec2 size) {
    const float cell = plugin_.cell_size();
    const TileGrid& g = doc_.grid;
    if (fit_pending_ && view_.fit(g.width(), g.height(), cell)) fit_pending_ = false;
    const GridView::Frame f = view_.begin("##field", g.width(), g.height(), cell, size);

    // Visible cells (plus overdraw_rows() below: objects reaching upwards).
    const int x0 = std::max(0, static_cast<int>(std::floor((f.canvas_min.x - f.origin.x) / f.cell)));
    const int y0 = std::max(0, static_cast<int>(std::floor((f.canvas_min.y - f.origin.y) / f.cell)));
    const int x1 = std::min(g.width() - 1, static_cast<int>(std::ceil((f.canvas_max.x - f.origin.x) / f.cell)));
    const int y1 = std::min(g.height() - 1,
                            static_cast<int>(std::ceil((f.canvas_max.y - f.origin.y) / f.cell)) + std::max(plugin_.overdraw_rows(), 0));
    CellContext ctx;
    ctx.draw = f.draw;
    ctx.doc = &doc_;
    ctx.cell = f.cell;
    ctx.time = ImGui::GetTime();
    for (int y = y0; y <= y1; ++y)
        for (int x = x0; x <= x1; ++x) {
            ctx.x = x;
            ctx.y = y;
            ctx.code = g.at(x, y);
            ctx.min = f.cell_min(x, y);
            ctx.max = f.cell_max(x, y);
            if (!plugin_.draw_field_cell(ctx)) draw_tile(f.draw, ctx.code, ctx.min, ctx.max);
        }
    for (int y = y0; y <= y1; ++y)
        for (int x = x0; x <= x1; ++x) {
            ctx.x = x;
            ctx.y = y;
            ctx.code = g.at(x, y);
            ctx.min = f.cell_min(x, y);
            ctx.max = f.cell_max(x, y);
            plugin_.draw_cell_overlay(ctx);
        }
    plugin_.draw_field_overlay(*this, f);

    hover_x_ = f.hovered_cell ? f.cell_x : -1;
    hover_y_ = f.hovered_cell ? f.cell_y : -1;
    if (f.hovered_cell && !plugin_.draw_hover(*this, f, f.cell_x, f.cell_y))
        f.draw->AddRect(f.cell_min(f.cell_x, f.cell_y), f.cell_max(f.cell_x, f.cell_y), IM_COL32(255, 255, 255, 120));
    if (g.in_bounds(cursor_x_, cursor_y_) && !plugin_.draw_cursor(*this, f, cursor_x_, cursor_y_, panel_ == Panel::Field)) {
        const ImU32 c = panel_ == Panel::Field ? kCursorActive : kCursorInactive;
        f.draw->AddRect(f.cell_min(cursor_x_, cursor_y_), f.cell_max(cursor_x_, cursor_y_), c, 0, 0, 3.0f);
    }
    view_.end(f);

    // Mouse painting: LMB places, RMB erases; a stroke is one undo step. Codes that do not
    // paint on drag (paints_on_drag) are placed once, on the click.
    const int code = selected_code();
    const bool drag_paints = plugin_.paints_on_drag(code);
    const bool left = ImGui::IsMouseDown(ImGuiMouseButton_Left);
    const bool right = ImGui::IsMouseDown(ImGuiMouseButton_Right);
    const bool clicked_left = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    if (f.hovered_cell && (clicked_left || ImGui::IsMouseClicked(ImGuiMouseButton_Right))) {
        stroke_ = next_stroke_++;
        panel_ = Panel::Field;
        cursor_x_ = f.cell_x;
        cursor_y_ = f.cell_y;
        if (clicked_left && !drag_paints) place_at(f.cell_x, f.cell_y, code, 0);
    }
    if (stroke_ != 0 && f.hovered_cell && (left || right) && (right || drag_paints)) {
        place_at(f.cell_x, f.cell_y, right ? plugin_.erase_value() : code, stroke_);
        cursor_x_ = f.cell_x;
        cursor_y_ = f.cell_y;
    }
    if (stroke_ != 0 && !left && !right) {
        stroke_ = 0;
        undo_.break_merge();
    }
}

void TileEditor::draw_palette(ImVec2 size) {
    const PaletteStyle style = plugin_.palette_style();
    const int cols = std::max(plugin_.palette_columns(), 1);
    const float base_spacing = ImGui::GetStyle().ItemSpacing.x;
    const float spacing = base_spacing * style.spacing_scale;
    const float item_w = style.fill_width
                             ? std::floor((size.x - spacing * static_cast<float>(cols - 1)) / static_cast<float>(cols))
                             : std::floor((size.x - spacing * static_cast<float>(cols + 1)) / static_cast<float>(cols));
    const float item_h = style.item_height > 0 ? style.item_height : item_w;
    const bool custom_spacing = style.spacing_scale != 1.0f;
    ImGui::TextUnformatted(plugin_.palette_header(panel_ == Panel::Palette).c_str());
    if (custom_spacing) ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {spacing, spacing});
    int in_group = 0;
    for (int i = 0; i < static_cast<int>(palette_.size()); ++i) {
        const TileInfo& t = palette_[static_cast<std::size_t>(i)];
        if (i == 0 || t.group != palette_[static_cast<std::size_t>(i - 1)].group) {
            in_group = 0;
            if (!t.group.empty()) {
                if (custom_spacing) ImGui::PopStyleVar();
                ImGui::SeparatorText(t.group.c_str());
                if (custom_spacing) ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {spacing, spacing});
            }
        }
        if (in_group % cols != 0) ImGui::SameLine();
        ++in_group;
        ImGui::PushID(i);
        const ImVec2 p = ImGui::GetCursorScreenPos();
        if (ImGui::InvisibleButton("tile", {item_w, item_h})) {
            selected_ = i;
            if (style.pick_returns_to_field) panel_ = Panel::Field;
        }
        const bool hovered = ImGui::IsItemHovered();
        ImDrawList* draw = ImGui::GetWindowDrawList();
        const ImVec2 q{p.x + item_w, p.y + item_h};
        if (!plugin_.draw_palette_item(draw, t, p, q, hovered)) draw_tile(draw, t.code, {p.x + 3, p.y + 3}, {q.x - 3, q.y - 3});
        if (i == selected_) {
            const ImU32 c = panel_ == Panel::Palette ? kCursorActive : style.selection_inactive;
            draw->AddRect(p, q, c, style.selection_rounding, 0, 3.0f);
        }
        if (hovered) plugin_.palette_tooltip(t);
        ImGui::PopID();
    }
    if (custom_spacing) ImGui::PopStyleVar();
    if (!palette_.empty()) plugin_.draw_palette_footer(*this, palette_[static_cast<std::size_t>(selected_)]);
}

void TileEditor::draw_default_check(const ValidationResult& r) {
    if (r.errors.empty() && r.warnings.empty()) ImGui::TextColored({0.4f, 0.9f, 0.4f, 1.0f}, "No problems");
    for (const std::string& p : r.errors) ImGui::TextColored({1.0f, 0.45f, 0.4f, 1.0f}, "%s", p.c_str());
    for (const std::string& p : r.warnings) ImGui::TextColored({1.0f, 0.8f, 0.35f, 1.0f}, "%s", p.c_str());
}

void TileEditor::draw_side_panel() {
    draw_palette(ImGui::GetContentRegionAvail());
    if (ImGui::CollapsingHeader("Properties", ImGuiTreeNodeFlags_DefaultOpen)) plugin_.draw_properties(*this);
    plugin_.draw_panels(*this, SidePanelSlot::AfterProperties);
    if (ImGui::CollapsingHeader("Check", ImGuiTreeNodeFlags_DefaultOpen)) {
        const ValidationResult r = check();
        if (!plugin_.draw_check(*this, r)) draw_default_check(r);
    }
    plugin_.draw_panels(*this, SidePanelSlot::AfterCheck);
}

void TileEditor::draw() {
    dialogs_.poll();
    update_title();
    handle_keys();
    if (!palette_.empty()) selected_ = std::clamp(selected_, 0, static_cast<int>(palette_.size()) - 1);
    cursor_x_ = std::clamp(cursor_x_, 0, std::max(doc_.grid.width() - 1, 0));
    cursor_y_ = std::clamp(cursor_y_, 0, std::max(doc_.grid.height() - 1, 0));

    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_MenuBar |
                                   ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoSavedSettings;
    ImGui::Begin("##tile_editor", nullptr, flags);
    draw_menu();
    const ImVec2 avail = ImGui::GetContentRegionAvail();
    const float status_h = ImGui::GetFrameHeightWithSpacing();
    const float side_w = plugin_.side_panel_width(avail.x);
    const float field_w = avail.x - side_w - ImGui::GetStyle().ItemSpacing.x;
    const float h = avail.y - status_h;
    ImGui::BeginChild("field_panel", {field_w, h}, ImGuiChildFlags_Borders, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    draw_field({0, 0});
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild("side_panel", {side_w, h}, ImGuiChildFlags_Borders);
    draw_side_panel();
    ImGui::EndChild();
    if (!plugin_.draw_status_bar(*this))
        ImGui::Text("%dx%d   cursor %d,%d   %s%s", doc_.grid.width(), doc_.grid.height(), cursor_x_, cursor_y_, status_.c_str(),
                    modified() ? "   (modified)" : "");
    ImGui::End();

    prompt_.draw();
    message_.draw();
}

}  // namespace vs
