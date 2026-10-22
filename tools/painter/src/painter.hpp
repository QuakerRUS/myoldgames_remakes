#pragma once
// Painter: the shared pixel-art / sprite-sheet editor (ImGui UI over a Document).
// The class is split over several files by area: painter.cpp (frame, textures, title),
// painter_file.cpp, painter_edit.cpp, painter_selection.cpp, painter_tools.cpp,
// painter_canvas.cpp, painter_keys.cpp, painter_menu.cpp, painter_frames.cpp,
// painter_animations.cpp, painter_colors.cpp, painter_dialogs.cpp.
#include <deque>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include <imgui.h>

#include <vs/editor/confirm_prompt.hpp>
#include <vs/editor/file_dialogs.hpp>
#include <vs/editor/grid_view.hpp>
#include <vs/editor/save_flow.hpp>
#include <vs/engine/texture.hpp>
#include <vs/engine/undo_stack.hpp>

#include "document.hpp"
#include "palette.hpp"
#include "raster.hpp"
#include "sheet_ops.hpp"
#include "tools.hpp"

struct SDL_Window;
namespace vs {
class Renderer;
}

namespace painter {

class Painter {
public:
    Painter(vs::Renderer& renderer, SDL_Window* window);

    void draw();  // one ImGui frame of the whole UI
    bool open(const std::filesystem::path& path);
    // Selects a frame by name and shows it alone (command line --frame).
    void show_frame(const std::string& name);
    // Window close: true = may quit now; false = a "save?" prompt is shown first.
    bool request_quit();
    bool quit_requested() const { return quit_now_; }
    // --selftest: drives every kind of edit through this editor, then checks that undoing all
    // of them restores the document exactly, redoing restores the edited state, and that the
    // edited document survives save + reload. Returns false and fills `log` on a failure.
    bool run_self_test(const std::filesystem::path& scratch_dir, std::string& log);

private:
    enum class Focus { Canvas, Palette };
    enum class View { Sheet, Frame };
    using After = vs::AfterSave;
    struct Floating {
        vs::Image pixels;
        int x = 0, y = 0;
        std::string label;
    };
    struct Drag {               // a mouse action on the canvas in progress
        Tool tool = Tool::Pencil;
        bool right = false;     // started with the right button (background colour)
        int ax = 0, ay = 0;     // anchor pixel
        int lx = 0, ly = 0;     // last pixel
        int grab_dx = 0, grab_dy = 0;  // moving a floating selection: pointer - top-left
        bool moved = false;
        bool moving = false;    // Select: dragging floating pixels
    };

    // painter_file.cpp
    void new_document(int w, int h, bool sheet);
    void reset_view_state();
    vs::SaveFlow::Hooks save_hooks();
    void confirm_write(const std::filesystem::path& target, std::function<void(bool)> proceed);
    bool save_to(const std::filesystem::path& path);  // writes now (no prompts); false + message on failure
    void save();
    void save_as();
    void ask_save_then(After action);
    void run_after(After action);
    void update_title();
    bool modified() const { return !undo_.is_clean(); }

    // painter_edit.cpp: pixel edits are made directly on doc_.image between begin_edit() and
    // end_edit(), which records the changed rectangle as one undo step.
    void begin_edit();
    void end_edit(const std::string& label);
    void cancel_edit();
    void touch() { ++doc_.revision; }
    void push_sheet(Sheet after, const std::string& label);
    void push_image(vs::Image after, std::optional<Sheet> sheet_after, const std::string& label);
    void track_sheet_item(const char* label);  // after an ImGui widget editing doc_.sheet in place
    void undo();
    void redo();
    IRect view_rect() const;   // the shown part of the image (a frame in Frame view)
    IRect edit_clip() const;   // where tools may paint (view rect ∩ selection)
    vs::FrameRect* selected_frame();
    vs::AnimationDef* selected_animation();
    void use_color(vs::Color c);  // remember in the recent colours

    // painter_selection.cpp
    void select_all();
    void deselect();
    void copy();
    void cut();
    void paste();
    void delete_selection();
    void lift_selection();
    void commit_floating();
    void cancel_floating();
    void flip(bool horizontal);
    void nudge_floating(int dx, int dy);

    // painter_tools.cpp
    void handle_canvas_mouse(const vs::GridView::Frame& f, const IRect& region);
    void begin_drag(Tool tool, bool right, int px, int py);
    void continue_drag(int px, int py);
    void finish_drag(int px, int py);
    void pick_color(int px, int py, bool right);
    void paint_at_cursor(vs::Color c, const char* label);
    void fill_at_cursor();
    vs::Color tool_color(bool right) const { return right ? bg_ : fg_; }

    // painter_canvas.cpp
    void draw_canvas_panel();
    void draw_canvas_toolbar();
    void draw_canvas_overlays(const vs::GridView::Frame& f, const IRect& region);
    void draw_checkerboard(ImDrawList* draw, ImVec2 a, ImVec2 b, ImVec2 clip_min, ImVec2 clip_max) const;
    void zoom_step(int dir);
    void fit_view();
    vs::GridView& current_view() { return view_ == View::Frame ? frame_view_ : sheet_view_; }

    // painter_keys.cpp
    void handle_keys();

    // painter_menu.cpp
    void draw_menu();
    void draw_toolstrip();
    void draw_status_bar();
    void draw_help_popup();

    // painter_frames.cpp
    void draw_document_panel();
    void draw_frames_section();
    void draw_frame_properties();
    void add_frame();
    void duplicate_frame();
    void remove_frame();
    void move_frame(int dir);
    void select_frame(int index);
    void make_sheet();

    // painter_animations.cpp
    void draw_animations_section();
    void draw_animation_entries(vs::AnimationDef& anim);
    void draw_preview();
    void add_animation();
    void remove_animation();

    // painter_colors.cpp
    void draw_color_section();
    void draw_palette_section(float height);
    void load_palette_dialog();
    void save_palette_dialog();
    bool color_swatch(const char* id, vs::Color c, ImVec2 size, bool outlined);

    // painter_dialogs.cpp
    void draw_new_popup();
    void draw_resize_popup();

    void sync_textures();
    void draw_frame();
    void recover_from_out_of_memory();

    vs::Renderer& renderer_;
    SDL_Window* window_;
    Document doc_;
    // History: at most 400 steps and about 512 MB of stored pixels (oldest steps dropped).
    static constexpr std::size_t kUndoByteBudget = std::size_t{512} << 20;
    vs::UndoStack undo_{400, kUndoByteBudget};
    vs::FileDialogs dialogs_;
    vs::ConfirmPrompt prompt_;
    vs::MessagePopup message_;
    std::filesystem::path last_dir_;
    vs::SaveFlow flow_;  // after prompt_, dialogs_ and last_dir_ (uses them)
    bool quit_now_ = false;
    std::string last_title_;
    std::string status_;

    // Textures mirror doc_.image and the floating selection.
    vs::Texture image_tex_;
    int tex_revision_ = -1;
    vs::Texture floating_tex_;
    bool floating_dirty_ = false;

    // View.
    View view_ = View::Sheet;
    vs::GridView sheet_view_;
    vs::GridView frame_view_;
    bool fit_pending_ = true;
    ImVec2 canvas_size_{0, 0};
    bool pixel_grid_ = true;
    bool tile_grid_ = false;
    bool onion_skin_ = false;
    bool show_frames_ = true;
    bool light_checker_ = false;
    bool snap_to_grid_ = true;
    TileGrid grid_{32, 32, 0};

    // Tools and colours.
    Tool tool_ = Tool::Pencil;
    Focus focus_ = Focus::Canvas;
    vs::Color fg_{0, 0, 0, 255};
    vs::Color bg_{0, 0, 0, 0};
    std::deque<vs::Color> recent_;
    Palette palette_;
    int palette_sel_ = 0;
    int palette_cols_ = 8;
    std::string palette_source_ = "image";
    bool edit_bg_ = false;      // the colour picker edits the background colour
    std::string hex_buf_;
    bool hex_editing_ = false;
    int cursor_x_ = 0, cursor_y_ = 0;  // keyboard frame cursor (image pixels)
    std::optional<std::pair<int, int>> hover_;
    std::optional<Drag> drag_;
    bool space_panned_ = false;

    // Editing.
    std::optional<vs::Image> edit_before_;
    std::optional<Sheet> sheet_before_;
    std::optional<IRect> selection_;
    std::optional<Floating> floating_;
    vs::Image clipboard_;

    // Sheet panels.
    int sel_frame_ = -1;
    int sel_anim_ = -1;
    std::string frame_name_buf_;
    int frame_name_for_ = -2;
    bool frame_name_editing_ = false;
    std::string anim_name_buf_;
    int anim_name_for_ = -2;
    bool anim_name_editing_ = false;
    int scrolled_frame_ = -2;  // the frame list scrolls to a selection made elsewhere
    int scrolled_anim_ = -2;
    bool preview_playing_ = true;
    double preview_ms_ = 0;
    int preview_scale_ = 0;  // 0 = automatic

    // Popups.
    bool open_new_popup_ = false;
    bool open_resize_popup_ = false;
    bool open_help_popup_ = false;
    int new_w_ = 256, new_h_ = 256;
    bool new_sheet_ = true;
    int resize_w_ = 0, resize_h_ = 0;
};

}  // namespace painter
