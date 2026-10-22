// Painter: --selftest. Scripted edits through the editor's own code paths, then undo/redo and
// save/reload checks (run with the offscreen video driver; no user interaction).
#include <vs/engine/json_file.hpp>

#include "painter.hpp"

namespace painter {

namespace {

bool same_state(const Document& a, const vs::Image& image, const std::optional<Sheet>& sheet, std::string& why) {
    if (a.image.width != image.width || a.image.height != image.height || a.image.pixels != image.pixels) {
        why = "pixels differ";
        return false;
    }
    if (a.sheet.has_value() != sheet.has_value()) {
        why = "sheet presence differs";
        return false;
    }
    if (sheet && !sheets_equivalent(*a.sheet, *sheet, &why)) return false;
    return true;
}

}  // namespace

bool Painter::run_self_test(const std::filesystem::path& scratch_dir, std::string& log) {
    auto fail = [&](const std::string& what) {
        log += "SELFTEST FAIL: " + what + "\n";
        return false;
    };
    auto step = [&](const std::string& what) { log += "  ok  " + what + "\n"; };
    if (!doc_.sheet) return fail("open a sprite sheet first (--open <sheet.json>)");
    log += "Painter self-test on " + vs::path_to_utf8(doc_.path.filename()) + "\n";

    const vs::Image image0 = doc_.image;
    const std::optional<Sheet> sheet0 = doc_.sheet;
    const vs::Color red{220, 30, 40, 255};
    if (!undo_.is_clean()) return fail("document not clean after opening");

    // Keyboard mode: Space/Enter paint, Backspace paints the background, F1 fills.
    fg_ = red;
    bg_ = {0, 0, 0, 0};
    cursor_x_ = 3;
    cursor_y_ = 3;
    paint_at_cursor(fg_, "Paint");
    if (doc_.image.get(3, 3) != red) return fail("keyboard paint");
    paint_at_cursor(bg_, "Paint background");
    if (doc_.image.get(3, 3).a != 0) return fail("keyboard background paint");
    cursor_x_ = 0;
    cursor_y_ = 0;
    fill_at_cursor();
    if (doc_.image.get(0, 0) != red) return fail("F1 fill");
    step("keyboard paint / background / fill");

    // Mouse tools (the same calls the canvas makes on press / move / release).
    auto stroke = [&](Tool t, bool right, int x0, int y0, int x1, int y1) {
        begin_drag(t, right, x0, y0);
        continue_drag(x1, y1);
        finish_drag(x1, y1);
        drag_.reset();
    };
    stroke(Tool::Pencil, false, 40, 2, 50, 8);
    if (doc_.image.get(50, 8) != red) return fail("pencil stroke");
    stroke(Tool::Line, false, 70, 1, 90, 20);
    if (doc_.image.get(90, 20) != red) return fail("line");
    stroke(Tool::FilledRect, false, 100, 2, 110, 12);
    if (doc_.image.get(105, 7) != red) return fail("filled rectangle");
    stroke(Tool::Rect, false, 120, 2, 130, 12);
    if (doc_.image.get(120, 7) != red) return fail("rectangle");
    stroke(Tool::Eraser, false, 100, 2, 110, 2);
    if (doc_.image.get(105, 2).a != 0) return fail("eraser");
    begin_drag(Tool::Picker, true, 105, 7);
    drag_.reset();
    if (bg_ != red) return fail("picker (right button -> background)");
    bg_ = {0, 0, 0, 0};
    stroke(Tool::Fill, false, 200, 100, 200, 100);
    step("pencil, line, rectangles, eraser, picker, fill");

    // Selection: select, copy, paste + move, flip, cut, delete.
    stroke(Tool::Select, false, 0, 0, 31, 31);
    if (!selection_ || selection_->w != 32) return fail("select rectangle");
    copy();
    paste();
    if (!floating_) return fail("paste creates floating pixels");
    nudge_floating(160, 0);
    commit_floating();
    for (int y = 0; y < clipboard_.height; ++y)
        for (int x = 0; x < clipboard_.width; ++x)
            if (clipboard_.get(x, y).a != 0 && doc_.image.get(160 + x, y) != clipboard_.get(x, y)) return fail("pasted pixels");
    flip(true);
    flip(false);
    stroke(Tool::Select, false, 40, 40, 60, 60);
    stroke(Tool::Select, false, 45, 45, 80, 45);  // drag inside: lift and move
    if (!floating_ || floating_->x != 75) return fail("drag inside the selection lifts and moves it");
    deselect();  // places the moved pixels
    stroke(Tool::Select, false, 0, 100, 20, 120);
    cut();
    delete_selection();
    deselect();
    step("select, copy, paste, move, flips, cut, delete");

    // Frame flip in Frame view.
    select_frame(1);
    view_ = View::Frame;
    flip(true);
    view_ = View::Sheet;
    step("flip frame");

    // Sheet edits.
    const std::size_t frames_before = doc_.sheet->data.frames.size();
    select_frame(0);
    add_frame();
    duplicate_frame();
    if (doc_.sheet->data.frames.size() != frames_before + 2) return fail("add / duplicate frame");
    {
        Sheet s = *doc_.sheet;
        if (!rename_frame(s, 0, "renamed_frame")) return fail("rename frame");
        push_sheet(std::move(s), "Rename frame");
    }
    move_frame(1);
    remove_frame();
    tool_ = Tool::FrameRect;
    stroke(Tool::FrameRect, false, 1, 1, 40, 20);
    stroke(Tool::Pivot, false, 10, 10, 12, 30);
    add_animation();
    {
        Sheet s = *doc_.sheet;
        s.data.animations.back().loop = false;
        s.data.animations.back().frames.push_back({s.data.frames[2].name, 77});
        push_sheet(std::move(s), "Animation edit");
    }
    remove_animation();
    push_image(resized_canvas(doc_.image, doc_.image.width + 10, doc_.image.height + 33), doc_.sheet, "Canvas size");
    tool_ = Tool::Pencil;
    if (const std::vector<std::string> p = validate_sheet(*doc_.sheet, doc_.image.width, doc_.image.height); !p.empty())
        return fail("sheet invalid after edits: " + p.front());
    step("add/duplicate/rename/reorder/delete frames, frame rect, pivot, animations, canvas size");

    const vs::Image image1 = doc_.image;
    const std::optional<Sheet> sheet1 = doc_.sheet;
    const std::size_t steps = undo_.position();
    std::string why;
    if (!modified()) return fail("document should be modified");

    while (undo_.can_undo()) undo();
    if (!same_state(doc_, image0, sheet0, why)) return fail("undo all did not restore the original: " + why);
    if (!undo_.is_clean()) return fail("undo all should make the document clean");
    step("undo of all " + std::to_string(steps) + " steps restores the original exactly");
    while (undo_.can_redo()) redo();
    if (!same_state(doc_, image1, sheet1, why)) return fail("redo all did not restore the edited state: " + why);
    step("redo of all steps restores the edited state");

    // Save + reload of the edited document.
    std::error_code ec;
    std::filesystem::create_directories(scratch_dir, ec);
    const std::filesystem::path target = scratch_dir / "selftest_sheet.json";
    if (!save_to(target)) return fail("save");
    if (modified()) return fail("saved document should be clean");
    try {
        const Document back = load_document(target);
        if (!same_state(back, image1, sheet1.has_value() ? std::optional<Sheet>([&] {
                Sheet s = *sheet1;
                s.data.image = "selftest_sheet.png";
                return s;
            }())
                                                         : std::nullopt,
                        why))
            return fail("reloaded document differs: " + why);
    } catch (const std::exception& e) {
        return fail(std::string("reload: ") + e.what());
    }
    step("save + reload keeps the edited pixels and sheet (" + vs::path_to_utf8(target) + ")");
    log += "SELFTEST PASSED\n";
    return true;
}

}  // namespace painter
