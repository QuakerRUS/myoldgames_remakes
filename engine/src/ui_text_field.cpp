#include <vs/engine/ui_text_field.hpp>

#include <cmath>

#include <vs/engine/utf8.hpp>

namespace vs {

bool TextField::insert(std::string_view utf8) {
    bool added = false;
    std::size_t len = utf8_length(text);
    std::size_t pos = 0;
    while (pos < utf8.size()) {
        char32_t c = utf8_next(utf8, pos);
        if (uppercase) c = to_upper(c);
        if (c < 0x20 || c == 0x7F || c == kReplacementChar) continue;
        if (filter && !filter(c)) continue;
        if (len >= max_length) break;
        utf8_append(text, c);
        ++len;
        added = true;
    }
    return added;
}

void TextField::update(const Input& input, double dt) {
    blink_ = std::fmod(blink_ + dt, 1.0);
    if (!input.text().empty()) {
        if (insert(input.text())) {
            theme_->sound(UiSound::Type);
            blink_ = 0;
        } else {
            theme_->sound(UiSound::Error);
        }
    }
    if (input.repeated(SDLK_BACKSPACE) && !text.empty()) {
        utf8_pop_back(text);
        theme_->sound(UiSound::Type);
        blink_ = 0;
    }
    if ((input.pressed(SDLK_RETURN) || input.pressed(SDLK_KP_ENTER)) && on_submit) {
        theme_->sound(UiSound::Select);
        on_submit(text);
    } else if (input.pressed(SDLK_ESCAPE) && on_cancel) {
        theme_->sound(UiSound::Cancel);
        on_cancel();
    }
}

void TextField::render(Renderer& renderer) const {
    if (draw_box) {
        renderer.fill_rounded_rect(rect, theme_->corner_radius, theme_->field_bg);
        if (theme_->border_width > 0)
            renderer.draw_rounded_rect(rect, theme_->corner_radius, theme_->field_border, theme_->border_width);
    }
    const Font* f = theme_->font;
    if (!f) return;
    const RectF inner{rect.x + theme_->padding, rect.y, rect.w - theme_->padding * 2, rect.h};
    TextStyle st = theme_->style(text.empty() ? theme_->text_disabled : theme_->text, align);
    const std::string& shown = text.empty() ? placeholder : text;
    f->draw_in_box(renderer, shown, inner, st);
    // Caret after the last character.
    if (blink_ < 0.5) {
        const float w = text.empty() ? 0.0f : f->measure(text);
        float x = inner.x + w;
        if (align == Align::Center) x = inner.center().x + w * 0.5f;
        if (align == Align::Right) x = inner.right();
        const float h = f->line_height();
        renderer.fill_rect({x + 1, rect.y + (rect.h - h) * 0.5f, 2, h}, theme_->caret);
    }
}

}  // namespace vs
