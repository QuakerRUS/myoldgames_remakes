// Painter: rectangular selection, clipboard, floating (moved / pasted) pixels and flips.
#include "painter.hpp"

namespace painter {

void Painter::select_all() {
    commit_floating();
    selection_ = view_rect();
    tool_ = Tool::Select;
}

void Painter::deselect() {
    commit_floating();
    selection_.reset();
}

void Painter::copy() {
    if (floating_) {
        clipboard_ = floating_->pixels;
    } else {
        const IRect r = selection_ ? intersect(*selection_, bounds(doc_.image)) : view_rect();
        if (r.empty()) return;
        clipboard_ = copy_region(doc_.image, r);
    }
    status_ = "Copied " + std::to_string(clipboard_.width) + "x" + std::to_string(clipboard_.height);
}

void Painter::cut() {
    if (floating_) {
        clipboard_ = floating_->pixels;
        floating_->pixels = vs::Image();  // the source area was already cleared when lifted
        floating_->label = "Cut";
        commit_floating();
        status_ = "Cut " + std::to_string(clipboard_.width) + "x" + std::to_string(clipboard_.height);
        return;
    }
    if (!selection_) return;
    copy();
    begin_edit();
    draw_rect(doc_.image, *selection_, {0, 0, 0, 0}, true, bounds(doc_.image));
    touch();
    end_edit("Cut");
}

void Painter::paste() {
    if (clipboard_.empty()) return;
    commit_floating();
    begin_edit();
    const IRect region = view_rect();
    int x = region.x, y = region.y;
    if (selection_) {
        x = selection_->x;
        y = selection_->y;
    }
    floating_ = Floating{clipboard_, x, y, "Paste"};
    floating_dirty_ = true;
    selection_ = IRect{x, y, clipboard_.width, clipboard_.height};
    tool_ = Tool::Select;
    status_ = "Pasted: drag to move, Enter to place, Esc to cancel";
}

void Painter::delete_selection() {
    if (floating_) {
        floating_->pixels = vs::Image();
        floating_->label = "Delete";
        commit_floating();
        return;
    }
    if (!selection_) return;
    begin_edit();
    draw_rect(doc_.image, *selection_, {0, 0, 0, 0}, true, bounds(doc_.image));
    touch();
    end_edit("Delete");
}

void Painter::lift_selection() {
    if (floating_ || !selection_) return;
    const IRect r = intersect(*selection_, bounds(doc_.image));
    if (r.empty()) return;
    begin_edit();
    floating_ = Floating{copy_region(doc_.image, r), r.x, r.y, "Move"};
    draw_rect(doc_.image, r, {0, 0, 0, 0}, true, bounds(doc_.image));
    touch();
    floating_dirty_ = true;
    selection_ = r;
}

void Painter::commit_floating() {
    if (!floating_) return;
    Floating f = std::move(*floating_);
    floating_.reset();
    floating_dirty_ = true;
    if (!f.pixels.empty()) {
        blit(doc_.image, f.pixels, f.x, f.y, bounds(doc_.image), true);
        touch();
        const IRect placed = intersect({f.x, f.y, f.pixels.width, f.pixels.height}, bounds(doc_.image));
        selection_ = placed.empty() ? std::nullopt : std::optional<IRect>(placed);
    } else {
        selection_.reset();
    }
    end_edit(f.label);
}

void Painter::cancel_floating() {
    if (!floating_) return;
    floating_.reset();
    floating_dirty_ = true;
    cancel_edit();
    selection_.reset();
    status_ = "Cancelled";
}

void Painter::nudge_floating(int dx, int dy) {
    if (!floating_) return;
    floating_->x += dx;
    floating_->y += dy;
    selection_ = IRect{floating_->x, floating_->y, floating_->pixels.width, floating_->pixels.height};
}

void Painter::flip(bool horizontal) {
    const char* label = horizontal ? "Flip horizontally" : "Flip vertically";
    if (floating_) {
        horizontal ? flip_horizontal(floating_->pixels, bounds(floating_->pixels)) : flip_vertical(floating_->pixels, bounds(floating_->pixels));
        floating_dirty_ = true;
        return;
    }
    const IRect r = selection_ ? intersect(*selection_, bounds(doc_.image)) : view_rect();
    if (r.empty()) return;
    begin_edit();
    horizontal ? flip_horizontal(doc_.image, r) : flip_vertical(doc_.image, r);
    touch();
    end_edit(label);
    status_ = std::string(label) + (selection_ ? " (selection)" : view_ == View::Frame ? " (frame)" : " (image)");
}

}  // namespace painter
