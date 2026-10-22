// Main menu: NEW GAME (choose an unlocked level), DEMO, OPTIONS, EXIT; the high score; the
// decorative snake chasing a rabbit around the menu box. Esc asks before exiting.
#include <SDL3/SDL_keycode.h>

#include <array>
#include <cmath>
#include <string>

#include <vs/engine/app.hpp>
#include <vs/engine/storage.hpp>
#include <vs/engine/ui_dialog.hpp>
#include <vs/engine/ui_menu.hpp>

#include <piton/level.hpp>

#include "launch_options.hpp"
#include "menu_snake.hpp"
#include "palette.hpp"
#include "scenes.hpp"
#include "screenshot.hpp"
#include "ui_draw.hpp"

namespace piton_game {
namespace {

// Lawn: 26 x 15 cells; the menu box sits two cells inside it.
constexpr int kLawnCols = 26;
constexpr int kLawnRows = 15;
constexpr vs::Vec2 kLawnOrigin{(1280 - 26 * 24) / 2.0f, 282};
constexpr vs::RectF kBox{kLawnOrigin.x + 48, kLawnOrigin.y + 48, 26 * 24 - 96, 15 * 24 - 96};

class MenuScene : public vs::Scene {
public:
    MenuScene(SharedPtr shared, MenuPage page) : shared_(std::move(shared)), page_(page) {}

    void on_enter() override {
        if (ready_) {
            shared_->music(Music::Menu);
            show_reset_notice();
            return;
        }
        ready_ = true;
        shared_ = Shared::ensure(shared_, app());
        dialog_ = std::make_unique<vs::Dialog>(&shared_->theme);
        dialog_->width = 520;
        main_ = std::make_unique<vs::Menu>(&shared_->theme);
        options_ = std::make_unique<vs::Menu>(&shared_->theme);
        build_main();
        build_options();
        load_levels();
        level_choice_ = shared_->save.level_reached;
        snake_.init(shared_->assets, kLawnOrigin, kLawnCols, kLawnRows);
        shared_->music(Music::Menu);
        show_reset_notice();

        if (launch_options().shot.active()) {
            const double t = launch_options().shot.time;
            for (double s = 0; s < t; s += 1.0 / 60.0) snake_.update(1.0 / 60.0);
            clock_ = t;
        }
    }

    void update(double dt) override {
        clock_ += dt;
        snake_.update(dt);
        const vs::Input& in = app().input();
        if (dialog_->is_open()) {
            dialog_->update(in);
            return;
        }
        switch (page_) {
        case MenuPage::Main:
            main_->update(in);
            if (in.pressed(SDLK_ESCAPE)) ask_exit();
            break;
        case MenuPage::Options:
            refresh_options();  // F11 / Alt+Enter may have changed the fullscreen state
            options_->update(in);
            if (in.pressed(SDLK_ESCAPE)) page_ = MenuPage::Main;
            break;
        case MenuPage::Levels: update_levels(in); break;
        }
    }

    bool on_quit_request() override { return true; }

    void render(vs::Renderer& r, double) override {
        const Assets& a = shared_->assets;
        draw_backdrop(r, a, 0.0f);
        r.draw(a.logo, std::nullopt, {std::round(640 - static_cast<float>(a.logo.width()) * 0.5f), 22});
        snake_.draw_lawn(r, a);
        snake_.draw_actors(r, a);
        r.fill_rect({kBox.x + 4, kBox.y + 6, kBox.w, kBox.h}, vs::rgba(0x00000050));
        r.fill_rect(kBox, pal::kPanel);
        r.draw_rect(kBox, pal::kWood, 3);

        switch (page_) {
        case MenuPage::Main: render_main(r); break;
        case MenuPage::Options: render_options(r); break;
        case MenuPage::Levels: render_levels(r); break;
        }
        draw_text(r, a.font8, "ARROWS - SELECT    ENTER - CONFIRM    ESC - BACK", {640, 700}, pal::kText,
                  vs::Align::Center, TextFx::Both);
        dialog_->render(r);
        if (shot_.active()) shot_.capture(app(), r);
    }

private:
    // Once per run, if the save file had been edited (Shared::progress_reset_notice).
    void show_reset_notice() {
        if (!shared_->progress_reset_notice) return;
        shared_->progress_reset_notice = false;
        dialog_->open("SAVE FILE WAS MODIFIED - PROGRESS RESET", vs::DialogButtons::Ok, [](vs::DialogResult) {},
                      vs::DialogResult::Ok);
    }

    // --- Pages -----------------------------------------------------------------------------
    void build_main() {
        main_->add("NEW GAME", [this] {
            level_choice_ = shared_->save.level_reached;
            page_ = MenuPage::Levels;
        });
        main_->add("DEMO", [this] { start(GameSetup{PlayMode::Demo, 1, 0, {}}); });
        main_->add("OPTIONS", [this] {
            options_->set_selected(0);
            page_ = MenuPage::Options;
        });
        main_->add("EXIT", [this] { ask_exit(); });
        main_->set_position({640, kBox.y + 34}, vs::Align::Center);
    }

    void build_options() {
        options_->add("");
        options_->item(0).on_adjust = [this](int d) { adjust_volume(shared_->save.sound_volume, d, true); };
        options_->add("");
        options_->item(1).on_adjust = [this](int d) { adjust_volume(shared_->save.music_volume, d, false); };
        options_->add("");
        options_->item(2).on_adjust = [this](int) {
            app().toggle_fullscreen();  // the listener (Shared) saves the setting
            refresh_options();
        };
        options_->add("BACK", [this] { page_ = MenuPage::Main; });
        options_->set_position({640, kBox.y + 64}, vs::Align::Center);
        refresh_options();
    }

    void refresh_options() {
        const SaveData& s = shared_->save;
        options_->item(0).label = "SOUND  " + percent(s.sound_volume);
        options_->item(1).label = "MUSIC  " + percent(s.music_volume);
        options_->item(2).label = std::string("FULLSCREEN ") + (app().is_fullscreen() ? " ON" : "OFF");
    }

    static std::string percent(int v) {
        std::string s = std::to_string(v) + "%";
        while (s.size() < 4) s.insert(s.begin(), ' ');
        return "< " + s + " >";
    }

    void adjust_volume(int& v, int d, bool sound) {
        v = std::clamp(v + d * 10, 0, 100);
        shared_->apply_volumes();
        shared_->save.save();
        refresh_options();
        if (sound) shared_->play(Sfx::Eat);
    }

    void load_levels() {
        for (int i = 0; i < piton::kLevelCount; ++i) {
            const std::filesystem::path p = vs::find_asset("levels/" + piton::levelFileName(i + 1));
            if (!p.empty()) level_ok_[static_cast<std::size_t>(i)] = piton::Level::load(p, levels_[static_cast<std::size_t>(i)]);
        }
    }

    void update_levels(const vs::Input& in) {
        const int reached = shared_->save.level_reached;
        if (in.repeated(SDLK_LEFT) || in.repeated(SDLK_DOWN)) {
            level_choice_ = level_choice_ > 1 ? level_choice_ - 1 : reached;
            shared_->play(Sfx::MenuMove, 0.7f);
        }
        if (in.repeated(SDLK_RIGHT) || in.repeated(SDLK_UP)) {
            level_choice_ = level_choice_ < reached ? level_choice_ + 1 : 1;
            shared_->play(Sfx::MenuMove, 0.7f);
        }
        if (in.pressed(SDLK_RETURN) || in.pressed(SDLK_KP_ENTER) || in.pressed(SDLK_SPACE) ||
            in.mouse_pressed(SDL_BUTTON_LEFT)) {
            shared_->play(Sfx::MenuSelect);
            start(GameSetup{PlayMode::Play, level_choice_, 0, {}});
        } else if (in.pressed(SDLK_ESCAPE) || in.mouse_pressed(SDL_BUTTON_RIGHT)) {
            shared_->play(Sfx::MenuMove, 0.7f, 0.8f);
            page_ = MenuPage::Main;
        }
    }

    void start(GameSetup setup) { app().replace_scene(make_game_scene(shared_, setup)); }

    void ask_exit() {
        dialog_->open("EXIT PITON?", vs::DialogButtons::YesNo, [this](vs::DialogResult res) {
            if (res == vs::DialogResult::Yes) app().quit(0);
        });
    }

    // --- Drawing ---------------------------------------------------------------------------
    void draw_marker(vs::Renderer& r, const vs::Menu& m) const {
        const vs::RectF rc = m.item_rect(m.selected());
        const float bob = std::round(static_cast<float>(std::sin(clock_ * 6.0)) * 2.0f);
        shared_->assets.ui.draw(r, "icon_rabbit", {rc.x - 30, rc.y + (rc.h - 22) * 0.5f + bob});
    }

    void render_main(vs::Renderer& r) {
        main_->render(r);
        draw_marker(r, *main_);
        const Assets& a = shared_->assets;
        const float y = kBox.bottom() - 40;
        a.ui.draw(r, "icon_star", {kBox.x + 92, y - 2});
        draw_text(r, a.font16, "HIGH SCORE " + padded(shared_->save.high_score, 6), {640 + 12, y}, pal::kGold,
                  vs::Align::Center);
    }

    void render_options(vs::Renderer& r) {
        draw_text(r, shared_->assets.font16, "OPTIONS", {640, kBox.y + 22}, pal::kGold, vs::Align::Center);
        options_->render(r);
        draw_marker(r, *options_);
    }

    void render_levels(vs::Renderer& r) {
        const Assets& a = shared_->assets;
        draw_text(r, a.font16, "SELECT LEVEL", {640, kBox.y + 18}, pal::kGold, vs::Align::Center);
        const bool can_left = shared_->save.level_reached > 1;
        const std::string label = std::string(can_left ? "< " : "  ") + "LEVEL " + padded(level_choice_, 2) +
                                  (can_left ? " >" : "  ");
        draw_text(r, a.font24, label, {640, kBox.y + 46}, pal::kText, vs::Align::Center);

        // Miniature of the level (5 px per cell).
        const std::size_t idx = static_cast<std::size_t>(level_choice_ - 1);
        const float cs = 5.0f;
        const vs::RectF map{640 - 19 * cs, kBox.y + 86, 38 * cs, 23 * cs};
        r.fill_rect(map.inset(-3), pal::kWood);
        if (level_ok_[idx]) {
            const piton::Level& lv = levels_[idx];
            r.fill_rect(map, vs::rgb(0x4E8A34));
            for (int y = 0; y < piton::kFieldHeight; ++y)
                for (int x = 0; x < piton::kFieldWidth; ++x)
                    if (lv.isBrick({x, y}))
                        r.fill_rect({map.x + static_cast<float>(x) * cs, map.y + static_cast<float>(y) * cs, cs, cs},
                                    vs::rgb(0xB5503C));
            const piton::Cell h = lv.startHead();
            r.fill_rect({map.x + static_cast<float>(h.x) * cs, map.y + static_cast<float>(h.y) * cs, cs, cs},
                        pal::kGold);
            if (!lv.name.empty())
                draw_text(r, a.font8, lv.name, {640, map.bottom() + 10}, pal::kText, vs::Align::Center);
        }
        draw_text(r, a.font8, "UNLOCKED: 1-" + std::to_string(shared_->save.level_reached), {640, kBox.bottom() - 20},
                  pal::kTextDim, vs::Align::Center);
    }

    SharedPtr shared_;
    MenuPage page_;
    bool ready_ = false;
    std::unique_ptr<vs::Dialog> dialog_;
    std::unique_ptr<vs::Menu> main_, options_;
    std::array<piton::Level, piton::kLevelCount> levels_{};
    std::array<bool, piton::kLevelCount> level_ok_{};
    int level_choice_ = 1;
    MenuSnake snake_;
    double clock_ = 0;
    ShotTaker shot_;
};

}  // namespace

std::unique_ptr<vs::Scene> make_menu_scene(SharedPtr shared, MenuPage page) {
    return std::make_unique<MenuScene>(std::move(shared), page);
}

}  // namespace piton_game
