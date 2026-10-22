#include "base_scene.hpp"

#include <SDL3/SDL.h>
#include <vs/engine/screenshot.hpp>
#include <vs/engine/storage.hpp>

namespace sg {

void update_canvas_scaling(vs::App& app) {
    int w = 0, h = 0;
    if (!app.window() || !SDL_GetWindowSizeInPixels(app.window(), &w, &h)) return;
    const bool integer = w >= app.config().logical_width && h >= app.config().logical_height;
    if (integer != app.config().integer_scale) app.set_integer_scale(integer);
}

void BaseScene::on_event(const SDL_Event& e) {
    if (e.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED || e.type == SDL_EVENT_WINDOW_RESIZED) update_canvas_scaling(app());
    if (e.type == SDL_EVENT_KEY_DOWN && !e.key.repeat) pending_keys_.push_back(e.key.key);
}

void BaseScene::update(double dt) {
    if (!res().script.empty()) res().script.pump(app().time());
    dt *= res().cmd.time_scale;
    if (quit_box_.is_open()) {
        // The scene logic (and a running level's clock) stands still while the question is
        // open; only the scene time runs on (ambient animations, --screenshot timing).
        pending_keys_.clear();
        logic_time_ += dt;
        last_dt_ = dt;
        time_ = logic_time_;
        quit_box_.update(input(), res(), dt);
        return;
    }
    keys_.swap(pending_keys_);
    pending_keys_.clear();
    logic_time_ += dt;
    last_dt_ = dt;
    time_ = logic_time_;
    tick(dt);
    keys_.clear();
}

void BaseScene::render(vs::Renderer& r, double alpha) {
    time_ = logic_time_ + alpha * last_dt_;
    draw(r);
    time_ = logic_time_;
    quit_box_.render(r, res());
    const std::string& shot = res().cmd.screenshot;
    if (!shot.empty() && !shot_done_ && app().top_scene() == this && screenshot_due()) {
        shot_done_ = true;
        const bool saved = vs::save_frame_png(r, vs::utf8_to_path(shot));
        if (saved) SDL_Log("screenshot saved: %s", shot.c_str());
        app().quit(saved ? 0 : 1);
    }
}

bool BaseScene::on_quit_request() {
    const CommandLine& cl = res().cmd;
    const bool automated = app().config().smoke || app().config().quit_after > 0 || cl.verify_replays ||
                           !cl.test_level.empty();
    if (automated) {
        res().save_settings();
        return true;
    }
    ask_quit();
    return false;
}

void BaseScene::ask_quit() {
    if (quit_box_.is_open()) return;
    quit_box_.open("EXIT SOLDIER?", [this](bool yes) {
        if (!yes) return;
        res().save_settings();
        app().quit(0);
    });
}

bool BaseScene::screenshot_due() const { return logic_time_ >= res().cmd.shot_time; }

}  // namespace sg
