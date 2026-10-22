#include <vs/engine/ui_dialog.hpp>

#include <algorithm>

namespace vs {

namespace {
SDL_Keycode hotkey(const std::string& label) {
    if (label.empty()) return SDLK_UNKNOWN;
    const unsigned char c = static_cast<unsigned char>(label[0]);
    if (c >= 'A' && c <= 'Z') return static_cast<SDL_Keycode>(c - 'A' + 'a');
    if (c >= 'a' && c <= 'z') return static_cast<SDL_Keycode>(c);
    return SDLK_UNKNOWN;
}
}  // namespace

void Dialog::open(std::string message, DialogButtons buttons, std::function<void(DialogResult)> on_close,
                  DialogResult default_choice) {
    message_ = std::move(message);
    buttons_ = buttons;
    on_close_ = std::move(on_close);
    open_ = true;
    selected_ = 0;
    const std::vector<Choice> list = choices();
    for (int i = 0; i < static_cast<int>(list.size()); ++i)
        if (list[static_cast<std::size_t>(i)].result == default_choice) selected_ = i;
}

std::vector<Dialog::Choice> Dialog::choices() const {
    switch (buttons_) {
    case DialogButtons::Ok:
        return {{DialogResult::Ok, &ok_label}};
    case DialogButtons::YesNo:
        return {{DialogResult::Yes, &yes_label}, {DialogResult::No, &no_label}};
    case DialogButtons::YesNoCancel:
    default:
        return {{DialogResult::Yes, &yes_label}, {DialogResult::No, &no_label}, {DialogResult::Cancel, &cancel_label}};
    }
}

RectF Dialog::panel_rect() const {
    const Font* body = theme_->title_font ? theme_->title_font : theme_->font;
    const float pad = theme_->padding;
    TextStyle st;
    st.max_width = width - pad * 4;
    const float text_h = body ? body->measure_block(message_, st).y : 40.0f;
    const float button_h = (theme_->font ? theme_->font->line_height() : 24.0f) + pad;
    const float h = pad * 2 + text_h + pad * 2 + button_h + pad;
    return {(screen_size.x - width) * 0.5f, (screen_size.y - h) * 0.5f, width, h};
}

RectF Dialog::button_rect(int index, int count) const {
    const RectF panel = panel_rect();
    const float pad = theme_->padding;
    const float button_h = (theme_->font ? theme_->font->line_height() : 24.0f) + pad;
    const float gap = pad;
    const float bw = std::min(180.0f, (panel.w - pad * 2 - gap * static_cast<float>(count - 1)) / static_cast<float>(count));
    const float total = bw * static_cast<float>(count) + gap * static_cast<float>(count - 1);
    const float x0 = panel.x + (panel.w - total) * 0.5f;
    return {x0 + static_cast<float>(index) * (bw + gap), panel.bottom() - pad - button_h, bw, button_h};
}

void Dialog::finish(DialogResult r) {
    open_ = false;
    theme_->sound(r == DialogResult::Cancel ? UiSound::Cancel : UiSound::Select);
    if (auto cb = std::move(on_close_)) cb(r);
}

void Dialog::update(const Input& input) {
    if (!open_) return;
    const std::vector<Choice> list = choices();
    const int n = static_cast<int>(list.size());
    auto move = [&](int dir) {
        const int next = std::clamp(selected_ + dir, 0, n - 1);
        if (next != selected_) {
            selected_ = next;
            theme_->sound(UiSound::Move);
        }
    };
    if (input.repeated(SDLK_LEFT)) move(-1);
    if (input.repeated(SDLK_RIGHT)) move(+1);
    if (input.repeated(SDLK_TAB)) {
        selected_ = (selected_ + (input.shift() ? n - 1 : 1)) % n;
        theme_->sound(UiSound::Move);
    }
    for (int i = 0; i < n; ++i) {
        const RectF r = button_rect(i, n);
        if (input.mouse_moved() && r.contains(input.mouse())) selected_ = i;
        if (input.mouse_pressed(SDL_BUTTON_LEFT) && r.contains(input.mouse())) {
            finish(list[static_cast<std::size_t>(i)].result);
            return;
        }
    }
    for (const Choice& c : list) {
        const SDL_Keycode k = hotkey(*c.label);
        if (k != SDLK_UNKNOWN && input.pressed(k)) {
            finish(c.result);
            return;
        }
    }
    if (input.pressed(SDLK_RETURN) || input.pressed(SDLK_KP_ENTER) || input.pressed(SDLK_SPACE)) {
        finish(list[static_cast<std::size_t>(selected_)].result);
        return;
    }
    if (input.pressed(SDLK_ESCAPE)) {
        finish(buttons_ == DialogButtons::YesNoCancel ? DialogResult::Cancel
               : buttons_ == DialogButtons::YesNo     ? DialogResult::No
                                                      : DialogResult::Ok);
    }
}

void Dialog::render(Renderer& renderer) const {
    if (!open_) return;
    renderer.fill_rect({0, 0, screen_size.x, screen_size.y}, theme_->overlay);
    const RectF panel = panel_rect();
    renderer.fill_rounded_rect(panel, theme_->corner_radius, theme_->panel_bg);
    if (theme_->border_width > 0)
        renderer.draw_rounded_rect(panel, theme_->corner_radius, theme_->panel_border, theme_->border_width);
    const Font* body = theme_->title_font ? theme_->title_font : theme_->font;
    const float pad = theme_->padding;
    if (body) {
        TextStyle st = theme_->style(theme_->text, Align::Center);
        st.max_width = width - pad * 4;
        body->draw(renderer, message_, {panel.center().x, panel.y + pad * 2}, st);
    }
    const std::vector<Choice> list = choices();
    const int n = static_cast<int>(list.size());
    for (int i = 0; i < n; ++i) {
        const RectF r = button_rect(i, n);
        const bool sel = i == selected_;
        renderer.fill_rounded_rect(r, theme_->corner_radius, sel ? theme_->button_hover : theme_->button_bg);
        if (theme_->border_width > 0)
            renderer.draw_rounded_rect(r, theme_->corner_radius, sel ? theme_->highlight_text : theme_->button_border,
                                       theme_->border_width);
        if (theme_->font)
            theme_->font->draw_in_box(renderer, *list[static_cast<std::size_t>(i)].label, r,
                                      theme_->style(sel ? theme_->highlight_text : theme_->button_text, Align::Center));
    }
}

}  // namespace vs
