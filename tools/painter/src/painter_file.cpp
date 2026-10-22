// Painter: new / open / save, the unsaved-changes prompt and the window title.
#include <SDL3/SDL.h>

#include <vs/engine/json_file.hpp>

#include "painter.hpp"

namespace painter {

namespace {

const std::vector<vs::FileFilter> kOpenFilters = {{"Images and sprite sheets", "png;json"},
                                                   {"Sprite sheet (JSON + PNG)", "json"},
                                                   {"PNG image", "png"}};

}  // namespace

void Painter::reset_view_state() {
    undo_.clear();
    edit_before_.reset();
    sheet_before_.reset();
    selection_.reset();
    floating_.reset();
    floating_dirty_ = true;
    drag_.reset();
    sel_frame_ = doc_.sheet && !doc_.sheet->data.frames.empty() ? 0 : -1;
    sel_anim_ = doc_.sheet && !doc_.sheet->data.animations.empty() ? 0 : -1;
    frame_name_for_ = anim_name_for_ = -2;
    view_ = View::Sheet;
    fit_pending_ = true;
    cursor_x_ = cursor_y_ = 0;
    preview_ms_ = 0;
    if (doc_.sheet) grid_ = guess_tile_grid(*doc_.sheet, grid_);
    palette_ = palette_from_image(doc_.image);
    palette_source_ = "image";
    palette_sel_ = 0;
    ++doc_.revision;
}

void Painter::new_document(int w, int h, bool sheet) {
    doc_ = Document{};
    doc_.image = vs::Image(std::max(w, 1), std::max(h, 1));
    if (sheet) {
        doc_.sheet = Sheet{};
        doc_.sheet->data.image = "untitled.png";
    }
    reset_view_state();
    if (palette_.size() <= 1) {  // an empty image: start with a small default palette
        palette_ = {{0, 0, 0, 0},       {0, 0, 0, 255},      {255, 255, 255, 255}, {128, 128, 128, 255},
                    {190, 38, 51, 255}, {224, 111, 139, 255}, {73, 60, 43, 255},    {164, 100, 34, 255},
                    {235, 137, 49, 255}, {247, 226, 107, 255}, {47, 72, 78, 255},   {68, 137, 26, 255},
                    {163, 206, 39, 255}, {27, 38, 50, 255},   {0, 87, 132, 255},    {49, 162, 242, 255},
                    {178, 220, 239, 255}};
        palette_source_ = "default";
    }
    status_ = "New " + std::string(sheet ? "sprite sheet " : "image ") + std::to_string(w) + "x" + std::to_string(h);
}

bool Painter::open(const std::filesystem::path& path) {
    try {
        Document loaded = load_document(path);
        doc_ = std::move(loaded);
    } catch (const std::exception& e) {
        message_.title = "Open failed";
        message_.open(e.what());
        return false;
    }
    reset_view_state();
    last_dir_ = path.parent_path();
    status_ = "Opened " + vs::path_to_utf8(path.filename());
    return true;
}

bool Painter::save_to(const std::filesystem::path& path) {
    commit_floating();
    try {
        save_document(doc_, path);
    } catch (const std::exception& e) {
        flow_.cancel_pending();
        message_.title = "Save failed";
        message_.open(e.what());
        return false;
    }
    last_dir_ = doc_.path.parent_path();
    undo_.set_clean();
    status_ = "Saved " + vs::path_to_utf8(doc_.path.filename());
    if (doc_.sheet) {
        const std::vector<std::string> problems = validate_sheet(*doc_.sheet, doc_.image.width, doc_.image.height);
        if (!problems.empty()) {
            std::string text = "Saved, but the game cannot load this sheet until these are fixed:";
            for (std::size_t i = 0; i < problems.size() && i < 12; ++i) text += "\n - " + problems[i];
            message_.title = "Sheet check";
            message_.open(text);
        }
    }
    return true;
}

vs::SaveFlow::Hooks Painter::save_hooks() {
    vs::SaveFlow::Hooks h;
    h.modified = [this] { return modified(); };
    h.path = [this] { return doc_.path; };
    h.choose_path = [this](vs::SaveFlow::PathCallback done) {
        std::vector<vs::FileFilter> filters;
        if (doc_.sheet)
            filters = {{"Sprite sheet (JSON + PNG)", "json"}, {"Sprite sheet (PNG + JSON)", "png"}};
        else
            filters = {{"PNG image", "png"}};
        const std::filesystem::path suggestion =
            doc_.path.empty() ? last_dir_ / (doc_.sheet ? "untitled.json" : "untitled.png") : doc_.path;
        dialogs_.save_file(window_, filters, suggestion, std::move(done));
    };
    h.confirm_write = [this](const std::filesystem::path& target, std::function<void(bool)> proceed) {
        confirm_write(target, std::move(proceed));
    };
    h.write = [this](const std::filesystem::path& p) { return save_to(p); };
    h.run = [this](After a) { run_after(a); };
    h.question = [this] {
        const std::string name = doc_.path.empty() ? std::string("Untitled") : vs::path_to_utf8(doc_.path.filename());
        return "\"" + name + "\" has unsaved changes. Save them?";
    };
    return h;
}

// A sheet is two files: saving hero.png also writes hero.json (and level.json writes
// level.png). The save dialog only confirms the chosen file, so ask before replacing a
// companion that belongs to another document.
void Painter::confirm_write(const std::filesystem::path& target, std::function<void(bool)> proceed) {
    std::filesystem::path other;
    try {
        other = foreign_companion(doc_, target);
    } catch (const std::exception&) {
        other.clear();  // e.g. an unreadable folder: the save itself reports the problem
    }
    if (other.empty()) {
        proceed(true);
        return;
    }
    prompt_.title = "Replace file?";
    prompt_.open("Saving \"" + vs::path_to_utf8(target.filename()) + "\" also writes \"" + vs::path_to_utf8(other.filename()) +
                     "\",\nwhich already exists and is not part of this document.\n\nReplace \"" +
                     vs::path_to_utf8(other.filename()) + "\"?",
                 [proceed](vs::PromptChoice c) { proceed(c == vs::PromptChoice::Yes); }, false);
}

void Painter::save() {
    commit_floating();
    flow_.save();
}

void Painter::save_as() {
    commit_floating();
    flow_.save_as();
}

void Painter::run_after(After action) {
    switch (action) {
    case After::New: open_new_popup_ = true; break;
    case After::Open:
        dialogs_.open_file(window_, kOpenFilters, last_dir_, [this](const std::filesystem::path& chosen) {
            if (!chosen.empty()) open(chosen);
        });
        break;
    case After::Quit: quit_now_ = true; break;
    case After::Nothing: break;
    }
}

void Painter::ask_save_then(After action) {
    commit_floating();
    flow_.ask_save_then(action);
}

bool Painter::request_quit() {
    if (!modified() && !floating_) return true;
    ask_save_then(After::Quit);
    return quit_now_;
}

void Painter::update_title() {
    std::string title = doc_.path.empty() ? std::string("Untitled") : vs::path_to_utf8(doc_.path.filename());
    if (modified() || floating_) title += " *";
    title += " \xE2\x80\x94 Painter \xE2\x80\x94 V Software";
    if (title != last_title_) {
        last_title_ = title;
        SDL_SetWindowTitle(window_, title.c_str());
    }
}

}  // namespace painter
