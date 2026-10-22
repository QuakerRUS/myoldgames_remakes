#include "piton_editor.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>

#include <imgui_stdlib.h>

#include <vs/editor/file_dialogs.hpp>
#include <vs/engine/error.hpp>
#include <vs/engine/storage.hpp>
#include <vs/imgui/imgui_layer.hpp>

namespace piton_editor {

using piton::Cell;
using piton::Dir;
using piton::kFieldHeight;
using piton::kFieldWidth;

namespace {

constexpr ImU32 kCursor = IM_COL32(255, 220, 64, 255);
constexpr ImU32 kCursorDim = IM_COL32(255, 220, 64, 110);
constexpr ImU32 kAxis = IM_COL32(80, 200, 255, 170);
constexpr float kIcon = 40.0f;  // palette icon size

char dirLetter(Dir d) {
    switch (d) {
    case Dir::Up: return 'u';
    case Dir::Right: return 'r';
    case Dir::Down: return 'd';
    case Dir::Left: return 'l';
    }
    return 'r';
}

const char* dirTitle(Dir d) {
    switch (d) {
    case Dir::Up: return "Up";
    case Dir::Right: return "Right";
    case Dir::Down: return "Down";
    case Dir::Left: return "Left";
    }
    return "Right";
}

// Same grass choice as the game (piton/game/src/field_renderer.cpp, seed 0): mostly
// grass_0..5, a few flowers (grass_6/7).
std::uint32_t hashCell(int x, int y, std::uint32_t seed) {
    std::uint32_t h = static_cast<std::uint32_t>(x) * 73856093u ^ static_cast<std::uint32_t>(y) * 19349663u ^ seed * 83492791u;
    h ^= h >> 13;
    h *= 0x5bd1e995u;
    h ^= h >> 15;
    return h;
}

int grassVariant(int x, int y) {
    const std::uint32_t h = hashCell(x, y, 0) % 100u;
    if (h < 3) return 6;
    if (h < 6) return 7;
    return static_cast<int>(hashCell(y, x, 7) % 6u);
}

const vs::FrameRect* needFrame(const vs::SpriteSheet& sheet, const std::string& name) {
    const vs::FrameRect* f = sheet.frame(name);
    if (!f) throw vs::Error("sprite frame missing: " + name);
    return f;
}

}  // namespace

PitonPlugin::PitonPlugin(std::filesystem::path default_dir) : default_dir_(std::move(default_dir)) {}

void PitonPlugin::load_resources(vs::Renderer& renderer) {
    tiles_ = vs::SpriteSheet::load(renderer, vs::asset_path("gfx/tiles.json"));
    snake_ = vs::SpriteSheet::load(renderer, vs::asset_path("gfx/snake.json"));
    for (int i = 0; i < 8; ++i) grass_[static_cast<std::size_t>(i)] = needFrame(tiles_, "grass_" + std::to_string(i));
    for (int i = 0; i < 16; ++i) wall_[static_cast<std::size_t>(i)] = needFrame(tiles_, "wall_" + std::to_string(i));
    shade_n_ = needFrame(tiles_, "shade_n");
    shade_w_ = needFrame(tiles_, "shade_w");
    shade_nw_ = needFrame(tiles_, "shade_nw");
    for (Dir d : piton::kAllDirs) {
        const std::size_t i = static_cast<std::size_t>(d);
        head_[i] = needFrame(snake_, std::string("head_") + dirLetter(d));
        tail_[i] = needFrame(snake_, std::string("tail_") + dirLetter(d));
    }
    body_h_ = needFrame(snake_, "body_h");
    body_v_ = needFrame(snake_, "body_v");
}

// ---------------------------------------------------------------- palette

std::vector<vs::TileInfo> PitonPlugin::palette() const {
    // Quick-select keys 1..6 = palette order (handle_keys).
    return {{kBrick, "Brick"},
            {kEmpty, "Empty"},
            {startCode(Dir::Up), "Snake start: Up"},
            {startCode(Dir::Right), "Snake start: Right"},
            {startCode(Dir::Down), "Snake start: Down"},
            {startCode(Dir::Left), "Snake start: Left"}};
}

vs::PaletteStyle PitonPlugin::palette_style() const {
    vs::PaletteStyle s;
    s.item_height = kIcon + 6;
    s.fill_width = true;
    s.selection_rounding = 4.0f;
    return s;
}

bool PitonPlugin::draw_palette_item(ImDrawList* draw, const vs::TileInfo& tile, ImVec2 p, ImVec2 q, bool) const {
    draw->AddRectFilled(p, q, IM_COL32(40, 44, 52, 255), 4.0f);
    draw_palette_icon(draw, tile.code, {p.x + 3, p.y + 3}, {p.x + 3 + kIcon, p.y + 3 + kIcon});
    const auto dir = startCodeDir(tile.code);
    const char* label = dir ? dirTitle(*dir) : tile.name.c_str();
    draw->AddText({p.x + kIcon + 10, p.y + 5}, IM_COL32(230, 230, 230, 255), label);
    if (dir) draw->AddText({p.x + kIcon + 10, p.y + 5 + ImGui::GetTextLineHeight()}, IM_COL32(160, 170, 180, 255), "start");
    return true;
}

void PitonPlugin::palette_tooltip(const vs::TileInfo& tile) const {
    const std::vector<vs::TileInfo> all = palette();
    int key = 0;
    for (std::size_t i = 0; i < all.size(); ++i)
        if (all[i].code == tile.code) key = static_cast<int>(i) + 1;
    ImGui::SetTooltip("%s  (key %d)", tile.name.c_str(), key);
}

void PitonPlugin::draw_palette_footer(vs::TileEditor&, const vs::TileInfo& selected) {
    ImGui::Text("Selected: %s", selected.name.c_str());
    ImGui::Separator();
}

// ---------------------------------------------------------------- drawing

void PitonPlugin::draw_image(ImDrawList* draw, const vs::SpriteSheet& sheet, const vs::FrameRect* f, ImVec2 a, ImVec2 b,
                             ImU32 tint) const {
    if (!f) return;
    const float tw = static_cast<float>(sheet.texture().width());
    const float th = static_cast<float>(sheet.texture().height());
    draw->AddImage(vs::ImGuiLayer::texture_id(sheet.texture()), a, b, {f->x / tw, f->y / th},
                   {(f->x + f->w) / tw, (f->y + f->h) / th}, tint);
}

void PitonPlugin::draw_palette_icon(ImDrawList* draw, int code, ImVec2 a, ImVec2 b) const {
    draw_image(draw, tiles_, grass_[0], a, b);
    if (code == kBrick) {
        draw_image(draw, tiles_, wall_[0], a, b);
    } else if (const auto d = startCodeDir(code)) {
        draw_image(draw, snake_, head_[static_cast<std::size_t>(*d)], a, b);
    }
}

void PitonPlugin::draw_start_snake(ImDrawList* draw, const vs::GridView::Frame& f, Cell head, Dir dir, ImU32 tint) const {
    const Dir back = piton::opposite(dir);
    const Cell body = piton::step(head, back);
    const Cell tail = piton::step(body, back);
    auto put = [&](Cell c, const vs::FrameRect* fr) {
        if (piton::insideField(c)) draw_image(draw, snake_, fr, f.cell_min(c.x, c.y), f.cell_max(c.x, c.y), tint);
    };
    put(tail, tail_[static_cast<std::size_t>(back)]);
    put(body, dir == Dir::Left || dir == Dir::Right ? body_h_ : body_v_);
    put(head, head_[static_cast<std::size_t>(dir)]);
}

// Grass + shading, or a brick (autotile mask 1=N, 2=E, 4=S, 8=W), as in the game.
bool PitonPlugin::draw_field_cell(const vs::CellContext& c) const {
    auto wall = [&c](int x, int y) { return c.at(x, y) == kBrick; };
    const int x = c.x, y = c.y;
    if (wall(x, y)) {
        const int m = (wall(x, y - 1) ? 1 : 0) | (wall(x + 1, y) ? 2 : 0) | (wall(x, y + 1) ? 4 : 0) | (wall(x - 1, y) ? 8 : 0);
        draw_image(c.draw, tiles_, wall_[static_cast<std::size_t>(m)], c.min, c.max);
        return true;
    }
    draw_image(c.draw, tiles_, grass_[static_cast<std::size_t>(grassVariant(x, y))], c.min, c.max);
    if (wall(x - 1, y)) draw_image(c.draw, tiles_, shade_w_, c.min, c.max);
    if (wall(x, y - 1)) draw_image(c.draw, tiles_, shade_n_, c.min, c.max);
    if (wall(x - 1, y - 1) && !wall(x - 1, y) && !wall(x, y - 1)) draw_image(c.draw, tiles_, shade_nw_, c.min, c.max);
    return true;
}

// Start snake, the cells it needs free ahead (white = free, red = blocked), symmetry axes.
void PitonPlugin::draw_field_overlay(vs::TileEditor& editor, const vs::GridView::Frame& f) {
    const vs::TileGrid& g = editor.document().grid;
    auto wall = [&g](int x, int y) { return g.in_bounds(x, y) && g.at(x, y) == kBrick; };
    const Cell head = docStart(editor.document());
    const Dir dir = docStartDir(editor.document());
    Cell ahead = head;
    for (int i = 0; i < piton::kMinFreeAhead; ++i) {
        ahead = piton::step(ahead, dir);
        if (!piton::insideField(ahead)) break;
        const ImVec2 a = f.cell_min(ahead.x, ahead.y), b = f.cell_max(ahead.x, ahead.y);
        const ImVec2 c{(a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f};
        f.draw->AddCircleFilled(c, std::max(f.cell * 0.12f, 1.5f), wall(ahead.x, ahead.y) ? IM_COL32(255, 70, 60, 230) : IM_COL32(255, 255, 255, 150));
    }
    draw_start_snake(f.draw, f, head, dir, IM_COL32_WHITE);

    if (mirrorsX(symmetry_)) {
        const float ax = f.origin.x + f.cell * (kFieldWidth / 2.0f);
        f.draw->AddLine({ax, f.origin.y}, {ax, f.origin.y + f.cell * kFieldHeight}, kAxis, 2.0f);
    }
    if (mirrorsY(symmetry_)) {
        const float ay = f.origin.y + f.cell * (kFieldHeight / 2.0f);
        f.draw->AddLine({f.origin.x, ay}, {f.origin.x + f.cell * kFieldWidth, ay}, kAxis, 2.0f);
    }
}

bool PitonPlugin::draw_hover(vs::TileEditor& editor, const vs::GridView::Frame& f, int x, int y) {
    if (const auto start_dir = startCodeDir(editor.selected_code())) {
        // Where the start would go (moved inwards near the edge, see withStart).
        draw_start_snake(f.draw, f, piton::Level::clampStart({x, y}, *start_dir), *start_dir, IM_COL32(255, 255, 255, 110));
    } else {
        for (Cell m : symmetricCells({x, y}, symmetry_))
            f.draw->AddRect(f.cell_min(m.x, m.y), f.cell_max(m.x, m.y), IM_COL32(255, 255, 255, 120));
    }
    return true;
}

bool PitonPlugin::draw_cursor(vs::TileEditor& editor, const vs::GridView::Frame& f, int x, int y, bool active) {
    const std::vector<Cell> cursors = startCodeDir(editor.selected_code()) ? std::vector<Cell>{{x, y}} : symmetricCells({x, y}, symmetry_);
    for (std::size_t i = 0; i < cursors.size(); ++i) {
        const ImU32 c = active && i == 0 ? kCursor : kCursorDim;
        f.draw->AddRect(f.cell_min(cursors[i].x, cursors[i].y), f.cell_max(cursors[i].x, cursors[i].y), c, 0, 0, i == 0 ? 3.0f : 2.0f);
    }
    return true;
}

// ---------------------------------------------------------------- rules

std::vector<vs::CellChange> PitonPlugin::place(const vs::TileDocument& doc, int x, int y, int code) const {
    return paintChanges(doc, {x, y}, code, symmetry_);
}

std::optional<vs::TileDocument> PitonPlugin::place_document(const vs::TileDocument& doc, int x, int y, int code) const {
    if (const auto d = startCodeDir(code)) return withStart(doc, {x, y}, *d);
    return std::nullopt;
}

std::string PitonPlugin::place_label(int code) const {
    if (const auto d = startCodeDir(code)) return std::string("Snake start ") + dirTitle(*d);
    return code == kBrick ? "Place Brick" : "Erase";
}

const piton::LevelCheck& PitonPlugin::level_check(const vs::TileDocument& doc) const {
    if (!check_valid_ || !(doc == checked_doc_)) {
        checked_doc_ = doc;
        check_ = piton::checkLevel(toLevel(doc));
        check_valid_ = true;
    }
    return check_;
}

vs::ValidationResult PitonPlugin::check(const vs::TileDocument& doc) const {
    const piton::LevelCheck& c = level_check(doc);
    return {c.errors, c.warnings};
}

namespace {
std::string startMovedStatus(Cell wanted, Cell head) {
    return "Start moved to " + std::to_string(head.x) + "," + std::to_string(head.y) + " (" + std::to_string(wanted.x) +
           "," + std::to_string(wanted.y) + " would put the snake outside the field)";
}
}  // namespace

void PitonPlugin::on_placed(vs::TileEditor& editor, int x, int y, int code) {
    if (!startCodeDir(code)) return;
    const Cell head = docStart(editor.document());
    if (head != Cell{x, y}) editor.set_status(startMovedStatus({x, y}, head));
}

void PitonPlugin::move_start(vs::TileEditor& editor, Cell head, Dir dir, const std::string& label) {
    const vs::TileDocument after = withStart(editor.document(), head, dir);
    const Cell placed = docStart(after);
    if (!(after == editor.document())) editor.apply_document(after, label);
    if (placed != head) editor.set_status(startMovedStatus(head, placed));
}

void PitonPlugin::rotate_start(vs::TileEditor& editor) {
    const vs::TileDocument& doc = editor.document();
    const Dir d = docStartDir(doc);
    const Dir next = d == Dir::Up ? Dir::Right : d == Dir::Right ? Dir::Down : d == Dir::Down ? Dir::Left : Dir::Up;
    move_start(editor, docStart(doc), next, std::string("Start direction ") + dirTitle(next));
}

// ---------------------------------------------------------------- panels

void PitonPlugin::draw_properties(vs::TileEditor& editor) {
    const vs::TileDocument& doc = editor.document();
    if (!name_active_) name_buf_ = docName(doc);
    ImGui::SetNextItemWidth(-60);
    ImGui::InputText("Name", &name_buf_);
    name_active_ = ImGui::IsItemActive();
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        vs::TileDocument copy = doc;
        setDocName(copy, name_buf_);
        editor.apply_document(std::move(copy), "Level name");
    }

    const Cell head = docStart(editor.document());
    const Dir dir = docStartDir(editor.document());
    ImGui::TextUnformatted("Start direction (R rotates):");
    for (Dir d : piton::kAllDirs) {
        if (d != Dir::Up) ImGui::SameLine();
        if (ImGui::RadioButton(dirTitle(d), d == dir) && d != dir)
            move_start(editor, head, d, std::string("Start direction ") + dirTitle(d));
    }
    int xy[2] = {head.x, head.y};
    ImGui::SetNextItemWidth(-60);
    if (ImGui::InputInt2("Head x,y", xy, ImGuiInputTextFlags_EnterReturnsTrue)) {
        move_start(editor, {xy[0], xy[1]}, dir, "Move start");
    }
}

void PitonPlugin::draw_tools(vs::TileEditor& editor) {
    if (!ImGui::CollapsingHeader("Tools", ImGuiTreeNodeFlags_DefaultOpen)) return;
    const vs::TileDocument& doc = editor.document();
    const float w = std::floor((ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) / 2.0f);
    if (ImGui::Button("Clear field", {w, 0})) editor.apply_document(cleared(doc), "Clear field");
    ImGui::SameLine();
    if (ImGui::Button("Fill border", {w, 0})) editor.apply_document(borderFilled(doc), "Fill border");
    if (ImGui::Button("Flip horizontally", {w, 0})) editor.apply_document(flippedX(doc), "Flip horizontally");
    ImGui::SameLine();
    if (ImGui::Button("Flip vertically", {w, 0})) editor.apply_document(flippedY(doc), "Flip vertically");
    if (ImGui::Button("Mirror left > right", {w, 0})) editor.apply_document(mirroredLeftToRight(doc), "Mirror left to right");
    ImGui::SameLine();
    if (ImGui::Button("Mirror top > bottom", {w, 0})) editor.apply_document(mirroredTopToBottom(doc), "Mirror top to bottom");
    ImGui::TextUnformatted("Symmetry painting:");
    const Symmetry modes[] = {Symmetry::None, Symmetry::Horizontal, Symmetry::Vertical, Symmetry::Both};
    for (int i = 0; i < 4; ++i) {
        if (i) ImGui::SameLine();
        if (ImGui::RadioButton(symmetryName(modes[i]), symmetry_ == modes[i])) symmetry_ = modes[i];
    }
}

void PitonPlugin::draw_panels(vs::TileEditor& editor, vs::SidePanelSlot slot) {
    if (slot == vs::SidePanelSlot::AfterProperties) draw_tools(editor);
}

bool PitonPlugin::draw_check(vs::TileEditor& editor, const vs::ValidationResult&) {
    const piton::LevelCheck& check = level_check(editor.document());
    ImGui::Text("Free cells: %d (min %d)", check.freeCells, piton::kMinFreeCells);
    ImGui::Text("Regions: %d   Longest dead end: %d (max %d)", check.regions, check.longestDeadEnd, piton::kMaxDeadEndLength);
    if (check.ok()) {
        ImGui::TextColored({0.4f, 0.9f, 0.4f, 1.0f}, "No problems");
        return true;
    }
    ImGui::PushTextWrapPos(0.0f);
    for (const std::string& p : check.errors) ImGui::TextColored({1.0f, 0.45f, 0.4f, 1.0f}, "- %s", p.c_str());
    for (const std::string& p : check.warnings) ImGui::TextColored({1.0f, 0.8f, 0.35f, 1.0f}, "- %s", p.c_str());
    ImGui::PopTextWrapPos();
    ImGui::TextDisabled(check.playable() ? "Warnings only: saving and F5 are allowed." :
                                           "Red: the level cannot be played; saving is allowed, F5 is not.");
    return true;
}

void PitonPlugin::level_menu_items(vs::TileEditor& editor) {
    if (ImGui::MenuItem("Rotate start direction", "R")) rotate_start(editor);
}

void PitonPlugin::draw_menus(vs::TileEditor& editor) {
    if (!ImGui::BeginMenu("Tools")) return;
    const vs::TileDocument& doc = editor.document();
    if (ImGui::MenuItem("Clear field")) editor.apply_document(cleared(doc), "Clear field");
    if (ImGui::MenuItem("Fill border")) editor.apply_document(borderFilled(doc), "Fill border");
    ImGui::Separator();
    if (ImGui::MenuItem("Flip horizontally")) editor.apply_document(flippedX(doc), "Flip horizontally");
    if (ImGui::MenuItem("Flip vertically")) editor.apply_document(flippedY(doc), "Flip vertically");
    if (ImGui::MenuItem("Mirror left half to right")) editor.apply_document(mirroredLeftToRight(doc), "Mirror left to right");
    if (ImGui::MenuItem("Mirror top half to bottom")) editor.apply_document(mirroredTopToBottom(doc), "Mirror top to bottom");
    ImGui::Separator();
    bool x = mirrorsX(symmetry_), y = mirrorsY(symmetry_);
    if (ImGui::MenuItem("Symmetry: horizontal", "H", &x)) symmetry_ = makeSymmetry(x, y);
    if (ImGui::MenuItem("Symmetry: vertical", "V", &y)) symmetry_ = makeSymmetry(x, y);
    ImGui::EndMenu();
}

void PitonPlugin::handle_keys(vs::TileEditor& editor) {
    auto pressed = [](ImGuiKey k) { return ImGui::IsKeyPressed(k, false); };
    if (pressed(ImGuiKey_H)) symmetry_ = makeSymmetry(!mirrorsX(symmetry_), mirrorsY(symmetry_));
    if (pressed(ImGuiKey_V)) symmetry_ = makeSymmetry(mirrorsX(symmetry_), !mirrorsY(symmetry_));
    if (pressed(ImGuiKey_R)) rotate_start(editor);
    const ImGuiKey digits[] = {ImGuiKey_1, ImGuiKey_2, ImGuiKey_3, ImGuiKey_4, ImGuiKey_5, ImGuiKey_6};
    for (int i = 0; i < static_cast<int>(editor.palette().size()) && i < 6; ++i)
        if (pressed(digits[i])) editor.select_index(i);
}

bool PitonPlugin::draw_status_bar(vs::TileEditor& editor) {
    const piton::LevelCheck& check = level_check(editor.document());
    const std::string problems = check.ok() ? "valid" : std::to_string(check.problems.size()) + " problem(s)";
    ImGui::Text("38x23   cursor %d,%d   symmetry: %s   %s   %s%s", editor.cursor_x(), editor.cursor_y(), symmetryName(symmetry_),
                problems.c_str(), editor.status().c_str(), editor.modified() ? "   (modified)" : "");
    return true;
}

vs::GridPos PitonPlugin::initial_cursor(const vs::TileDocument& doc, bool opened) const {
    const Cell c = opened ? docStart(doc) : kNewLevelStart;
    return {c.x, c.y};
}

// ---------------------------------------------------------------- test-play

std::filesystem::path PitonPlugin::test_play_file() const {
    return std::filesystem::temp_directory_path() / "piton_editor_testplay.json";
}

bool PitonPlugin::test_play(const std::filesystem::path& level_file) {
    const std::filesystem::path game = vs::exe_dir() / "piton.exe";
    std::error_code ec;
    if (!std::filesystem::exists(game, ec)) throw vs::Error("piton.exe was not found next to the editor:\n" + vs::path_to_utf8(game));
    return vs::launch_process({vs::path_to_utf8(game), "--test-level", vs::path_to_utf8(level_file)});
}

}  // namespace piton_editor
