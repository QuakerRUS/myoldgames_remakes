#include "level_scene.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "ui_kit.hpp"

namespace sg {

namespace {
constexpr float kMapW = kPanelX;  // the map uses the screen left of the side panel
constexpr float kMapH = 720;
}  // namespace

LevelScene::LevelScene(ResourcesPtr res, soldier::Level level) : BaseScene(std::move(res)), level_(std::move(level)) {}

void LevelScene::init_view() {
    view_ = std::make_unique<LevelView>(game());
    cam_.viewport = {0, 0, kMapW, kMapH};
    const float ww = static_cast<float>(level_.width) * kCell, wh = static_cast<float>(level_.height) * kCell;
    cam_.set_bounds({0, 0, ww, wh});
    apply_zoom();
    flush_events();  // a replay applies the commands of tick 0 in its constructor
}

void LevelScene::apply_zoom() {
    cam_.zoom = res().save.settings.zoom2 ? 2.0f : 1.0f;
    cam_.center_on(view_->player_center(time()));
}

void LevelScene::toggle_zoom() {
    res().save.settings.zoom2 = !res().save.settings.zoom2;
    res().save_settings();
    res().play("menu_move", 0.6f);
    apply_zoom();
}

void LevelScene::start_music() {
    const int t = game().timer();
    fast_music_ = t >= 0 && t <= 15;
    res().play_music(fast_music_ ? "game_fast" : "game", true, 1.0);
}

ViewClock LevelScene::clock() const { return {time(), tick_length(), acc_}; }

void LevelScene::flush_events() {
    const ViewClock c = clock();
    for (const soldier::Event& e : game().take_events()) {
        if (res().cmd.log_events)
            std::printf("tick %d %s at %d,%d to %d,%d value %d\n", game().tick_count(),
                        std::string(soldier::event_name(e.type)).c_str(), e.at.x, e.at.y, e.to.x, e.to.y, e.value);
        view_->on_event(e, c);
        sounds_.on_event(e, game(), *view_, c);
        if (e.type == soldier::EventType::TimerChanged && e.value <= 15 && !fast_music_) {
            fast_music_ = true;
            res().play_music("game_fast", true, 0.5);
        }
    }
}

void LevelScene::apply(soldier::Command c) {
    game().apply(c);
    flush_events();
}

void LevelScene::tick(double dt) {
    if (!prompt_) handle_input();
    if (prompt_) {
        if (!keys().empty() || input().mouse_pressed(SDL_BUTTON_LEFT)) on_continue();
    }

    if (!paused_) {
        acc_ += dt;
        const double len = tick_length();
        int n = 0;
        while (acc_ >= len && game().status() == soldier::Status::Playing && n < max_ticks_per_step_) {
            acc_ -= len;
            advance_tick();
            flush_events();
            ++n;
        }
        if (game().status() == soldier::Status::Playing) {
            if (n == max_ticks_per_step_) acc_ = std::min(acc_, len);
            after_ticks();
        }
    }

    if (game().status() != soldier::Status::Playing && !over_) {
        over_ = true;
        over_time_ = time();
        if (game().status() == soldier::Status::Won)
            res().play_music("victory", false, 0.3);
        else
            app().audio().stop_music(1.5);
        on_game_over();
    }
    if (over_ && !prompt_ && view_->ending_done(time()) && time() > over_time_ + ending_pause()) prompt_ = true;

    sounds_.update(res(), time());
    cam_.follow(view_->player_center(time()), dt, 6.0f);
    const int st = res().cmd.shot_tick;
    if (st >= 0 && shot_reached_ < 0 && (game().tick_count() >= st || over_)) shot_reached_ = time();
}

bool LevelScene::screenshot_due() const {
    if (res().cmd.shot_tick >= 0) return shot_reached_ >= 0 && time() >= shot_reached_ + res().cmd.shot_delay;
    return BaseScene::screenshot_due();
}

std::string LevelScene::end_title() const {
    if (game().status() == soldier::Status::Won) return "LEVEL COMPLETE";
    switch (game().death_cause()) {
    case soldier::DeathCause::GaveUp: return "MISSION ABORTED";
    case soldier::DeathCause::TimeOut: return "TIME IS OUT";
    case soldier::DeathCause::Guillotine: return "GUILLOTINED";
    case soldier::DeathCause::Drowned: return "DROWNED";
    }
    return {};
}

std::string LevelScene::end_subtitle() const { return {}; }

void LevelScene::draw_end_message(vs::Renderer& r) {
    const bool won = game().status() == soldier::Status::Won;
    r.fill_rect({0, 0, kMapW, kMapH}, vs::Color{0, 0, 0, 110});
    const std::string title = end_title(), sub = end_subtitle();
    const float w = std::max(560.0f, res().font24.measure(title) + 120);
    const vs::RectF box{(kMapW - w) * 0.5f, 260, w, sub.empty() ? 170.0f : 210.0f};
    draw_panel(r, res(), box);
    float y = box.y + 40;
    draw_text(r, res().font24, title, {box.center().x, y}, won ? colors::green : colors::red, vs::Align::Center);
    y += 48;
    if (!sub.empty()) {
        draw_text(r, res().body, sub, {box.center().x, y}, colors::text, vs::Align::Center);
        y += 40;
    }
    const uint8_t a = static_cast<uint8_t>(150 + 105 * (0.5 + 0.5 * std::sin(time() * 5.0)));
    draw_text(r, res().font16, "PRESS ANY KEY TO CONTINUE", {box.center().x, y}, colors::green.with_alpha(a),
              vs::Align::Center);
}

void LevelScene::draw(vs::Renderer& r) {
    // The camera follows in update(); rendering only interpolates the view time.
    view_->render(r, res(), cam_, clock());

    // "LEVEL n" banner at the start.
    if (time() < 2.2) {
        const float t = static_cast<float>(time());
        const float a = t < 1.4f ? 1.0f : 1.0f - (t - 1.4f) / 0.8f;
        const uint8_t alpha = static_cast<uint8_t>(255 * std::clamp(a, 0.0f, 1.0f));
        const std::string text = "LEVEL " + std::to_string(level_.number);
        const float tw = res().font32.measure(text) + 64;
        r.fill_rounded_rect({(kMapW - tw) / 2, 40, tw, 64}, 8, vs::Color{0, 0, 0, static_cast<uint8_t>(alpha * 0.6f)});
        draw_text(r, res().font32, text, {kMapW / 2, 56}, colors::accent.with_alpha(alpha), vs::Align::Center);
    }

    draw_hud(r, res(), game(), hud_info(), time());

    if (paused_) {
        r.fill_rect({0, 0, kMapW, kMapH}, vs::Color{0, 0, 0, 120});
        draw_text(r, res().font32, "PAUSED", {kMapW / 2, 320}, colors::accent, vs::Align::Center);
        draw_text(r, res().body, "PRESS P TO CONTINUE", {kMapW / 2, 370}, colors::text, vs::Align::Center);
    }
    if (prompt_) draw_end_message(r);
}

}  // namespace sg
