// "YOU WIN!" after the 10th level: the final score, the high score, falling stars and rabbits.
#include <cmath>
#include <vector>

#include <vs/engine/app.hpp>

#include <piton/types.hpp>

#include "easing.hpp"
#include "launch_options.hpp"
#include "palette.hpp"
#include "scenes.hpp"
#include "screenshot.hpp"
#include "ui_draw.hpp"

namespace piton_game {
namespace {

class WinScene : public vs::Scene {
public:
    WinScene(SharedPtr shared, int score, int previous_high)
        : shared_(std::move(shared)), score_(score), previous_high_(previous_high) {}

    void on_enter() override {
        if (ready_) return;
        ready_ = true;
        shared_ = Shared::ensure(shared_, app());
        SaveData& s = shared_->save;
        const int before = previous_high_ >= 0 ? previous_high_ : s.high_score;
        record_ = score_ > before;  // a tie is not a new record
        if (score_ > s.high_score) s.high_score = score_;
        s.level_reached = piton::kLevelCount;
        s.save();
        shared_->music(Music::Victory, false);
        for (int i = 0; i < 40; ++i) confetti_.push_back(spawn(i, true));
        if (launch_options().shot.active()) advance(launch_options().shot.time);
    }

    void update(double dt) override {
        if (shot_.active()) return;
        advance(dt);
        if (time_ > 1.2 && app().input().any_pressed()) app().replace_scene(make_menu_scene(shared_));
        // After the jingle: back to the menu music.
        if (!music_back_ && time_ > 6.0) {
            music_back_ = true;
            shared_->music(Music::Menu);
        }
    }

    void render(vs::Renderer& r, double) override {
        const Assets& a = shared_->assets;
        draw_backdrop(r, a, 0.35f);
        for (const Bit& b : confetti_) {
            if (b.star)
                a.ui.draw(r, "icon_star", {std::round(b.x), std::round(b.y)});
            else
                a.ui.draw(r, "icon_rabbit", {std::round(b.x), std::round(b.y)});
        }
        r.draw(a.logo, std::nullopt, {std::round(640 - static_cast<float>(a.logo.width()) * 0.5f), 40});

        const float pop = ease::out_back(ease::phase(time_, 0.2, 0.6));
        const vs::RectF panel{340, 300, 600, 330};
        if (pop > 0) {
            draw_panel(r, a, panel);
            const float bounce = std::round(static_cast<float>(std::sin(time_ * 4.0)) * 4.0f);
            draw_text(r, a.font48, "YOU WIN!", {640, panel.y + 40 + bounce}, pal::kGold, vs::Align::Center, TextFx::Both);
            draw_text(r, a.font16, "ALL 10 LEVELS COMPLETE", {640, panel.y + 120}, pal::kGreen, vs::Align::Center);
            draw_text(r, a.font16, "FINAL SCORE " + padded(score_, 6), {640, panel.y + 170}, pal::kText, vs::Align::Center);
            draw_text(r, a.font16, "HIGH SCORE  " + padded(shared_->save.high_score, 6), {640, panel.y + 206},
                      record_ ? pal::kGold : pal::kText, vs::Align::Center);
            if (record_) draw_text(r, a.font8, "NEW HIGH SCORE!", {640, panel.y + 240}, pal::kGold, vs::Align::Center);
            if (time_ > 1.2)
                draw_text(r, a.font8, "PRESS ANY KEY", {640, panel.bottom() - 40}, pal::kTextDim, vs::Align::Center);
        }
        shot_.capture(app(), r);
    }

private:
    struct Bit {
        float x, y, speed, sway;
        bool star;
    };

    Bit spawn(int i, bool anywhere) {
        seed_ = seed_ * 1103515245u + 12345u;
        const float rx = static_cast<float>((seed_ >> 8) % 1280);
        const float ry = anywhere ? static_cast<float>((seed_ >> 4) % 720) : -30.0f;
        return {rx, ry, 40.0f + static_cast<float>((seed_ >> 12) % 60), static_cast<float>(i % 7), (i % 3) != 0};
    }

    void advance(double dt) {
        time_ += dt;
        int i = 0;
        for (Bit& b : confetti_) {
            b.y += b.speed * static_cast<float>(dt);
            b.x += static_cast<float>(std::sin(time_ * 1.5 + b.sway)) * 20.0f * static_cast<float>(dt);
            if (b.y > 730) b = spawn(i, false);
            ++i;
        }
    }

    SharedPtr shared_;
    int score_;
    int previous_high_;
    bool ready_ = false;
    bool record_ = false;
    bool music_back_ = false;
    double time_ = 0;
    unsigned seed_ = 2026;
    std::vector<Bit> confetti_;
    ShotTaker shot_;
};

}  // namespace

std::unique_ptr<vs::Scene> make_win_scene(SharedPtr shared, int score, int previous_high) {
    return std::make_unique<WinScene>(std::move(shared), score, previous_high);
}

}  // namespace piton_game
