#include "game_scene.hpp"

#include <SDL3/SDL_log.h>

#include "levels.hpp"

namespace sg {

using soldier::Command;
using soldier::Dir;

GameScene::GameScene(ResourcesPtr r, soldier::Level level, GameOptions opts)
    : LevelScene(std::move(r), level), game_(std::move(level)), opts_(opts) {
    init_view();
    if (res().cmd.autoplay_record) {
        autoplay_ = builtin_record(level_.number);
        if (autoplay_) apply_autoplay();
    }
}

void GameScene::on_enter() {
    update_canvas_scaling(app());
    start_music();
}

double GameScene::tick_length() const { return res().save.settings.tick_seconds(); }

void GameScene::apply_autoplay() {
    const auto& cmds = autoplay_->commands;
    while (autoplay_next_ < cmds.size() && cmds[autoplay_next_].tick <= game_.tick_count()) {
        if (cmds[autoplay_next_].tick == game_.tick_count()) apply(cmds[autoplay_next_].command);
        ++autoplay_next_;
    }
}

void GameScene::advance_tick() {
    game_.tick();
    if (autoplay_) apply_autoplay();
    if (res().cmd.give_up_at >= 0 && game_.tick_count() == res().cmd.give_up_at) {
        flush_events();
        apply(Command::give_up());
    }
}

namespace {
// Keys that produce no character in the original (INKEY$ returned nothing for them).
bool is_modifier(SDL_Keycode k) {
    switch (k) {
    case SDLK_LSHIFT:
    case SDLK_RSHIFT:
    case SDLK_LCTRL:
    case SDLK_RCTRL:
    case SDLK_LALT:
    case SDLK_RALT:
    case SDLK_LGUI:
    case SDLK_RGUI:
    case SDLK_CAPSLOCK:
    case SDLK_NUMLOCKCLEAR:
    case SDLK_SCROLLLOCK:
    case SDLK_MODE: return true;
    default: return false;
    }
}
bool is_arrow(SDL_Keycode k) { return k == SDLK_DOWN || k == SDLK_UP || k == SDLK_LEFT || k == SDLK_RIGHT; }
}  // namespace

void GameScene::handle_input() {
    for (SDL_Keycode k : keys()) {
        if (is_modifier(k)) continue;
        // As in the original, every key overwrites the one-key buffer: any key but an arrow
        // (unmapped keys, P, Z included) cancels a buffered step. A cancelled step is never
        // executed, so it never appears in the recording. Keys while paused do not reach the game.
        if (!is_arrow(k) && !paused_ && !autoplay_ && game_.status() == soldier::Status::Playing)
            game_.discard_buffered_move();
        if (k == SDLK_Z) {
            toggle_zoom();
            continue;
        }
        if (k == SDLK_P) {
            if (game_.status() == soldier::Status::Playing) paused_ = !paused_;
            continue;
        }
        if (paused_ || game_.status() != soldier::Status::Playing) continue;
        if (autoplay_ && k != SDLK_ESCAPE) continue;
        switch (k) {
        case SDLK_DOWN:
        case SDLK_UP:
        case SDLK_LEFT:
        case SDLK_RIGHT: {
            const Dir d = k == SDLK_DOWN ? Dir::Down : k == SDLK_UP ? Dir::Up : k == SDLK_LEFT ? Dir::Left : Dir::Right;
            held_ = d;
            held_tick_ = game_.tick_count();
            apply(Command::move(d));
            break;
        }
        case SDLK_COMMA:
        case SDLK_LESS: apply(Command::turn_ccw()); break;
        case SDLK_PERIOD:
        case SDLK_GREATER: apply(Command::turn_cw()); break;
        case SDLK_TAB: apply(Command::next_item()); break;
        case SDLK_SPACE:
        case SDLK_RETURN:
        case SDLK_KP_ENTER: apply(Command::use_item()); break;
        case SDLK_ESCAPE: apply(Command::give_up()); break;
        default: break;
        }
    }
}

bool GameScene::any_arrow_down(Dir& d) const {
    auto key = [](Dir x) {
        switch (x) {
        case Dir::Down: return SDLK_DOWN;
        case Dir::Up: return SDLK_UP;
        case Dir::Left: return SDLK_LEFT;
        case Dir::Right: return SDLK_RIGHT;
        }
        return SDLK_DOWN;
    };
    if (held_ && input().down(key(*held_))) {
        d = *held_;
        return true;
    }
    for (Dir x : {Dir::Down, Dir::Up, Dir::Left, Dir::Right})
        if (input().down(key(x))) {
            d = x;
            return true;
        }
    return false;
}

void GameScene::after_ticks() {
    // Holding an arrow keeps walking: a new step as soon as the cooldown allows it
    // (at most one command per tick, so that a held key against a wall does not flood).
    if (paused_ || autoplay_) return;
    Dir d{};
    if (!any_arrow_down(d)) return;
    if (game_.can_move_now() && !game_.buffered_move() && held_tick_ != game_.tick_count()) {
        held_tick_ = game_.tick_count();
        apply(Command::move(d));
    }
}

void GameScene::on_game_over() {
    paused_ = false;
    if (game_.status() != soldier::Status::Won || opts_.slot < 0) return;
    SaveSystem& save = res().save;
    const PlayerSlot& slot = save.slots[static_cast<std::size_t>(opts_.slot)];
    save.level_completed(opts_.slot, level_.number);
    try {
        new_record_ = save.offer_record(opts_.slot, level_.number, soldier::make_recording(game_, level_.number, slot.name));
    } catch (const std::exception& e) {
        SDL_Log("cannot save the record: %s", e.what());
    }
}

void GameScene::on_continue() {
    if (opts_.test_level) {
        app().quit(0);
        return;
    }
    app().replace_scene(make_menu_scene(shared_res()));
}

HudInfo GameScene::hud_info() const {
    HudInfo h;
    h.level = level_.number;
    h.paused = paused_;
    return h;
}

std::string GameScene::end_title() const {
    if (game_.status() == soldier::Status::Won && level_.number >= kLevelCount && !opts_.test_level) return "YOU WIN!";
    return LevelScene::end_title();
}

std::string GameScene::end_subtitle() const {
    if (game_.status() != soldier::Status::Won) return {};
    if (level_.number >= kLevelCount && !opts_.test_level) return "ALL 51 LEVELS ARE COMPLETE";
    if (new_record_) return "NEW RECORD: " + std::to_string(game_.tick_count()) + " TICKS";
    return {};
}

}  // namespace sg
