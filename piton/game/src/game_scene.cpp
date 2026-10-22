// The game screen: one level of play (or the demo, or the editor's test-play). Runs the
// piton::Game at 8 ticks per second and handles READY, pause, death, GAME OVER and level
// completion.
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_timer.h>

#include <algorithm>
#include <memory>
#include <optional>

#include <vs/engine/app.hpp>
#include <vs/engine/error.hpp>
#include <vs/engine/storage.hpp>
#include <vs/engine/ui_dialog.hpp>

#include <piton/demo_pilot.hpp>
#include <piton/game.hpp>
#include <piton/level.hpp>

#include "game_overlays.hpp"
#include "launch_options.hpp"
#include "palette.hpp"
#include "playfield_view.hpp"
#include "scenes.hpp"
#include "scoring.hpp"
#include "screenshot.hpp"
#include "ui_draw.hpp"

namespace piton_game {
namespace {

using piton::Dir;

enum class Phase { Ready, Playing, Dying, GameOver, Complete };

constexpr double kReadyTime = 2.0;      // "READY" then "GO!"
constexpr double kDemoReadyTime = 1.2;
constexpr double kDyingTime = 1.4;      // dead head on screen before GAME OVER
constexpr double kCompleteTime = 4.5;   // the level complete banner

piton::Level load_level(const GameSetup& s) {
    piton::Level level;
    std::string error;
    const std::filesystem::path path = s.mode == PlayMode::Test ? vs::utf8_to_path(s.test_path)
                                                                : vs::asset_path("levels/" + piton::levelFileName(s.level));
    if (!piton::Level::load(path, level, &error)) throw vs::Error("Cannot load level " + error);  // error names the file
    if (s.mode == PlayMode::Test) {
        // Hard errors make the level unplayable (or impossible to complete); warnings are fine.
        const piton::LevelCheck check = piton::checkLevel(level);
        if (!check.playable()) {
            std::string text = "Cannot test-play " + vs::path_to_utf8(path) + ":";
            for (const std::string& e : check.errors) text += "\n - " + e;
            throw vs::Error(text);
        }
    }
    return level;
}

std::optional<Dir> key_dir(SDL_Keycode k) {
    switch (k) {
    case SDLK_UP: case SDLK_W: return Dir::Up;
    case SDLK_DOWN: case SDLK_S: return Dir::Down;
    case SDLK_LEFT: case SDLK_A: return Dir::Left;
    case SDLK_RIGHT: case SDLK_D: return Dir::Right;
    default: return std::nullopt;
    }
}

class GameScene : public vs::Scene {
public:
    GameScene(SharedPtr shared, GameSetup setup)
        : shared_(std::move(shared)), setup_(std::move(setup)), level_(load_level(setup_)),
          seed_(launch_options().shot.active() ? 7919u * static_cast<unsigned>(setup_.level) : SDL_GetTicksNS()),
          game_(level_, seed_), score_(setup_.score) {}

    void on_enter() override {
        if (ready_) return;
        ready_ = true;
        shared_ = Shared::ensure(shared_, app());
        if (setup_.high_at_start < 0) setup_.high_at_start = shared_->save.high_score;  // the game starts
        dialog_ = std::make_unique<vs::Dialog>(&shared_->theme);
        dialog_->width = 520;
        view_.init(shared_->assets, game_, static_cast<unsigned>(setup_.level * 31 + 5));
        shared_->music(Music::Game);
        if (launch_options().shot.active()) prepare_shot();
    }

    void on_event(const SDL_Event& e) override {
        // Losing focus pauses (also during READY / GO, which would otherwise run on).
        if (e.type == SDL_EVENT_WINDOW_FOCUS_LOST && pausable() && !shot_.active()) paused_ = true;
        // Every arrow pressed is a turn, in order (two presses within one logic step both count).
        if (e.type == SDL_EVENT_KEY_DOWN && !e.key.repeat)
            if (const std::optional<Dir> d = key_dir(e.key.key)) turns_.push_back(*d);
    }

    bool on_quit_request() override {
        save_progress();
        return true;
    }

    void update(double dt) override {
        clock_ += dt;
        const std::vector<Dir> turns = std::move(turns_);  // arrows pressed since the last step
        turns_.clear();
        if (shot_.active()) {  // frozen for the screenshot
            phase_time_ += dt;
            return;
        }
        const vs::Input& in = app().input();
        if (dialog_->is_open()) {  // the phase clock (READY, dying, complete) stands still
            dialog_->update(in);
            return;
        }
        if (setup_.mode == PlayMode::Demo && in.any_pressed()) {
            back_to_menu();
            return;
        }
        if (in.pressed(SDLK_ESCAPE) && phase_ != Phase::GameOver) {
            on_escape();
            return;
        }
        if (pausable() && in.pressed(SDLK_P)) {
            paused_ = !paused_;
            shared_->play(Sfx::MenuSelect, 0.6f);
        }
        if (paused_) return;

        phase_time_ += dt;
        view_.update(dt, game_);
        switch (phase_) {
        case Phase::Ready: update_ready(turns); break;
        case Phase::Playing: update_playing(turns, dt); break;
        case Phase::Dying:
            tick_time_ = std::min(kTickSeconds, tick_time_ + dt);
            if (phase_time_ >= kDyingTime) enter_game_over();
            break;
        case Phase::GameOver:
            if (phase_time_ > 0.6 && in.any_pressed()) leave_after_game_over();
            break;
        case Phase::Complete:
            tick_time_ = std::min(kTickSeconds, tick_time_ + dt);
            if (phase_time_ >= kCompleteTime || (phase_time_ > 1.5 && in.any_pressed())) next_level();
            break;
        }
    }

    void render(vs::Renderer& r, double alpha) override {
        const Assets& a = shared_->assets;
        draw_backdrop(r, a, 0.62f);
        draw_hud(r, a, hud());
        draw_wood_frame(r, a, {kFieldOrigin.x, kFieldOrigin.y, kFieldW, kFieldH}, 16.0f);
        view_.draw(r, a, game_, progress(alpha));
        draw_mode_label(r, a, setup_.mode, clock_);
        render_overlay(r);
        dialog_->render(r);
        if (shot_.active()) shot_.capture(app(), r);
    }

private:
    // --- Flow ------------------------------------------------------------------------------
    void set_phase(Phase p) {
        phase_ = p;
        phase_time_ = 0;
    }

    // P and focus loss pause READY / GO and the play itself (not the demo).
    bool pausable() const { return (phase_ == Phase::Ready || phase_ == Phase::Playing) && setup_.mode != PlayMode::Demo; }

    void queue_turns(const std::vector<Dir>& turns) {
        if (setup_.mode == PlayMode::Demo) return;
        for (Dir d : turns) game_.queueTurn(d);  // the game keeps up to kMaxQueuedTurns
    }

    void update_ready(const std::vector<Dir>& turns) {
        queue_turns(turns);
        const double total = setup_.mode == PlayMode::Demo ? kDemoReadyTime : kReadyTime;
        if (phase_time_ >= total) {
            set_phase(Phase::Playing);
            tick_time_ = 0;
        }
    }

    void update_playing(const std::vector<Dir>& turns, double dt) {
        // A level without a free cell for the first rabbit is complete from the start.
        if (game_.state() == piton::GameState::LevelComplete) {
            enter_complete();
            return;
        }
        queue_turns(turns);
        tick_time_ += dt;
        while (tick_time_ >= kTickSeconds && phase_ == Phase::Playing) {
            tick_time_ -= kTickSeconds;
            do_tick();
        }
    }

    void do_tick() {
        if (setup_.mode == PlayMode::Demo) pilot_.drive(game_);
        const std::vector<piton::Event>& events = game_.tick();
        handle_events(events);
    }

    void handle_events(const std::vector<piton::Event>& events) {
        const int points = rabbit_points(level_number());
        for (const piton::Event& e : events) {
            switch (e.type) {
            case piton::EventType::Turned: shared_->play(Sfx::Turn, setup_.mode == PlayMode::Demo ? 0.25f : 0.45f); break;
            case piton::EventType::Ate:
                score_ += points;
                shared_->play(Sfx::Eat, 0.9f, 0.95f + 0.1f * static_cast<float>(e.value % 5) / 4.0f);
                break;
            case piton::EventType::Died:
                shared_->play(Sfx::Die);
                shared_->stop_music(0.6);
                view_.shake(0.35);
                set_phase(Phase::Dying);
                break;
            case piton::EventType::LevelComplete: enter_complete(); break;
            case piton::EventType::Moved:
            case piton::EventType::RabbitSpawned: break;
            }
        }
        view_.on_events(game_, events, points, kTickSeconds);
        if (setup_.mode == PlayMode::Play && score_ > shared_->save.high_score) shared_->save.high_score = score_;
    }

    void enter_complete() {
        set_phase(Phase::Complete);
        const int bonus = level_bonus(level_number());
        score_ += bonus;
        bonus_ = bonus;
        shared_->play(Sfx::LevelComplete);
        shared_->music(Music::Victory, false);
        if (setup_.mode == PlayMode::Play) {
            if (score_ > shared_->save.high_score) shared_->save.high_score = score_;
            shared_->save.level_reached =
                std::max(shared_->save.level_reached, std::min(setup_.level + 1, piton::kLevelCount));
            shared_->save.save();
        }
    }

    void enter_game_over() {
        if (setup_.mode == PlayMode::Demo) {  // the demo just goes on with the next level
            next_level();
            return;
        }
        set_phase(Phase::GameOver);
        new_record_ = setup_.mode == PlayMode::Play && score_ > setup_.high_at_start;  // a tie is no record
        save_progress();
    }

    void leave_after_game_over() {
        if (setup_.mode == PlayMode::Test) {
            app().quit(0);
            return;
        }
        back_to_menu();
    }

    void next_level() {
        switch (setup_.mode) {
        case PlayMode::Test: app().quit(0); return;
        case PlayMode::Demo: {
            GameSetup s = setup_;
            s.level = setup_.level % piton::kLevelCount + 1;
            s.score = phase_ == Phase::Complete ? score_ : 0;
            if (s.level == 1) s.score = 0;
            app().replace_scene(make_game_scene(shared_, s));
            return;
        }
        case PlayMode::Play:
            if (setup_.level >= piton::kLevelCount) {
                app().replace_scene(make_win_scene(shared_, score_, setup_.high_at_start));
                return;
            }
            GameSetup s = setup_;
            s.level = setup_.level + 1;
            s.score = score_;
            app().replace_scene(make_game_scene(shared_, s));
            return;
        }
    }

    void on_escape() {
        if (setup_.mode == PlayMode::Test) {
            app().quit(0);
            return;
        }
        dialog_->open("QUIT TO MENU?", vs::DialogButtons::YesNo,
                      [this](vs::DialogResult res) {
                          if (res == vs::DialogResult::Yes) back_to_menu();
                      },
                      vs::DialogResult::No);
    }

    void back_to_menu() {
        save_progress();
        app().replace_scene(make_menu_scene(shared_));
    }

    void save_progress() {
        if (setup_.mode != PlayMode::Play) return;
        if (score_ > shared_->save.high_score) shared_->save.high_score = score_;
        shared_->save.save();
    }

    int level_number() const { return setup_.mode == PlayMode::Test ? 1 : setup_.level; }

    // Sub-tick progress of the snake's motion for rendering.
    float progress(double alpha) const {
        if (shot_.active() && phase_ == Phase::Playing) return shot_.alpha();
        double t = tick_time_;
        if (phase_ == Phase::Playing && !paused_ && !dialog_->is_open()) t += alpha * app().config().logic_step;
        return static_cast<float>(std::clamp(t / kTickSeconds, 0.0, 1.0));
    }

    HudInfo hud() const {
        HudInfo h;
        h.level = level_number();
        h.level_name = level_.name;
        h.eaten = game_.rabbitsEaten();
        h.target = game_.rabbitsToWin();
        h.score = score_;
        h.high = setup_.mode == PlayMode::Play ? std::max(shared_->save.high_score, score_) : shared_->save.high_score;
        h.mode = setup_.mode;
        return h;
    }

    // --- Overlays --------------------------------------------------------------------------
    void render_overlay(vs::Renderer& r) {
        const Assets& a = shared_->assets;
        const float appear = static_cast<float>(std::min(1.0, phase_time_ / 0.35));
        switch (phase_) {
        case Phase::Ready: {
            if (paused_) {
                draw_banner(r, a, "PAUSED", pal::kGold, {{"PRESS P TO CONTINUE", 8, pal::kText}}, 1.0f);
                break;
            }
            const double total = setup_.mode == PlayMode::Demo ? kDemoReadyTime : kReadyTime;
            const bool go = phase_time_ > total * 0.7;
            const double since = go ? phase_time_ - total * 0.7 : phase_time_;
            draw_big_word(r, a, go ? "GO!" : "READY", go ? pal::kGreen : pal::kGold, static_cast<float>(since / 0.3));
            break;
        }
        case Phase::Playing:
            if (paused_)
                draw_banner(r, a, "PAUSED", pal::kGold, {{"PRESS P TO CONTINUE", 8, pal::kText}}, 1.0f);
            break;
        case Phase::Dying: break;
        case Phase::GameOver: {
            std::vector<BannerLine> lines{{"SCORE " + padded(score_, 6), 16, pal::kText},
                                          {"RABBITS " + std::to_string(game_.rabbitsEaten()) + "/" +
                                               std::to_string(game_.rabbitsToWin()),
                                           16, pal::kText}};
            if (new_record_) lines.push_back({"NEW HIGH SCORE!", 16, pal::kGold});
            lines.push_back({"PRESS ANY KEY", 8, pal::kTextDim});
            draw_banner(r, a, "GAME OVER", pal::kRed, lines, appear);
            break;
        }
        case Phase::Complete: {
            const std::string title = "LEVEL " + std::to_string(level_number()) + " COMPLETE";
            std::vector<BannerLine> lines{{"LEVEL BONUS +" + std::to_string(bonus_), 16, pal::kGold},
                                          {"SCORE " + padded(score_, 6), 16, pal::kText}};
            if (setup_.mode == PlayMode::Play && setup_.level < piton::kLevelCount)
                lines.push_back({"NEXT: LEVEL " + std::to_string(setup_.level + 1), 8, pal::kGreen});
            draw_banner(r, a, title, pal::kGreen, lines, appear);
            break;
        }
        }
    }

    // --- Debug screenshots -----------------------------------------------------------------
    void fast_forward(int ticks) {
        shared_->muted = true;
        for (int i = 0; i < ticks && game_.state() == piton::GameState::Running; ++i) {
            pilot_.drive(game_);
            handle_events(game_.tick());
            view_.update(kTickSeconds, game_);
        }
        shared_->muted = false;
    }

    // Plays on until the neck is a corner (the head swings) or the tail swings round a corner.
    void fast_forward_to_turn(bool tail) {
        auto corner = [](const piton::Segment& s) { return s.in != s.out; };
        auto tail_turns = [&] {
            const std::vector<piton::Segment> seg = game_.segments();
            const std::optional<Dir> motion = piton::dirBetween(game_.prevTail(), game_.tail());
            return motion && *motion != seg.back().out;
        };
        for (int i = 0; i < 600 && game_.state() == piton::GameState::Running; ++i) {
            fast_forward(1);
            if (tail ? tail_turns() : corner(game_.segments()[1])) return;
        }
    }

    void prepare_shot() {
        const ShotOptions& s = launch_options().shot;
        if (s.ticks > 0) {
            set_phase(Phase::Playing);
            fast_forward(s.ticks);
            if (!s.until_turn.empty()) fast_forward_to_turn(s.until_turn == "tail");
        } else {
            phase_time_ = 0.5;  // READY on screen
        }
        if (s.scene == "pause") paused_ = true;
        if (s.scene == "quit") on_escape();
        if (s.scene == "gameover") {
            shared_->muted = true;
            for (int i = 0; i < 3000 && game_.state() == piton::GameState::Running; ++i) {
                // Steer into the nearest obstacle.
                std::optional<Dir> fatal;
                for (Dir d : piton::kAllDirs)
                    if (d != piton::opposite(game_.heading()) && !game_.isFree(piton::step(game_.head(), d)) &&
                        piton::step(game_.head(), d) != game_.tail())
                        fatal = d;
                if (fatal)
                    game_.steer(*fatal);
                else
                    pilot_.drive(game_);
                handle_events(game_.tick());
            }
            shared_->muted = false;
            enter_game_over();
            phase_time_ = 1.0;
        }
        if (s.scene == "complete") {
            fast_forward(100000);
            phase_time_ = 1.0;
        }
        if (phase_ == Phase::Playing) tick_time_ = 0;
    }

    SharedPtr shared_;
    GameSetup setup_;
    piton::Level level_;
    std::uint64_t seed_;
    piton::Game game_;
    piton::DemoPilot pilot_;
    PlayfieldView view_;
    std::unique_ptr<vs::Dialog> dialog_;
    Phase phase_ = Phase::Ready;
    double phase_time_ = 0;
    double tick_time_ = 0;  // time since the last tick
    double clock_ = 0;
    int score_ = 0;
    int bonus_ = 0;
    bool paused_ = false;
    bool new_record_ = false;
    std::vector<Dir> turns_;  // arrow presses collected from the SDL events, in order
    bool ready_ = false;
    ShotTaker shot_;
};

}  // namespace

std::unique_ptr<vs::Scene> make_game_scene(SharedPtr shared, GameSetup setup) {
    return std::make_unique<GameScene>(std::move(shared), std::move(setup));
}

}  // namespace piton_game
