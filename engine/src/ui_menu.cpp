#include <vs/engine/ui_menu.hpp>

#include <algorithm>

namespace vs {

int Menu::add(std::string label, std::function<void()> on_select) {
    MenuItem item;
    item.label = std::move(label);
    item.on_select = std::move(on_select);
    items_.push_back(std::move(item));
    return count() - 1;
}

void Menu::set_selected(int index) {
    if (items_.empty()) return;
    selected_ = std::clamp(index, 0, count() - 1);
}

float Menu::item_height() const {
    return (theme_->font ? theme_->font->line_height() : 24.0f) + theme_->item_spacing;
}

RectF Menu::item_rect(int index) const {
    const Font* f = theme_->font;
    const MenuItem& it = items_[static_cast<std::size_t>(index)];
    const float w = (f ? f->measure(theme_->menu_marker + it.label) : 100.0f) + theme_->padding * 2;
    const float h = item_height();
    float x = anchor_.x - theme_->padding;
    if (align_ == Align::Center) x = anchor_.x - w * 0.5f;
    if (align_ == Align::Right) x = anchor_.x - w + theme_->padding;
    return {x, anchor_.y + h * static_cast<float>(index) - theme_->item_spacing * 0.5f, w, h};
}

void Menu::move(int dir) {
    if (items_.empty()) return;
    const int n = count();
    int i = selected_;
    for (int k = 0; k < n; ++k) {
        i = (i + dir + n) % n;
        if (items_[static_cast<std::size_t>(i)].enabled) break;
    }
    if (i != selected_) {
        selected_ = i;
        theme_->sound(UiSound::Move);
        if (on_change) on_change(selected_);
    }
}

void Menu::update(const Input& input) {
    if (items_.empty()) return;
    if (input.repeated(SDLK_UP)) move(-1);
    if (input.repeated(SDLK_DOWN)) move(+1);
    if (input.wheel() > 0) move(-1);
    if (input.wheel() < 0) move(+1);

    if (input.mouse_moved() || input.mouse_pressed(SDL_BUTTON_LEFT)) {
        for (int i = 0; i < count(); ++i) {
            if (items_[static_cast<std::size_t>(i)].enabled && item_rect(i).contains(input.mouse()) && i != selected_) {
                selected_ = i;
                theme_->sound(UiSound::Move);
                if (on_change) on_change(selected_);
            }
        }
    }

    MenuItem& cur = items_[static_cast<std::size_t>(selected_)];
    if (!cur.enabled) return;
    if (cur.on_adjust) {
        if (input.repeated(SDLK_LEFT)) {
            theme_->sound(UiSound::Move);
            cur.on_adjust(-1);
        }
        if (input.repeated(SDLK_RIGHT)) {
            theme_->sound(UiSound::Move);
            cur.on_adjust(+1);
        }
    }
    const bool click = input.mouse_pressed(SDL_BUTTON_LEFT) && item_rect(selected_).contains(input.mouse());
    if (input.pressed(SDLK_RETURN) || input.pressed(SDLK_KP_ENTER) || input.pressed(SDLK_SPACE) || click) {
        theme_->sound(UiSound::Select);
        if (cur.on_select)
            cur.on_select();
        else if (cur.on_adjust)
            cur.on_adjust(+1);
        return;
    }
    if (input.pressed(SDLK_ESCAPE) && on_cancel) {
        theme_->sound(UiSound::Cancel);
        on_cancel();
    }
}

void Menu::render(Renderer& renderer) const {
    const Font* f = theme_->font;
    if (!f) return;
    for (int i = 0; i < count(); ++i) {
        const MenuItem& it = items_[static_cast<std::size_t>(i)];
        const bool sel = i == selected_;
        if (sel && theme_->highlight_bg.a > 0)
            renderer.fill_rounded_rect(item_rect(i), theme_->corner_radius, theme_->highlight_bg);
        const Color c = !it.enabled ? theme_->text_disabled : sel ? theme_->highlight_text : theme_->text;
        const std::string text = sel ? theme_->menu_marker + it.label : it.label;
        f->draw(renderer, text, {anchor_.x, anchor_.y + item_height() * static_cast<float>(i)}, theme_->style(c, align_));
    }
}

}  // namespace vs
