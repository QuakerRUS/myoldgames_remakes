#include "options_panel.hpp"

#include <algorithm>

#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_mouse.h>

#include "ui_kit.hpp"

namespace sg {

namespace {
constexpr vs::RectF kBox{340, 120, 600, 494};
}

void OptionsPanel::open() {
    open_ = true;
    row_ = 0;
}

vs::RectF OptionsPanel::row_rect(int i) const {
    return {kBox.x + 30, kBox.y + 92 + static_cast<float>(i) * 54, kBox.w - 60, 46};
}

void OptionsPanel::adjust(vs::App& app, Resources& res, int dir) {
    Settings& s = res.save.settings;
    switch (row_) {
    case Speed: s.speed = std::clamp(s.speed + dir, 0, static_cast<int>(kSpeedSteps.size()) - 1); break;
    case Sound:
        s.sound = std::clamp(s.sound + dir, 0, 10);
        res.apply_volumes();
        res.play("button_press", 0.9f);
        break;
    case Music:
        s.music = std::clamp(s.music + dir, 0, 10);
        res.apply_volumes();
        break;
    case Zoom: s.zoom2 = !s.zoom2; break;
    case Fullscreen: app.set_fullscreen(!app.is_fullscreen()); break;  // the listener saves it
    default: return;
    }
    res.play("menu_move", 0.6f);
    res.save_settings();
}

bool OptionsPanel::update(vs::App& app, Resources& res, double) {
    if (!open_) return false;
    const vs::Input& in = app.input();
    if (in.repeated(SDLK_UP)) {
        row_ = (row_ + RowCount - 1) % RowCount;
        res.play("menu_move", 0.6f);
    }
    if (in.repeated(SDLK_DOWN) || in.repeated(SDLK_TAB)) {
        row_ = (row_ + 1) % RowCount;
        res.play("menu_move", 0.6f);
    }
    for (int i = 0; i < RowCount; ++i)
        if (in.mouse_moved() && row_rect(i).contains(in.mouse())) row_ = i;
    if (in.repeated(SDLK_LEFT)) adjust(app, res, -1);
    if (in.repeated(SDLK_RIGHT)) adjust(app, res, +1);
    const bool click = in.mouse_pressed(SDL_BUTTON_LEFT) && row_rect(row_).contains(in.mouse());
    if (in.pressed(SDLK_RETURN) || in.pressed(SDLK_KP_ENTER) || in.pressed(SDLK_SPACE) || click) {
        if (row_ == Back) {
            res.play("menu_select", 0.7f);
            open_ = false;
        } else if (row_ == Fullscreen || row_ == Zoom) {
            adjust(app, res, 1);
        } else {
            // Enter cycles forward and wraps.
            Settings& s = res.save.settings;
            if (row_ == Speed && s.speed == static_cast<int>(kSpeedSteps.size()) - 1) s.speed = -1;
            if (row_ == Sound && s.sound == 10) s.sound = -1;
            if (row_ == Music && s.music == 10) s.music = -1;
            adjust(app, res, 1);
        }
    }
    if (in.pressed(SDLK_ESCAPE)) {
        res.play("menu_select", 0.7f);
        open_ = false;
    }
    return true;
}

void OptionsPanel::render(vs::Renderer& r, const Resources& res, const vs::App& app, double time) const {
    if (!open_) return;
    dim_screen(r, 150);
    draw_panel(r, res, kBox);
    draw_text(r, res.font24, "OPTIONS", {kBox.center().x, kBox.y + 36}, colors::accent, vs::Align::Center);
    const Settings& s = res.save.settings;
    static const char* labels[] = {"GAME SPEED", "SOUND", "MUSIC", "ZOOM", "FULLSCREEN", "BACK"};
    for (int i = 0; i < RowCount; ++i) {
        const vs::RectF rr = row_rect(i);
        const bool sel = i == row_;
        if (sel) {
            r.fill_rect(rr, vs::rgba(0xFFC93322));
            draw_marker(r, res, {rr.x + 4, rr.center().y}, time);
        }
        const vs::Color c = sel ? colors::accent : colors::text;
        draw_text(r, res.font16, labels[i], {rr.x + 24, rr.y + 15}, c);
        if (i == Back) continue;
        const float vx = rr.right() - 160;
        std::string value;
        if (i == Speed) value = s.speed_label();
        if (i == Zoom) value = s.zoom2 ? "2X" : "1X";
        if (i == Fullscreen) value = app.is_fullscreen() ? "ON" : "OFF";
        if (i == Sound || i == Music) {
            const int v = i == Sound ? s.sound : s.music;
            for (int k = 0; k < 10; ++k)
                r.fill_rect({vx + 4 + static_cast<float>(k) * 12, rr.y + 13, 9, 20},
                            k < v ? (sel ? colors::accent : colors::green) : vs::rgba(0xFFFFFF22));
        } else {
            draw_text(r, res.font16, value, {vx + 64, rr.y + 15}, c, vs::Align::Center);
        }
        if (sel) {
            draw_text(r, res.font16, "<", {vx - 20, rr.y + 15}, colors::dim);
            draw_text(r, res.font16, ">", {vx + 132, rr.y + 15}, colors::dim);
        }
    }
    draw_text(r, res.body_small, "UP/DOWN SELECT   LEFT/RIGHT CHANGE   ESC BACK", {kBox.center().x, kBox.bottom() - 40},
              colors::dim, vs::Align::Center);
}

}  // namespace sg
