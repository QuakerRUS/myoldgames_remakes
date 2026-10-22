#include "ui_kit.hpp"

#include <algorithm>
#include <cmath>

#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_mouse.h>

namespace sg {

namespace {
constexpr float kScreenW = 1280, kScreenH = 720;
}

void draw_nine(vs::Renderer& r, const vs::SpriteSheet& sheet, std::string_view frame, int border, vs::RectF dst,
               vs::Color tint) {
    const vs::FrameRect* f = sheet.frame(frame);
    if (!f) return;
    const float b = static_cast<float>(border);
    const float fx = static_cast<float>(f->x), fy = static_cast<float>(f->y);
    const float fw = static_cast<float>(f->w), fh = static_cast<float>(f->h);
    const float bw = std::min(b, dst.w * 0.5f), bh = std::min(b, dst.h * 0.5f);
    const float sx[4] = {fx, fx + b, fx + fw - b, fx + fw};
    const float sy[4] = {fy, fy + b, fy + fh - b, fy + fh};
    const float dx[4] = {dst.x, dst.x + bw, dst.right() - bw, dst.right()};
    const float dy[4] = {dst.y, dst.y + bh, dst.bottom() - bh, dst.bottom()};
    for (int j = 0; j < 3; ++j)
        for (int i = 0; i < 3; ++i) {
            const vs::RectF src{sx[i], sy[j], sx[i + 1] - sx[i], sy[j + 1] - sy[j]};
            const vs::RectF d{dx[i], dy[j], dx[i + 1] - dx[i], dy[j + 1] - dy[j]};
            if (d.w <= 0 || d.h <= 0) continue;
            r.draw_stretched(sheet.texture(), src, d, tint);
        }
}

void draw_panel(vs::Renderer& r, const Resources& res, vs::RectF dst, uint8_t alpha) {
    // Soft drop shadow first.
    r.fill_rounded_rect({dst.x + 6, dst.y + 8, dst.w, dst.h}, 10, vs::Color{0, 0, 0, static_cast<uint8_t>(alpha / 3)});
    draw_nine(r, res.ui, "frame9", 16, dst, vs::Color{255, 255, 255, alpha});
}

void draw_inset(vs::Renderer& r, const Resources& res, vs::RectF dst) { draw_nine(r, res.ui, "inset9", 8, dst); }

void draw_button(vs::Renderer& r, const Resources& res, vs::RectF dst, bool hot) {
    draw_nine(r, res.ui, hot ? "button9_hot" : "button9", 8, dst);
}

vs::Vec2 draw_text(vs::Renderer& r, const vs::Font& f, std::string_view text, vs::Vec2 pos, vs::Color c, vs::Align align) {
    vs::TextStyle st;
    st.color = c;
    st.align = align;
    st.shadow = vs::Color{0, 0, 0, static_cast<uint8_t>(std::min<int>(200, c.a))};
    const float s = std::max(1.0f, std::floor(f.size() / 12.0f));
    st.shadow_offset = {s, s};
    return f.draw(r, text, pos, st);
}

void draw_marker(vs::Renderer& r, const Resources& res, vs::Vec2 pos, double time) {
    const vs::AnimationDef* a = res.ui.animation("menu_arrow");
    std::string frame = "menu_arrow_0";
    if (a && a->total_ms() > 0) {
        double ms = std::fmod(time * 1000.0, static_cast<double>(a->total_ms()));
        for (const auto& fr : a->frames) {
            if (ms < fr.ms) {
                frame = fr.frame;
                break;
            }
            ms -= fr.ms;
        }
    }
    vs::DrawParams p;
    p.scale = {2, 2};
    res.ui.draw(r, frame, {pos.x - 32, pos.y - 16}, p);
}

void dim_screen(vs::Renderer& r, uint8_t alpha) { r.fill_rect({0, 0, kScreenW, kScreenH}, vs::Color{0, 0, 0, alpha}); }

// ---- NoticeBox ------------------------------------------------------------------------

void NoticeBox::open(std::string title, std::string line, vs::Color title_color, std::function<void()> on_close) {
    open_ = true;
    title_ = std::move(title);
    line_ = std::move(line);
    color_ = title_color;
    on_close_ = std::move(on_close);
    t_ = 0;
}

bool NoticeBox::update(const vs::Input& in, double dt) {
    if (!open_) return false;
    t_ += dt;
    if (t_ > 0.15 && in.any_pressed()) {
        open_ = false;
        if (auto cb = std::move(on_close_)) cb();
    }
    return true;
}

void NoticeBox::render(vs::Renderer& r, const Resources& res) const {
    if (!open_) return;
    dim_screen(r, 140);
    const float w = std::max(560.0f, res.font24.measure(title_) + 120);
    const vs::RectF box{(kScreenW - w) * 0.5f, 250, w, line_.empty() ? 190.0f : 230.0f};
    draw_panel(r, res, box);
    float y = box.y + 42;
    draw_text(r, res.font24, title_, {box.center().x, y}, color_, vs::Align::Center);
    y += 50;
    if (!line_.empty()) {
        draw_text(r, res.body, line_, {box.center().x, y}, colors::text, vs::Align::Center);
        y += 44;
    }
    const uint8_t a = static_cast<uint8_t>(150 + 105 * (0.5 + 0.5 * std::sin(t_ * 5.0)));
    draw_text(r, res.font16, "PRESS ANY KEY TO CONTINUE", {box.center().x, y}, colors::green.with_alpha(a),
              vs::Align::Center);
}

// ---- ConfirmBox ------------------------------------------------------------------------

void ConfirmBox::open(std::string question, std::function<void(bool)> on_answer, bool default_yes) {
    open_ = true;
    yes_ = default_yes;
    question_ = std::move(question);
    on_answer_ = std::move(on_answer);
    t_ = 0;
}

vs::RectF ConfirmBox::button_rect(bool yes) const {
    const float cx = kScreenW * 0.5f;
    return {yes ? cx - 170.0f : cx + 20.0f, 360, 150, 52};
}

void ConfirmBox::finish(bool yes, Resources& res) {
    open_ = false;
    res.play("menu_select", 0.7f);
    if (auto cb = std::move(on_answer_)) cb(yes);
}

bool ConfirmBox::update(const vs::Input& in, Resources& res, double dt) {
    if (!open_) return false;
    t_ += dt;
    if (in.repeated(SDLK_LEFT) || in.repeated(SDLK_RIGHT) || in.repeated(SDLK_TAB)) {
        yes_ = !yes_;
        res.play("menu_move", 0.6f);
    }
    for (bool b : {true, false}) {
        if (in.mouse_moved() && button_rect(b).contains(in.mouse())) yes_ = b;
        if (in.mouse_pressed(SDL_BUTTON_LEFT) && button_rect(b).contains(in.mouse())) {
            finish(b, res);
            return true;
        }
    }
    if (in.pressed(SDLK_Y)) finish(true, res);
    else if (in.pressed(SDLK_N) || in.pressed(SDLK_ESCAPE)) finish(false, res);
    else if (in.pressed(SDLK_RETURN) || in.pressed(SDLK_KP_ENTER) || in.pressed(SDLK_SPACE)) finish(yes_, res);
    return true;
}

void ConfirmBox::render(vs::Renderer& r, const Resources& res) const {
    if (!open_) return;
    dim_screen(r, 140);
    const float w = std::max(560.0f, res.font24.measure(question_) + 120);
    const vs::RectF box{(kScreenW - w) * 0.5f, 250, w, 190};
    draw_panel(r, res, box);
    draw_text(r, res.font24, question_, {box.center().x, box.y + 42}, colors::accent, vs::Align::Center);
    for (bool b : {true, false}) {
        const vs::RectF br = button_rect(b);
        const bool hot = b == yes_;
        draw_button(r, res, br, hot);
        res.font16.draw_in_box(r, b ? "YES" : "NO", br,
                               [&] {
                                   vs::TextStyle st;
                                   st.color = hot ? colors::accent : colors::text;
                                   st.align = vs::Align::Center;
                                   st.shadow = colors::shadow;
                                   return st;
                               }());
    }
}

}  // namespace sg
