#include "menu_scene.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>

#include <SDL3/SDL_log.h>

#include "game_scene.hpp"
#include "levels.hpp"
#include "replay_scene.hpp"

namespace sg {

namespace {

constexpr vs::RectF kPlayersBox{110, 196, 540, 460};
constexpr vs::RectF kItemsBox{690, 196, 480, 460};
constexpr float kRowH = 38;
constexpr float kItemH = 70;
const char* const kItemLabels[] = {"NEW GAME", "REPLAY LEVEL", "DELETE PLAYER", "OPTIONS", "EXIT"};

}  // namespace

MenuScene::MenuScene(ResourcesPtr r) : BaseScene(std::move(r)) {
    field_theme_ = res().theme;
    field_theme_.font = &res().font24;
}

void MenuScene::on_enter() {
    update_canvas_scaling(app());
    res().play_music("menu", true, 1.0);
    if (!first_enter_) return;
    first_enter_ = false;
    const std::string& m = res().cmd.menu_open;
    if (m == "name") {
        ask_name();
        field_.text = "RECRUIT";
    } else if (m == "level") {
        ask_level(false);
    } else if (m == "delete") {
        activate(DeletePlayer);
    } else if (m == "options") {
        options_.open();
    } else if (m == "exit") {
        ask_exit();
    } else if (m == "denied") {
        notice_.open("ACCESS DENIED");
    } else if (m == "norecord") {
        notice_.open("NO RECORD");
    } else if (m == "win") {
        notice_.open("LEVEL NOT FOUND. YOU WIN!", {}, colors::green);
    }
}

void MenuScene::on_exit() {
    if (prompt_open_) input().stop_text_input();
    res().save.save();
}

void MenuScene::select_slot(int s) {
    s = (s + kSlotCount) % kSlotCount;
    if (s != res().save.selected) {
        res().save.selected = s;
        res().play("menu_move", 0.6f);
    }
}

// ---- prompts -----------------------------------------------------------------------------

void MenuScene::open_prompt(std::string title, std::string hint, std::size_t max_len, bool digits, std::string initial,
                            std::function<void(const std::string&)> on_submit) {
    prompt_open_ = true;
    prompt_title_ = std::move(title);
    prompt_hint_ = std::move(hint);
    field_.text = std::move(initial);
    field_.max_length = max_len;
    field_.uppercase = true;
    field_.filter = digits ? vs::TextField::digit : vs::TextField::latin_upper_or_digit;
    field_.rect = {440, 352, 400, 56};
    field_.align = vs::Align::Center;
    field_.draw_box = false;
    field_.on_submit = [this, cb = std::move(on_submit)](const std::string& text) {
        close_prompt();
        res().play("menu_select", 0.7f);
        cb(text);
    };
    field_.on_cancel = [this] {
        close_prompt();
        res().play("menu_move", 0.5f, 0.8f);
    };
    input().start_text_input();
}

void MenuScene::close_prompt() {
    prompt_open_ = false;
    input().stop_text_input();
}

void MenuScene::ask_name() {
    const int s = slot();
    open_prompt("ENTER YOUR NAME", "A-Z, 0-9, UP TO 8 CHARACTERS", kNameLength, false, {}, [this, s](const std::string& name) {
        if (name.empty()) return;
        res().save.create_player(s, name);
    });
}

void MenuScene::ask_level(bool replay) {
    const PlayerSlot& p = res().save.slots[static_cast<std::size_t>(slot())];
    const int s = slot();
    if (replay) {
        open_prompt("REPLAY LEVEL", "COMPLETED LEVEL NUMBER", 3, true, {}, [this, s](const std::string& text) {
            const int level = std::atoi(text.c_str());
            if (level <= 0) return;
            const PlayerSlot& pl = res().save.slots[static_cast<std::size_t>(s)];
            if (level >= pl.level_reached || !res().save.has_record(s, level)) {
                res().play("error", 0.7f);
                notice_.open("NO RECORD");
                return;
            }
            start_replay(level);
        });
    } else {
        const int reached = std::min(p.level_reached, kLevelCount);
        open_prompt("LEVEL", "1 - " + std::to_string(reached), 3, true, std::to_string(reached),
                    [this, s](const std::string& text) {
                        const int level = std::atoi(text.c_str());
                        if (level <= 0) return;
                        const PlayerSlot& pl = res().save.slots[static_cast<std::size_t>(s)];
                        if (level > pl.level_reached) {
                            res().play("error", 0.7f);
                            notice_.open("ACCESS DENIED");
                            return;
                        }
                        if (level > kLevelCount) {
                            res().play("level_win", 0.8f);
                            notice_.open("LEVEL NOT FOUND. YOU WIN!", {}, colors::green);
                            return;
                        }
                        start_game(level);
                    });
    }
}

void MenuScene::ask_exit() { ask_quit(); }  // the same question as closing the window

void MenuScene::start_game(int level) {
    try {
        GameOptions o;
        o.slot = slot();
        app().replace_scene(std::make_unique<GameScene>(shared_res(), load_level(level), o));
    } catch (const std::exception& e) {
        SDL_Log("level %d: %s", level, e.what());
        notice_.open("LEVEL NOT FOUND");
    }
}

void MenuScene::start_replay(int level) {
    try {
        auto rec = res().save.load_record(slot(), level);
        if (!rec) {
            notice_.open("NO RECORD");
            return;
        }
        app().replace_scene(std::make_unique<ReplayScene>(shared_res(), load_level(level), *rec));
    } catch (const std::exception& e) {
        SDL_Log("replay %d: %s", level, e.what());
        notice_.open("NO RECORD");
    }
}

void MenuScene::activate(int item) {
    const PlayerSlot& p = res().save.slots[static_cast<std::size_t>(slot())];
    switch (item) {
    case NewGame:
        if (p.empty()) ask_name();
        else ask_level(false);
        break;
    case ReplayLevel:
        if (p.empty()) ask_name();
        else ask_level(true);
        break;
    case DeletePlayer:
        if (p.empty()) {
            res().play("error", 0.6f);
            break;
        }
        confirm_.open("ARE YOU SURE?", [this, s = slot()](bool yes) {
            if (yes) res().save.delete_player(s);
        });
        break;
    case Options: options_.open(); break;
    case Exit: ask_exit(); break;
    default: break;
    }
}

// ---- input -------------------------------------------------------------------------------

void MenuScene::tick(double dt) {
    const vs::Input& in = input();
    if (notice_.update(in, dt)) return;
    if (confirm_.update(in, res(), dt)) return;
    if (options_.update(app(), res(), dt)) return;
    if (prompt_open_) {
        field_.update(in, dt);
        return;
    }

    if (in.repeated(SDLK_TAB)) select_slot(slot() + (in.shift() ? -1 : 1));
    if (in.repeated(SDLK_LEFT) || in.repeated(SDLK_RIGHT)) {
        focus_ = focus_ == Focus::Players ? Focus::Items : Focus::Players;
        res().play("menu_move", 0.5f);
    }
    if (in.repeated(SDLK_UP) || in.repeated(SDLK_DOWN)) {
        const int d = in.repeated(SDLK_UP) ? -1 : 1;
        if (focus_ == Focus::Players) {
            select_slot(slot() + d);
        } else {
            item_ = (item_ + d + ItemCount) % ItemCount;
            res().play("menu_move", 0.6f);
        }
    }
    // Mouse: hovering selects, clicking activates.
    for (int i = 0; i < kSlotCount; ++i)
        if (slot_rect(i).contains(in.mouse()) && (in.mouse_moved() || in.mouse_pressed(SDL_BUTTON_LEFT))) {
            focus_ = Focus::Players;
            if (in.mouse_pressed(SDL_BUTTON_LEFT)) select_slot(i);
        }
    for (int i = 0; i < ItemCount; ++i)
        if (item_rect(i).contains(in.mouse()) && (in.mouse_moved() || in.mouse_pressed(SDL_BUTTON_LEFT))) {
            focus_ = Focus::Items;
            if (item_ != i) {
                item_ = i;
                res().play("menu_move", 0.6f);
            }
            if (in.mouse_pressed(SDL_BUTTON_LEFT)) {
                res().play("menu_select", 0.7f);
                activate(i);
                return;
            }
        }
    if (in.pressed(SDLK_RETURN) || in.pressed(SDLK_KP_ENTER) || in.pressed(SDLK_SPACE)) {
        res().play("menu_select", 0.7f);
        if (focus_ == Focus::Players)
            focus_ = Focus::Items;
        else
            activate(item_);
    }
    if (in.pressed(SDLK_ESCAPE)) ask_exit();
}

// ---- drawing -----------------------------------------------------------------------------

vs::RectF MenuScene::slot_rect(int i) const {
    return {kPlayersBox.x + 22, kPlayersBox.y + 66 + static_cast<float>(i) * kRowH, kPlayersBox.w - 44, kRowH - 4};
}

vs::RectF MenuScene::item_rect(int i) const {
    return {kItemsBox.x + 40, kItemsBox.y + 60 + static_cast<float>(i) * kItemH, kItemsBox.w - 80, kItemH - 14};
}

void MenuScene::draw_players(vs::Renderer& r) {
    draw_panel(r, res(), kPlayersBox, 235);
    draw_text(r, res().font16, "PLAYERS", {kPlayersBox.x + 70, kPlayersBox.y + 32}, colors::cyan);
    draw_text(r, res().font16, "LEVEL", {kPlayersBox.right() - 36, kPlayersBox.y + 32}, colors::cyan, vs::Align::Right);
    for (int i = 0; i < kSlotCount; ++i) {
        const vs::RectF rr = slot_rect(i);
        const PlayerSlot& p = res().save.slots[static_cast<std::size_t>(i)];
        const bool sel = i == slot();
        if (sel) {
            draw_inset(r, res(), rr);
            if (focus_ == Focus::Players) draw_marker(r, res(), {rr.x + 2, rr.center().y}, time());
        }
        const float ty = rr.y + 10;
        const vs::Color c = sel ? vs::rgb(0xFFFFFF) : (p.empty() ? colors::dim : colors::text);
        draw_text(r, res().font16, std::to_string(i + 1), {rr.x + 46, ty}, sel ? colors::accent : colors::dim,
                  vs::Align::Right);
        draw_text(r, res().font16, p.empty() ? "--------" : p.name, {rr.x + 70, ty}, c);
        std::string lvl = p.empty() ? "-" : std::to_string(p.level_reached);
        if (!p.empty() && p.level_reached > kLevelCount) lvl = "WIN";
        draw_text(r, res().font16, lvl, {rr.right() - 14, ty}, sel ? colors::accent : c, vs::Align::Right);
    }
}

void MenuScene::draw_items(vs::Renderer& r) {
    draw_panel(r, res(), kItemsBox, 235);
    for (int i = 0; i < ItemCount; ++i) {
        const vs::RectF rr = item_rect(i);
        const bool sel = i == item_;
        draw_button(r, res(), rr, sel && focus_ == Focus::Items);
        if (sel && focus_ == Focus::Items) draw_marker(r, res(), {rr.x - 6, rr.center().y}, time());
        const vs::Color c = sel ? (focus_ == Focus::Items ? colors::accent : vs::rgb(0xFFFFFF)) : colors::text;
        draw_text(r, res().font24, kItemLabels[i], {rr.center().x, rr.y + 16}, c, vs::Align::Center);
    }
}

void MenuScene::draw_prompt(vs::Renderer& r) {
    dim_screen(r, 150);
    const vs::RectF box{340, 230, 600, 250};
    draw_panel(r, res(), box);
    draw_text(r, res().font24, prompt_title_, {box.center().x, box.y + 40}, colors::accent, vs::Align::Center);
    draw_inset(r, res(), field_.rect);
    field_.render(r);
    draw_text(r, res().body_small, prompt_hint_ + "   ENTER OK   ESC CANCEL", {box.center().x, box.bottom() - 46},
              colors::dim, vs::Align::Center);
}

void MenuScene::draw(vs::Renderer& r) {
    r.draw(res().menu_bg, std::nullopt, {0, 0});
    r.fill_gradient({0, 0, 1280, 720}, vs::rgba(0x05080CB0), vs::rgba(0x05080C70));

    // Title.
    const float lw = 700;
    const float s = lw / static_cast<float>(res().logo.width());
    vs::DrawParams p;
    p.scale = {s, s};
    vs::DrawParams sh = p;
    sh.tint = vs::Color{0, 0, 0, 150};
    const float bob = static_cast<float>(std::sin(time() * 1.3)) * 2.0f;
    r.draw(res().logo, std::nullopt, {640 - lw / 2 + 6, 34 + 8 + bob}, sh);
    r.draw(res().logo, std::nullopt, {640 - lw / 2, 34 + bob}, p);
    draw_text(r, res().font16, "2.0", {640 + lw / 2 + 6, 128 + bob}, colors::accent);

    draw_players(r);
    draw_items(r);

    draw_text(r, res().body_small, "TAB / ARROWS: SELECT PLAYER     ENTER: CHOOSE     ESC: EXIT", {640, 680}, colors::dim,
              vs::Align::Center);

    if (prompt_open_) draw_prompt(r);
    options_.render(r, res(), app(), time());
    confirm_.render(r, res());
    notice_.render(r, res());
}

std::unique_ptr<vs::Scene> make_menu_scene(ResourcesPtr res) { return std::make_unique<MenuScene>(std::move(res)); }

}  // namespace sg
