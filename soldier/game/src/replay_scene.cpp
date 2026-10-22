#include "replay_scene.hpp"

#include <algorithm>

#include "levels.hpp"

namespace sg {

namespace {
constexpr int kExtraTicks = 4000;  // a recording that never ends stops this long after its last command
}

ReplayScene::ReplayScene(ResourcesPtr r, soldier::Level level, soldier::Recording rec)
    : LevelScene(std::move(r), level), replayer_(std::move(level), rec), player_(rec.player) {
    if (rec.result) expected_tick_ = rec.result->tick;
    if (!rec.commands.empty()) last_command_tick_ = rec.commands.back().tick;
    const double s = res().cmd.replay_speed;
    speed_ = s >= 4 ? 4 : s >= 2 ? 2 : 1;
    init_view();
}

void ReplayScene::set_verify(std::function<void(const ReplayResult&)> done) {
    verify_ = std::move(done);
    max_ticks_per_step_ = 400;
}

void ReplayScene::on_enter() {
    update_canvas_scaling(app());
    if (!verify_) start_music();
}

double ReplayScene::tick_length() const {
    if (verify_) return 1e-5;
    return res().save.settings.tick_seconds() / speed_;
}

void ReplayScene::advance_tick() {
    replayer_.step();
    // A recording that never finishes (should not happen) is stopped by giving up.
    if (!replayer_.finished() && !replayer_.commands_left() &&
        replayer_.game().tick_count() > last_command_tick_ + kExtraTicks) {
        flush_events();
        replayer_.game().apply(soldier::Command::give_up());
    }
}

void ReplayScene::handle_input() {
    if (verify_) return;
    for (SDL_Keycode k : keys()) {
        if (k == SDLK_1) speed_ = 1;
        else if (k == SDLK_2) speed_ = 2;
        else if (k == SDLK_4) speed_ = 4;
        else if (k == SDLK_Z) toggle_zoom();
        else if (k == SDLK_TAB) speed_ = speed_ == 1 ? 2 : speed_ == 2 ? 4 : 1;
        else {
            on_continue();
            return;
        }
    }
}

void ReplayScene::on_game_over() {
    if (!verify_ || reported_) return;
    reported_ = true;
    ReplayResult r;
    r.level = level_.number;
    r.status = replayer_.game().status();
    r.tick = replayer_.game().tick_count();
    r.expected_tick = expected_tick_;
    sounds_.clear();
    verify_(r);
}

void ReplayScene::on_continue() {
    if (verify_) return;
    if (res().cmd.scene == "replay" && !res().cmd.record.empty()) {
        app().quit(0);
        return;
    }
    app().replace_scene(make_menu_scene(shared_res()));
}

HudInfo ReplayScene::hud_info() const {
    HudInfo h;
    h.level = level_.number;
    h.replay = true;
    h.player = player_;
    h.replay_speed = "x" + std::to_string(speed_);
    return h;
}

std::string ReplayScene::end_title() const {
    if (replayer_.game().status() == soldier::Status::Won) return "REPLAY COMPLETE";
    return LevelScene::end_title();
}

}  // namespace sg
