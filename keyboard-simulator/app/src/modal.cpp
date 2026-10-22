#include "modal.hpp"

#include <algorithm>

#include "draw_util.hpp"
#include "palette.hpp"

namespace ks {

namespace {
constexpr float kButtonGap = 12.0f;
constexpr float kTitleGap = 14.0f;
constexpr float kButtonsGap = 26.0f;
}  // namespace

void Modal::open(Spec spec) {
    spec_ = std::move(spec);
    open_ = true;
    appear_ = 0;
    selected_ = std::clamp(spec_.selected, 0, std::max(0, static_cast<int>(spec_.buttons.size()) - 1));
    hovered_ = -1;
    layout();
}

float Modal::block_height(const Block& b) const {
    const Fonts& f = *fonts_;
    vs::TextStyle wrap;
    wrap.max_width = content_width();
    switch (b.kind) {
    case Block::Kind::Heading: return 12.0f + f.ui_bold.line_height() + 2.0f;
    case Block::Kind::Text: return f.ui.measure_block(b.text, wrap).y + 2.0f;
    case Block::Kind::Center: return f.ui.measure_block(b.text, wrap).y + 2.0f;
    case Block::Kind::CenterTitle: return f.title.line_height() + 4.0f;
    case Block::Kind::CenterMuted: return f.small.measure_block(b.text, wrap).y + 2.0f;
    case Block::Kind::Image: return b.size + 14.0f;
    case Block::Kind::Divider: return 25.0f;
    case Block::Kind::Space: return b.size;
    }
    return 0;
}

void Modal::layout() {
    float h = kPad;
    if (!spec_.title.empty()) h += fonts_->title.line_height() + kTitleGap;
    for (const Block& b : spec_.blocks) h += block_height(b);
    h += kButtonsGap + kButtonH + kPad;
    card_ = {(screen_.x - spec_.width) * 0.5f, (screen_.y - h) * 0.5f, spec_.width, h};

    button_rects_.clear();
    const std::size_t n = spec_.buttons.size();
    std::vector<float> widths;
    float total = 0;
    for (const ModalButton& b : spec_.buttons) {
        widths.push_back(std::max(140.0f, fonts_->ui_bold.measure(b.label) + 48.0f));
        total += widths.back();
    }
    total += kButtonGap * static_cast<float>(n > 0 ? n - 1 : 0);
    float x = spec_.center_buttons ? card_.center().x - total * 0.5f : card_.right() - kPad - total;
    const float y = card_.bottom() - kPad - kButtonH;
    for (std::size_t i = 0; i < n; ++i) {
        button_rects_.push_back({x, y, widths[i], kButtonH});
        x += widths[i] + kButtonGap;
    }
}

void Modal::activate(int index) {
    if (index < 0 || index >= static_cast<int>(spec_.buttons.size())) return;
    open_ = false;
    if (on_click) on_click();
    if (auto action = spec_.buttons[static_cast<std::size_t>(index)].action) action();
}

void Modal::update(const vs::Input& input, double dt) {
    if (!open_) return;
    appear_ = std::min(1.0f, appear_ + static_cast<float>(dt) / 0.16f);
    const int n = static_cast<int>(spec_.buttons.size());
    hovered_ = -1;
    for (int i = 0; i < n; ++i)
        if (button_rects_[static_cast<std::size_t>(i)].contains(input.mouse())) hovered_ = i;
    if (input.mouse_moved() && hovered_ >= 0) selected_ = hovered_;
    if (input.mouse_released(SDL_BUTTON_LEFT) && hovered_ >= 0) return activate(hovered_);
    if (n == 0) return;
    if (input.repeated(SDLK_LEFT)) selected_ = std::max(0, selected_ - 1);
    if (input.repeated(SDLK_RIGHT)) selected_ = std::min(n - 1, selected_ + 1);
    if (input.repeated(SDLK_TAB)) selected_ = (selected_ + (input.shift() ? n - 1 : 1)) % n;
    if (input.pressed(SDLK_RETURN) || input.pressed(SDLK_KP_ENTER)) return activate(selected_);
    if (input.pressed(SDLK_ESCAPE)) return activate(spec_.cancel);
}

void Modal::render(vs::Renderer& r) const {
    if (!open_) return;
    const Fonts& f = *fonts_;
    const float e = ease_out(appear_);
    r.fill_rect({0, 0, screen_.x, screen_.y}, fade(pal::overlay, e));
    const vs::RectF card = offset(card_, 0, (1.0f - e) * 18.0f);
    draw_shadow(r, card, 20.0f, 2.2f);
    r.fill_rounded_rect(card, 20.0f, pal::card);

    const float x = card.x + kPad, w = content_width(), cx = card.center().x;
    float y = card.y + kPad;
    if (!spec_.title.empty()) {
        vs::TextStyle st;
        st.color = pal::ink;
        f.title.draw(r, spec_.title, {x, y}, st);
        y += f.title.line_height() + kTitleGap;
    }
    for (const Block& b : spec_.blocks) {
        vs::TextStyle st;
        st.max_width = w;
        switch (b.kind) {
        case Block::Kind::Heading:
            st.color = pal::accent_dark;
            f.ui_bold.draw(r, b.text, {x, y + 12.0f}, st);
            break;
        case Block::Kind::Text:
            st.color = pal::ink_soft;
            f.ui.draw(r, b.text, {x, y}, st);
            break;
        case Block::Kind::Center:
            st.color = pal::ink_soft;
            st.align = vs::Align::Center;
            f.ui.draw(r, b.text, {cx, y}, st);
            break;
        case Block::Kind::CenterTitle:
            st.color = pal::ink;
            st.align = vs::Align::Center;
            f.title.draw(r, b.text, {cx, y}, st);
            break;
        case Block::Kind::CenterMuted:
            st.color = pal::muted;
            st.align = vs::Align::Center;
            f.small.draw(r, b.text, {cx, y}, st);
            break;
        case Block::Kind::Image:
            if (b.image && b.image->valid())
                r.draw_stretched(*b.image, std::nullopt, {cx - b.size * 0.5f, y, b.size, b.size});
            break;
        case Block::Kind::Divider:
            r.fill_rect({x + w * 0.2f, y + 12.0f, w * 0.6f, 1.0f}, pal::divider);
            break;
        case Block::Kind::Space:
            break;
        }
        y += block_height(b);
    }

    for (std::size_t i = 0; i < spec_.buttons.size(); ++i) {
        const ModalButton& b = spec_.buttons[i];
        const vs::RectF rc = offset(button_rects_[i], 0, (1.0f - e) * 18.0f);
        const bool sel = static_cast<int>(i) == selected_;
        const bool hov = static_cast<int>(i) == hovered_;
        vs::Color fill = b.primary ? pal::accent : pal::card_alt;
        if (hov) fill = b.primary ? mix(pal::accent, pal::accent_dark, 0.35f) : pal::accent_soft;
        if (sel) draw_glow(r, rc, 12.0f, fade(pal::accent, 0.35f), 5.0f, 4);
        r.fill_rounded_rect(rc, 12.0f, fill);
        if (!b.primary) r.draw_rounded_rect(rc, 12.0f, sel ? pal::accent : pal::divider, sel ? 2.0f : 1.0f);
        vs::TextStyle st;
        st.color = b.primary ? vs::rgb(0xFFFFFF) : pal::ink;
        st.align = vs::Align::Center;
        f.ui_bold.draw_in_box(r, b.label, rc, st);
    }
}

}  // namespace ks
