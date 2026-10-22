#include <vs/engine/ui_button.hpp>

namespace vs {

void Button::update(const Input& input) {
    hovered_ = enabled && rect.contains(input.mouse());
    if (!enabled) {
        pressed_ = false;
        return;
    }
    if (input.mouse_pressed(SDL_BUTTON_LEFT) && hovered_) pressed_ = true;
    if (input.mouse_released(SDL_BUTTON_LEFT)) {
        const bool click = pressed_ && hovered_;
        pressed_ = false;
        if (click) {
            theme_->sound(UiSound::Select);
            if (on_click) on_click();
            return;
        }
    }
    if (hotkey != SDLK_UNKNOWN && input.pressed(hotkey)) {
        theme_->sound(UiSound::Select);
        if (on_click) on_click();
    }
}

void Button::render(Renderer& renderer) const {
    Color bg = theme_->button_bg;
    if (active) bg = theme_->button_active;
    if (hovered_) bg = theme_->button_hover;
    if (pressed_) bg = theme_->button_pressed;
    const RectF r = pressed_ ? RectF{rect.x, rect.y + 1, rect.w, rect.h} : rect;
    renderer.fill_rounded_rect(r, theme_->corner_radius, bg);
    if (theme_->border_width > 0)
        renderer.draw_rounded_rect(r, theme_->corner_radius, theme_->button_border, theme_->border_width);
    if (theme_->font) {
        TextStyle st;
        st.color = enabled ? theme_->button_text : theme_->text_disabled;
        st.align = Align::Center;
        theme_->font->draw_in_box(renderer, label, r, st);
    }
}

}  // namespace vs
